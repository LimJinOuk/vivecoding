#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_DEGREE 100

void remove_spaces(char* str) {
    char* i = str;
    char* j = str;
    while (*j != 0) {
        *i = *j++;
        if (*i != ' ') i++;
    }
    *i = 0;
}

void replace_double_star(char* str) {
    int len = strlen(str);
    for (int i = 0; i < len - 1; i++) {
        if (str[i] == '*' && str[i + 1] == '*') {
            str[i] = '^';
            for (int j = i + 1; j < len; j++) {
                str[j] = str[j + 1];
            }
            len--;
        }
    }
}

int is_blank_line(const char* str) {
    for (int i = 0; str[i]; i++) {
        if (!isspace(str[i])) return 0;
    }
    return 1;
}

void parse_polynomial(const char* line, int* coeff) {
    memset(coeff, 0, sizeof(int) * (MAX_DEGREE + 1));
    int sign = 1, i = 0;

    while (line[i]) {
        int coef = 0, degree = 0, has_coef = 0;

        if (line[i] == '+') {
            sign = 1;
            i++;
        }
        else if (line[i] == '-') {
            sign = -1;
            i++;
        }

        while (isdigit(line[i])) {
            coef = coef * 10 + (line[i] - '0');
            has_coef = 1;
            i++;
        }

        if (line[i] == 'x') {
            i++;
            if (!has_coef) coef = 1;
            if (line[i] == '^') {
                i++;
                while (isdigit(line[i])) {
                    degree = degree * 10 + (line[i] - '0');
                    i++;
                }
            }
            else {
                degree = 1;
            }
        }
        else {
            degree = 0;
        }

        if (degree <= MAX_DEGREE) {
            coeff[degree] += sign * coef;
        }
    }
}

void print_polynomial(int* coeff) {
    int first = 1;
    for (int i = MAX_DEGREE; i >= 0; i--) {
        if (coeff[i] != 0) {
            if (!first && coeff[i] > 0) printf(" + ");
            if (coeff[i] < 0) printf(" - ");
            if (abs(coeff[i]) != 1 || i == 0)
                printf("%d", abs(coeff[i]));
            else if (abs(coeff[i]) == 1 && i == 0)
                printf("1");

            if (i >= 1) {
                printf("x");
                if (i > 1) printf("^%d", i);
            }
            first = 0;
        }
    }
    if (first) printf("0");
    printf("\n");
}

void add_polynomials(int* a, int* b, int* result) {
    for (int i = 0; i <= MAX_DEGREE; i++) {
        result[i] = a[i] + b[i];
    }
}

void multiply_polynomials(int* a, int* b, int* result) {
    memset(result, 0, sizeof(int) * (MAX_DEGREE * 2 + 1));
    for (int i = 0; i <= MAX_DEGREE; i++) {
        for (int j = 0; j <= MAX_DEGREE; j++) {
            if (i + j <= MAX_DEGREE * 2)
                result[i + j] += a[i] * b[j];
        }
    }
}

int main() {
    FILE* file = fopen("input.txt", "r");
    if (!file) {
        printf("파일을 열 수 없습니다.\n");
        return 1;
    }

    char buffer[1024];
    char line1[1024] = "", line2[1024] = "";
    int read_count = 0;
    int pair = 1;

    int poly1[MAX_DEGREE + 1], poly2[MAX_DEGREE + 1];
    int sum[MAX_DEGREE + 1], product[MAX_DEGREE * 2 + 1];

    while (fgets(buffer, sizeof(buffer), file)) {
        // 줄 끝 개행 제거
        buffer[strcspn(buffer, "\r\n")] = 0;

        if (is_blank_line(buffer)) continue;

        if (read_count == 0) {
            strcpy(line1, buffer);
            read_count = 1;
        }
        else {
            strcpy(line2, buffer);
            read_count = 0;

            // 처리 시작
            printf("\n▶ [%d번째 다항식 쌍]\n", pair++);

            replace_double_star(line1);
            replace_double_star(line2);
            remove_spaces(line1);
            remove_spaces(line2);

            parse_polynomial(line1, poly1);
            parse_polynomial(line2, poly2);

            printf("정리된 첫 번째 다항식: ");
            print_polynomial(poly1);
            printf("정리된 두 번째 다항식: ");
            print_polynomial(poly2);

            add_polynomials(poly1, poly2, sum);
            printf("두 다항식의 합: ");
            print_polynomial(sum);

            multiply_polynomials(poly1, poly2, product);
            printf("두 다항식의 곱: ");
            print_polynomial(product);
        }
    }

    fclose(file);
    return 0;
}