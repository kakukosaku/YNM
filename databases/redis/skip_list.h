// 跳跃表 - 有序集合

typedef struct skipListNode {
    //后退指针
    struct skipListNode *backward;
    // 分值
    double score;
    // 成员对象指针
    void *obj;
    // 层
    struct skipListLevel {
        // 前进指针
        struct skipListNode *forward;
        // 跨度, 下一个节点与当前节点的距离
        unsigned int span;
    } level[];
} skipListNode;


typedef struct skipList{
    // 表头结点&表尾结点
    skipListNode *header, *tail;
    // 表中节点的数量
    unsigned long length;
    // 表中层数最大的节点的层数
    int level;
} skipList;
