// 双向链表

typedef struct listNode {
    // 前一个节点
    struct listNode *prev;
    // 后一个节点
    struct listNode *next;
    // 节点"内容"指针
    void *value;
}listNode;


typedef struct list {
    // 头指针
    listNode *head;
    // 尾指针
    listNode *tail;
    // 链表长度
    unsigned long len;
    // ...
}list;
