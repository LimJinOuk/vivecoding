#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_LINE 8192
#define MAX_TOKENS 1000
#define MAX_TOKEN_LEN 64
#define STACK_SIZE 1000

typedef struct {
    double data[STACK_SIZE];
    int top;
} DoubleStack;

typedef struct {
    char data[STACK_SIZE][MAX_TOKEN_LEN];
    int top;
} StringStack;

void initDoubleStack(DoubleStack *s) { s->top = -1; }
int isEmptyDouble(DoubleStack *s) { return s->top == -1; }
void pushDouble(DoubleStack *s, double val) { s->data[++s->top] = val; }
double popDouble(DoubleStack *s) { return s->top < 0 ? 0 : s->data[s->top--]; }

void initStringStack(StringStack *s) { s->top = -1; }
int isEmptyString(StringStack *s) { return s->top == -1; }
void pushString(StringStack *s, const char *val) { strcpy(s->data[++s->top], val); }
char* popString(StringStack *s) { return s->top < 0 ? NULL : s->data[s->top--]; }
char* peekString(StringStack *s) { return s->top < 0 ? NULL : s->data[s->top]; }

int precedence(char *op) {
    if (!strcmp(op, "+") || !strcmp(op, "-")) return 1;
    if (!strcmp(op, "*") || !strcmp(op, "/")) return 2;
    if (!strcmp(op, "^")) return 3;
    return 0;
}

int isOperator(char *token) {
    return !strcmp(token, "+") || !strcmp(token, "-") ||
           !strcmp(token, "*") || !strcmp(token, "/") || !strcmp(token, "^");
}

int isNumber(const char *token) {
    char *end;
    strtod(token, &end);
    return end != token && *end == '\0';
}

void preprocess_line(char *line) {
    char temp[MAX_LINE];
    int i = 0, j = 0;

    while (line[i]) {
        // 유니코드 하이픈 — or –
        if ((unsigned char)line[i] == 0xE2 &&
            (unsigned char)line[i + 1] == 0x80 &&
            ((unsigned char)line[i + 2] == 0x93 || (unsigned char)line[i + 2] == 0x94)) {
            temp[j++] = '-';
            i += 3;
        }
        // '**' → '^'
        else if (line[i] == '*' && line[i + 1] == '*') {
            temp[j++] = '^';
            i += 2;
        }
        // e 상수
        else if (line[i] == 'e' && (i == 0 || !isalnum(line[i - 1])) && !isalnum(line[i + 1])) {
            strcpy(&temp[j], "2.7182818");
            j += strlen("2.7182818");
            i++;
        }
        // f 제거 (float 접미사)
        else if (line[i] == 'f') {
            i++; // skip
        }
        // -( → -1*( 치환
        else if (line[i] == '-' && line[i + 1] == '(') {
            strcpy(&temp[j], "-1*(");
            j += 4;
            i += 2;
        }
        // 나머지 그대로 복사
        else {
            temp[j++] = line[i++];
        }
    }

    temp[j] = '\0';
    strcpy(line, temp);
}


int tokenize(char *line, char tokens[][MAX_TOKEN_LEN]) {
    int count = 0;
    char *p = line;

    while (*p) {
        while (isspace(*p)) p++;
        if (!*p) break;

        // 괄호 안 음수 처리: (-1.0) → 하나의 숫자
        if (*p == '(' && (*(p + 1) == '-' || *(p + 1) == '+') &&
            (isdigit(*(p + 2)) || *(p + 2) == '.')) {
            int i = 0;
            p++; // skip '('
            tokens[count][i++] = *p++;
            while (*p && (isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' ||
                          *p == '-' || *p == '+')) {
                tokens[count][i++] = *p++;
                if (i >= MAX_TOKEN_LEN - 1) break;
            }
            tokens[count][i] = '\0';
            count++;
            if (*p == ')') p++;
            continue;
        }

        // 단항 연산자 위치에서 -3.5e0 인식
        if (strchr("+-", *p) &&
            (count == 0 || isOperator(tokens[count - 1]) || !strcmp(tokens[count - 1], "("))) {
            int i = 0;
            tokens[count][i++] = *p++;
            while (isdigit(*p) || *p == '.') tokens[count][i++] = *p++;
            if (*p == 'e' || *p == 'E') {
                tokens[count][i++] = *p++;
                if (*p == '-' || *p == '+') tokens[count][i++] = *p++;
                while (isdigit(*p)) tokens[count][i++] = *p++;
            }
            tokens[count][i] = '\0';
            count++;
            continue;
        }

        if (strchr("()+-*/^", *p)) {
            tokens[count][0] = *p++;
            tokens[count][1] = '\0';
            count++;
        } else if (isdigit(*p) || *p == '.') {
            int i = 0;
            while (*p && (isdigit(*p) || *p == '.' || *p == 'e' || *p == 'E' ||
                          *p == '-' || *p == '+')) {
                tokens[count][i++] = *p++;
                if (i >= MAX_TOKEN_LEN - 1) break;
            }
            tokens[count][i] = '\0';
            count++;
        } else {
            return -1;
        }
    }

    return count;
}

int toPostfix(char tokens[][MAX_TOKEN_LEN], int count, char output[][MAX_TOKEN_LEN]) {
    StringStack opStack;
    initStringStack(&opStack);
    int outCount = 0;

    for (int i = 0; i < count; i++) {
        if (isNumber(tokens[i])) {
            strcpy(output[outCount++], tokens[i]);
        } else if (!strcmp(tokens[i], "(")) {
            pushString(&opStack, tokens[i]);
        } else if (!strcmp(tokens[i], ")")) {
            while (!isEmptyString(&opStack) && strcmp(peekString(&opStack), "(")) {
                strcpy(output[outCount++], popString(&opStack));
            }
            if (isEmptyString(&opStack)) return -1;
            popString(&opStack);
        } else if (isOperator(tokens[i])) {
            while (!isEmptyString(&opStack) && isOperator(peekString(&opStack)) &&
                   precedence(peekString(&opStack)) >= precedence(tokens[i])) {
                strcpy(output[outCount++], popString(&opStack));
            }
            pushString(&opStack, tokens[i]);
        } else {
            return -1;
        }
    }

    while (!isEmptyString(&opStack)) {
        if (!strcmp(peekString(&opStack), "(")) return -1;
        strcpy(output[outCount++], popString(&opStack));
    }

    return outCount;
}

int evaluatePostfix(char tokens[][MAX_TOKEN_LEN], int count, double *result) {
    DoubleStack stack;
    initDoubleStack(&stack);

    for (int i = 0; i < count; i++) {
        if (isNumber(tokens[i])) {
            pushDouble(&stack, strtod(tokens[i], NULL));
        } else if (isOperator(tokens[i])) {
            if (stack.top < 1) return 0;
            double b = popDouble(&stack);
            double a = popDouble(&stack);
            double r = 0;
            if (!strcmp(tokens[i], "+")) r = a + b;
            else if (!strcmp(tokens[i], "-")) r = a - b;
            else if (!strcmp(tokens[i], "*")) r = a * b;
            else if (!strcmp(tokens[i], "/")) {
                if (b == 0) return 0;
                r = a / b;
            } else if (!strcmp(tokens[i], "^")) r = pow(a, b);
            pushDouble(&stack, r);
        } else {
            return 0;
        }
    }

    if (stack.top != 0) return 0;
    *result = popDouble(&stack);
    return 1;
}

int isInvalidLine(const char *line) {
    for (int i = 0; line[i]; i++) {
        if (line[i] == '{' || line[i] == '}' || line[i] == '[' || line[i] == ']')
            return 1;
    }
    return strlen(line) == 0 || strspn(line, " \t\r\n") == strlen(line);
}

int main() {
    FILE *fp = fopen("input.txt", "r");
    if (!fp) {
        perror("input.txt");
        return 1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;

        if (isInvalidLine(line)) {
            printf("Invalid Expression\n");
            continue;
        }

        preprocess_line(line);

        char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
        int tokenCount = tokenize(line, tokens);
        if (tokenCount < 1) {
            printf("Invalid Expression\n");
            continue;
        }

        char postfix[MAX_TOKENS][MAX_TOKEN_LEN];
        int postfixCount = toPostfix(tokens, tokenCount, postfix);
        if (postfixCount < 1) {
            printf("Invalid Expression\n");
            continue;
        }

        // 출력: 후위 표기식
        printf("Postfix: ");
        for (int i = 0; i < postfixCount; i++) {
            printf("%s ", postfix[i]);
        }
        printf("\n");

        // 출력: 결과
        double result;
        if (!evaluatePostfix(postfix, postfixCount, &result)) {
            printf("Result: Invalid Expression\n");
        } else {
            printf("Result: %.2f\n", result);
        }
    }

    fclose(fp);
    return 0;
}

