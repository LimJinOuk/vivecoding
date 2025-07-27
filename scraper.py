import requests
from bs4 import BeautifulSoup
import time
import urllib.parse
from collections import deque
import re
import logging

# 로깅 설정
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class IMDBCrawler:
    def __init__(self):
        self.base_url = "https://www.imdb.com"
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        })
        self.actor_cache = {}  # 배우명 -> IMDB ID 캐시
        self.movie_cache = {}  # 배우 ID -> 영화 리스트 캐시
        self.cast_cache = {}   # 영화 ID -> 배우 리스트 캐시
        
    def search_actor(self, actor_name):
        """배우 이름으로 IMDB에서 검색하여 배우 ID 반환"""
        if actor_name in self.actor_cache:
            logger.info(f"📋 캐시: {actor_name} 정보 로드됨")
            return self.actor_cache[actor_name]
            
        logger.info(f"🔍 배우 검색: '{actor_name}' 검색 중...")
        start_time = time.time()
        
        search_url = f"{self.base_url}/find/?q={urllib.parse.quote(actor_name)}&ref_=nv_sr_sm"
        
        try:
            response = self.session.get(search_url)
            response.raise_for_status()
            soup = BeautifulSoup(response.content, 'html.parser')
            
            # 배우 결과에서 첫 번째 항목 찾기
            actor_links = soup.find_all('a', class_='ipc-metadata-list-summary-item__t')
            
            for link in actor_links:
                href = link.get('href', '')
                if '/name/' in href:
                    actor_id = href.split('/name/')[1].split('/')[0]
                    actor_url = f"{self.base_url}{href}"
                    
                    elapsed_time = time.time() - start_time
                    logger.info(f"✅ 배우 발견: {actor_name} -> {actor_id} ({elapsed_time:.2f}초)")
                    
                    self.actor_cache[actor_name] = (actor_id, actor_url)
                    return actor_id, actor_url
                    
        except Exception as e:
            logger.error(f"❌ 배우 검색 중 오류: {e}")
            
        elapsed_time = time.time() - start_time
        logger.warning(f"❌ 배우 검색 실패: '{actor_name}' ({elapsed_time:.2f}초)")
        return None, None
    
    def get_actor_movies(self, actor_id, actor_url, actor_name=""):
        """배우의 출연 영화 목록 반환"""
        if actor_id in self.movie_cache:
            logger.info(f"📋 캐시: {actor_name}의 영화 목록 로드됨")
            return self.movie_cache[actor_id]
            
        logger.info(f"🎬 {actor_name}의 영화 목록 크롤링 중...")
        start_time = time.time()
        
        try:
            response = self.session.get(actor_url)
            response.raise_for_status()
            soup = BeautifulSoup(response.content, 'html.parser')
            
            movies = []
            
            # 다양한 방법으로 영화 제목 추출 시도
            # 방법 1: 기본 filmography 링크
            movie_elements = soup.find_all('a', class_='ipc-metadata-list-summary-item__t')
            
            for element in movie_elements:
                href = element.get('href', '')
                if '/title/' in href:
                    movie_id = href.split('/title/')[1].split('/')[0]
                    movie_title = element.get_text(strip=True)
                    movie_url = f"{self.base_url}{href}"
                    if movie_title:  # 제목이 있는 경우만 추가
                        movies.append((movie_id, movie_title, movie_url))
            
            # 방법 2: 다른 클래스명으로 시도
            if not movies:
                movie_links = soup.find_all('a', href=re.compile(r'/title/tt\d+/'))
                for link in movie_links:
                    href = link.get('href', '')
                    movie_id = href.split('/title/')[1].split('/')[0]
                    movie_title = link.get_text(strip=True)
                    movie_url = f"{self.base_url}{href}"
                    if movie_title and len(movie_title) > 1:  # 의미있는 제목인 경우만
                        movies.append((movie_id, movie_title, movie_url))
            
            # 방법 3: 더 넓은 범위로 검색
            if not movies:
                all_links = soup.find_all('a')
                for link in all_links:
                    href = link.get('href', '')
                    if '/title/tt' in href:
                        try:
                            movie_id = href.split('/title/')[1].split('/')[0]
                            movie_title = link.get_text(strip=True)
                            movie_url = f"{self.base_url}{href}"
                            if movie_title and len(movie_title) > 1 and not movie_title.isdigit():
                                movies.append((movie_id, movie_title, movie_url))
                        except:
                            continue
            
            # 중복 제거 및 필터링
            seen_movies = set()
            filtered_movies = []
            for movie_id, movie_title, movie_url in movies:
                if movie_id not in seen_movies and movie_title and len(movie_title.strip()) > 1:
                    seen_movies.add(movie_id)
                    filtered_movies.append((movie_id, movie_title.strip(), movie_url))
            
            movies = filtered_movies[:50]  # 상위 50개 영화만 선택
            
            elapsed_time = time.time() - start_time
            logger.info(f"✅ 영화 목록 완료: {len(movies)}편 발견 ({elapsed_time:.1f}초)")
            
            self.movie_cache[actor_id] = movies
            return movies
            
        except Exception as e:
            logger.error(f"❌ 영화 목록 크롤링 오류: {e}")
            return []
    
    def get_movie_cast(self, movie_id, movie_url, movie_title=""):
        """영화의 출연 배우 목록 반환"""
        if movie_id in self.cast_cache:
            return self.cast_cache[movie_id]
            
        start_time = time.time()
        
        try:
            response = self.session.get(movie_url)
            response.raise_for_status()
            soup = BeautifulSoup(response.content, 'html.parser')
            
            cast = []
            
            # 다양한 방법으로 배우 이름 추출
            # 방법 1: 표준 cast 링크
            cast_elements = soup.find_all('a', href=re.compile(r'/name/nm\d+/'))
            
            for element in cast_elements:
                href = element.get('href', '')
                if '/name/' in href:
                    actor_id = href.split('/name/')[1].split('/')[0]
                    # 다양한 방법으로 배우 이름 추출 시도
                    actor_name = element.get_text(strip=True)
                    
                    # 이름이 비어있다면 부모 요소에서 찾기
                    if not actor_name:
                        parent = element.find_parent()
                        if parent:
                            actor_name = parent.get_text(strip=True)
                    
                    # 이름이 여전히 비어있다면 alt 속성에서 찾기
                    if not actor_name:
                        img = element.find('img')
                        if img and img.get('alt'):
                            actor_name = img.get('alt').strip()
                    
                    if actor_name and actor_id.startswith('nm') and len(actor_name) > 1:
                        cast.append((actor_id, actor_name))
            
            # 방법 2: 이미지 alt 속성에서 배우 이름 추출
            if len(cast) < 5:  # 배우가 너무 적다면 다른 방법 시도
                images = soup.find_all('img', alt=True)
                for img in images:
                    alt_text = img.get('alt', '').strip()
                    parent_link = img.find_parent('a')
                    if parent_link and '/name/nm' in parent_link.get('href', ''):
                        href = parent_link.get('href', '')
                        actor_id = href.split('/name/')[1].split('/')[0]
                        if alt_text and len(alt_text) > 1 and not alt_text.lower().startswith('poster'):
                            cast.append((actor_id, alt_text))
            
            # 중복 제거 및 필터링
            seen_actors = set()
            filtered_cast = []
            for actor_id, actor_name in cast:
                if actor_id not in seen_actors and actor_name and len(actor_name.strip()) > 1:
                    seen_actors.add(actor_id)
                    filtered_cast.append((actor_id, actor_name.strip()))
            
            cast = filtered_cast[:30]  # 상위 30명 배우만 선택
            
            elapsed_time = time.time() - start_time
            
            self.cast_cache[movie_id] = cast
            return cast
            
        except Exception as e:
            logger.error(f"❌ 출연진 크롤링 오류: {e}")
            return []

class KevinBaconGame:
    def __init__(self):
        self.crawler = IMDBCrawler()
        
    def find_connection(self, start_actor, target_actor, max_depth=6, progress_callback=None):
        """두 배우 간의 연결 경로 찾기 (BFS 알고리즘 사용)"""
        logger.info(f"🎯 Kevin Bacon Game 시작: '{start_actor}' → '{target_actor}'")
        
        if progress_callback:
            progress_callback("starting", 0, "", "", 10.0, f"'{start_actor}'과 '{target_actor}' 연결 찾기 시작")
        
        # 시작 배우와 목표 배우의 IMDB ID 찾기
        start_id, start_url = self.crawler.search_actor(start_actor)
        if not start_id:
            if progress_callback:
                progress_callback("error", 0, "", "", 0, f"'{start_actor}' 배우를 찾을 수 없습니다.")
            return None
            
        target_id, target_url = self.crawler.search_actor(target_actor)
        if not target_id:
            if progress_callback:
                progress_callback("error", 0, "", "", 0, f"'{target_actor}' 배우를 찾을 수 없습니다.")
            return None
            
        if start_id == target_id:
            if progress_callback:
                progress_callback("completed", 0, "", "", 100.0, "두 배우가 동일합니다!")
            return []
        
        if progress_callback:
            progress_callback("searching", 0, "", "", 20.0, "연결 경로 탐색 시작")
        
        # BFS를 위한 큐와 방문 기록
        queue = deque([(start_id, start_actor, [])])  # (배우ID, 배우명, 경로)
        visited_actors = {start_id}
        
        for depth in range(max_depth):
            step_progress = 20.0 + (depth / max_depth) * 70.0
            
            if progress_callback:
                progress_callback(
                    "searching", 
                    depth + 1, 
                    "", 
                    "", 
                    step_progress,
                    f"{depth + 1}단계 탐색 중... (대기 중인 배우: {len(queue)}명)"
                )
            
            level_size = len(queue)
            
            if level_size == 0:
                break
                
            for i in range(level_size):
                if not queue:
                    break
                    
                current_actor_id, current_actor_name, path = queue.popleft()
                
                if progress_callback:
                    current_progress = step_progress + (i / level_size) * (70.0 / max_depth)
                    progress_callback(
                        "searching",
                        depth + 1,
                        current_actor_name,
                        "",
                        current_progress,
                        f"'{current_actor_name}'의 영화 목록 확인 중..."
                    )
                
                # 현재 배우의 영화 목록 가져오기
                movies = self.crawler.get_actor_movies(current_actor_id, 
                                                     f"{self.crawler.base_url}/name/{current_actor_id}/",
                                                     current_actor_name)
                
                for j, (movie_id, movie_title, movie_url) in enumerate(movies):
                    # 영화 제목이 비어있다면 건너뛰기
                    if not movie_title or len(movie_title.strip()) < 2:
                        continue
                    
                    if progress_callback:
                        movie_progress = step_progress + (i / level_size + j / len(movies) / level_size) * (70.0 / max_depth)
                        progress_callback(
                            "searching",
                            depth + 1,
                            current_actor_name,
                            movie_title,
                            movie_progress,
                            f"영화 '{movie_title}' 출연진 확인 중..."
                        )
                    
                    # 각 영화의 출연진 가져오기
                    cast = self.crawler.get_movie_cast(movie_id, movie_url, movie_title)
                    
                    for actor_id, actor_name in cast:
                        if actor_id == target_id:
                            # 목표 배우 발견!
                            final_path = path + [{
                                "from_actor": current_actor_name,
                                "movie": movie_title,
                                "to_actor": actor_name
                            }]
                            
                            if progress_callback:
                                progress_callback(
                                    "completed", 
                                    depth + 1,
                                    current_actor_name,
                                    movie_title,
                                    100.0,
                                    f"연결 발견! {len(final_path)}단계"
                                )
                            
                            logger.info(f"🎉 연결 발견: {len(final_path)}단계")
                            return final_path
                            
                        if actor_id not in visited_actors:
                            visited_actors.add(actor_id)
                            new_path = path + [{
                                "from_actor": current_actor_name,
                                "movie": movie_title,
                                "to_actor": actor_name
                            }]
                            queue.append((actor_id, actor_name, new_path))
        
        if progress_callback:
            progress_callback("completed", max_depth, "", "", 100.0, f"{max_depth}단계 내에서 연결을 찾을 수 없습니다.")
        
        logger.warning(f"❌ {max_depth}단계 내에서 연결을 찾을 수 없습니다.")
        return None
        
    def print_result(self, path):
        """결과 출력"""
        if path is None:
            print("\n❌ 연결을 찾을 수 없습니다.")
            return
            
        if len(path) == 0:
            print("\n✨ 두 배우가 동일합니다.")
            return
            
        print(f"\n🏆 연결 경로 발견! ({len(path)}단계)")
        print("=" * 50)
        for i, step in enumerate(path, 1):
            print(f"{i}. {step['from_actor']} → 「{step['movie']}」 → {step['to_actor']}")
        print("=" * 50)

def main():
    """테스트용 메인 함수"""
    game = KevinBaconGame()
    
    print("🎭 IMDB Kevin Bacon Game (BeautifulSoup)")
    print("=" * 50)
    
    start_actor = input("기준 배우 이름: ").strip()
    if not start_actor:
        print("⚠️ 배우 이름을 입력해주세요.")
        return
        
    target_actor = input("목표 배우 이름: ").strip()
    if not target_actor:
        print("⚠️ 배우 이름을 입력해주세요.")
        return
        
    # 진행상황 콜백 함수
    def progress_callback(status, step, actor, movie, percentage, message):
        print(f"[{percentage:5.1f}%] {message}")
    
    # 케빈 베이컨 게임 실행
    total_start_time = time.time()
    path = game.find_connection(start_actor, target_actor, progress_callback=progress_callback)
    total_elapsed_time = time.time() - total_start_time
    
    # 결과 출력
    game.print_result(path)
    print(f"\n⏱️ 총 소요시간: {total_elapsed_time:.1f}초")

if __name__ == "__main__":
    main()