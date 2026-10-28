#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int value;
    struct node *next;
} Node;

Node *append(Node **head, Node **tail, int v) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (n == NULL) {
        printf("内存分配失败\n");
        exit(1);
    }
    n->value = v;
    n->next = NULL;
    if (*head == NULL) {
        *head = n;
    } else {
        (*tail)->next = n;
    }
    *tail = n;
    return n;
}

void print_list(Node *head) {
    for (Node *p = head; p != NULL; p = p->next) {
        printf("%d ", p->value);
    }
    printf("\n");
}

Node *reverse(Node *head) {
    Node *prev = NULL, *cur = head;
    while (cur != NULL) {
        Node *next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}

int find_from(Node *head, int v, int start) {
    Node *p = head;
    int idx = 1;
    while (p != NULL) {
        if (idx > start && p->value == v) {
            return idx;
        }
        p = p->next;
        idx++;
    }
    return -1;
}

void free_list(Node *head) {
    while (head != NULL) {
        Node *t = head->next;
        free(head);
        head = t;
    }
}

int main(void) {
    Node *head = NULL, *tail = NULL;
    int init[] = {1, 3, 5, 7, 5, 9};
    int n = (int)(sizeof(init) / sizeof(init[0]));

    for (int i = 0; i < n; i++) {
        append(&head, &tail, init[i]);
    }

    printf("原链表：");
    print_list(head);

    head = reverse(head);

    printf("反序后：");
    print_list(head);

    int a = find_from(head, 5, 0);
    int b = (a < 0) ? -1 : find_from(head, 5, a);

    if (a < 0) {
        printf("没有找到值为 5 的节点\n");
    } else {
        printf("第一个值为 5 的节点序号：%d\n", a);
    }
    if (b < 0) {
        printf("下一个值为 5 的节点序号：-1（不存在）\n");
    } else {
        printf("下一个值为 5 的节点序号：%d\n", b);
    }

    free_list(head);
    return 0;
}
