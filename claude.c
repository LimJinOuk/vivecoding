#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_N 1000
#define INF 999999

// 인접 리스트의 노드 구조체
typedef struct AdjNode {
    int vertex;
    struct AdjNode* next;
} AdjNode;

// 그래프 구조체
typedef struct Graph {
    int numVertices;
    AdjNode** adjList;
} Graph;

// 그래프 초기화
Graph* createGraph(int vertices) {
    Graph* graph = (Graph*)malloc(sizeof(Graph));
    graph->numVertices = vertices;
    graph->adjList = (AdjNode**)malloc((vertices + 1) * sizeof(AdjNode*));
    
    // 모든 인접 리스트를 NULL로 초기화 (1번부터 시작하므로 vertices+1)
    for (int i = 0; i <= vertices; i++) {
        graph->adjList[i] = NULL;
    }
    
    return graph;
}

// 새로운 인접 노드 생성
AdjNode* createAdjNode(int vertex) {
    AdjNode* newNode = (AdjNode*)malloc(sizeof(AdjNode));
    newNode->vertex = vertex;
    newNode->next = NULL;
    return newNode;
}

// 단방향 간선 추가 (중복 방지를 위해)
void addEdge(Graph* graph, int src, int dest) {
    AdjNode* newNode = createAdjNode(dest);
    newNode->next = graph->adjList[src];
    graph->adjList[src] = newNode;
}

// 파일에서 그래프 읽기
Graph* readGraphFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return NULL;
    }
    
    int numVertices;
    fscanf(file, "%d", &numVertices);
    
    Graph* graph = createGraph(numVertices);
    
    char line[1000];
    fgets(line, sizeof(line), file); // 첫 줄의 개행문자 처리
    
    // 각 줄을 읽어서 연결 정보 파싱
    while (fgets(line, sizeof(line), file)) {
        char* token = strtok(line, " \n");
        if (!token) continue;
        
        int src = atoi(token);
        
        // 나머지 토큰들은 연결된 노드들
        while ((token = strtok(NULL, " \n")) != NULL) {
            int dest = atoi(token);
            addEdge(graph, src, dest);
        }
    }
    
    fclose(file);
    return graph;
}

// 그래프 출력 (디버깅용)
void printGraph(Graph* graph) {
    for (int i = 1; i <= graph->numVertices; i++) {
        printf("%d: ", i);
        AdjNode* temp = graph->adjList[i];
        while (temp) {
            printf("%d ", temp->vertex);
            temp = temp->next;
        }
        printf("\n");
    }
}

// 큐 구조체 (BFS용)
typedef struct Queue {
    int items[MAX_N];
    int front;
    int rear;
} Queue;

// 큐 초기화
Queue* createQueue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    q->front = -1;
    q->rear = -1;
    return q;
}

// 큐가 비어있는지 확인
int isEmpty(Queue* q) {
    return q->rear == -1;
}

// 큐에 요소 추가
void enqueue(Queue* q, int value) {
    if (q->rear == MAX_N - 1) {
        printf("큐가 가득 참\n");
    } else {
        if (q->front == -1) {
            q->front = 0;
        }
        q->rear++;
        q->items[q->rear] = value;
    }
}

// 큐에서 요소 제거
int dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("큐가 비어있음\n");
        return -1;
    } else {
        int item = q->items[q->front];
        q->front++;
        if (q->front > q->rear) {
            q->front = q->rear = -1;
        }
        return item;
    }
}

// BFS로 시작 노드에서 모든 노드까지의 거리 계산
int* bfs(Graph* graph, int start) {
    int* dist = (int*)malloc((graph->numVertices + 1) * sizeof(int));
    int* visited = (int*)malloc((graph->numVertices + 1) * sizeof(int));
    
    // 초기화
    for (int i = 1; i <= graph->numVertices; i++) {
        dist[i] = INF;
        visited[i] = 0;
    }
    
    Queue* q = createQueue();
    
    dist[start] = 0;
    visited[start] = 1;
    enqueue(q, start);
    
    while (!isEmpty(q)) {
        int current = dequeue(q);
        
        AdjNode* temp = graph->adjList[current];
        while (temp) {
            int neighbor = temp->vertex;
            if (!visited[neighbor]) {
                visited[neighbor] = 1;
                dist[neighbor] = dist[current] + 1;
                enqueue(q, neighbor);
            }
            temp = temp->next;
        }
    }
    
    free(visited);
    free(q);
    return dist;
}

// 두 노드 간의 최단 거리 계산
int getDistance(Graph* graph, int src, int dest) {
    int* dist = bfs(graph, src);
    int result = dist[dest];
    free(dist);
    return result == INF ? -1 : result;
}

// (1) 나와 너의 거리는? - 67번과 26번 사이의 거리
void question1(Graph* graph) {
    int dist = getDistance(graph, 67, 26);
    printf("(1) 67번과 26번 사이의 거리: %d\n", dist);
}

// (2) Lone Wolf는? - 연결된 컴포넌트의 수 계산
int countComponents(Graph* graph) {
    int* visited = (int*)malloc((graph->numVertices + 1) * sizeof(int));
    int components = 0;
    
    // 초기화
    for (int i = 1; i <= graph->numVertices; i++) {
        visited[i] = 0;
    }
    
    // 각 노드에서 DFS 시작
    for (int i = 1; i <= graph->numVertices; i++) {
        if (!visited[i]) {
            // 새로운 컴포넌트 발견
            components++;
            
            // DFS로 연결된 모든 노드 방문 표시
            int* stack = (int*)malloc(graph->numVertices * sizeof(int));
            int top = 0;
            stack[top] = i;
            
            while (top >= 0) {
                int current = stack[top--];
                if (!visited[current]) {
                    visited[current] = 1;
                    
                    AdjNode* temp = graph->adjList[current];
                    while (temp) {
                        if (!visited[temp->vertex]) {
                            stack[++top] = temp->vertex;
                        }
                        temp = temp->next;
                    }
                }
            }
            free(stack);
        }
    }
    
    free(visited);
    return components;
}

void question2(Graph* graph) {
    int components = countComponents(graph);
    printf("(2) 연결된 컴포넌트 수 (Lone Wolf): %d\n", components);
}

// (3) 3단계 이내에 가장 많은 사람에게 도달할 수 있는 사람
int countReachable(Graph* graph, int start, int maxDist) {
    int* dist = bfs(graph, start);
    int count = 0;
    
    for (int i = 1; i <= graph->numVertices; i++) {
        if (dist[i] <= maxDist) {
            count++;
        }
    }
    
    free(dist);
    return count;
}

void question3(Graph* graph) {
    int bestPerson = 1;
    int maxReachable = 0;
    
    for (int i = 1; i <= graph->numVertices; i++) {
        int reachable = countReachable(graph, i, 3);
        if (reachable > maxReachable) {
            maxReachable = reachable;
            bestPerson = i;
        }
    }
    
    printf("(3) 3단계 이내에 가장 많은 사람(%d명)에게 도달 가능한 사람: %d번\n", 
           maxReachable, bestPerson);
}

// (4) 3단계 이내로 모든 사람에게 연락하기 위한 최소 인원 조합
void question4(Graph* graph) {
    int* covered = (int*)malloc((graph->numVertices + 1) * sizeof(int));
    int* selected = (int*)malloc((graph->numVertices + 1) * sizeof(int));
    int selectedCount = 0;
    
    // 초기화
    for (int i = 1; i <= graph->numVertices; i++) {
        covered[i] = 0;
        selected[i] = 0;
    }
    
    // 그리디 방식: 매번 가장 많은 미커버 노드를 커버하는 노드 선택
    while (1) {
        int bestNode = -1;
        int bestNewCovered = 0;
        
        // 각 노드에 대해 새로 커버할 수 있는 노드 수 계산
        for (int i = 1; i <= graph->numVertices; i++) {
            if (selected[i]) continue;
            
            int* dist = bfs(graph, i);
            int newCovered = 0;
            
            for (int j = 1; j <= graph->numVertices; j++) {
                if (!covered[j] && dist[j] <= 3) {
                    newCovered++;
                }
            }
            
            if (newCovered > bestNewCovered) {
                bestNewCovered = newCovered;
                bestNode = i;
            }
            
            free(dist);
        }
        
        if (bestNode == -1 || bestNewCovered == 0) break;
        
        // 선택된 노드로 커버 업데이트
        selected[bestNode] = 1;
        selectedCount++;
        
        int* dist = bfs(graph, bestNode);
        for (int j = 1; j <= graph->numVertices; j++) {
            if (dist[j] <= 3) {
                covered[j] = 1;
            }
        }
        free(dist);
    }
    
    printf("(4) 3단계 이내 전체 커버를 위한 최소 인원(%d명): ", selectedCount);
    for (int i = 1; i <= graph->numVertices; i++) {
        if (selected[i]) {
            printf("%d ", i);
        }
    }
    printf("\n");
    
    free(covered);
    free(selected);
}

// 메인 함수
int main() {
    printf("=== 케빈 베이컨 게임 ===\n");
    
    // 그래프 파일 읽기
    Graph* graph = readGraphFromFile("kb.txt");
    if (!graph) {
        printf("그래프를 읽을 수 없습니다.\n");
        return 1;
    }
    
    printf("그래프 로드 완료: %d명의 사람\n\n", graph->numVertices);
    
    // 4가지 질문 해결
    question1(graph);
    question2(graph);
    question3(graph);
    question4(graph);
    
    // 메모리 해제
    for (int i = 1; i <= graph->numVertices; i++) {
        AdjNode* temp = graph->adjList[i];
        while (temp) {
            AdjNode* toDelete = temp;
            temp = temp->next;
            free(toDelete);
        }
    }
    free(graph->adjList);
    free(graph);
    
    return 0;
}