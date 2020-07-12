// Redis 对象

#include "predefine.h"

typedef struct redisObject {
    // 类型, 使用 *4 bits*
    unsigned type : 4;
    // 编码, 决定底层数据结构类型
    unsigned encoding : 4;
    // 指向底层数据结构的指针
    void *ptr;
    // 引用计数
    int refcount;
    unsigned lru : LRU_BITS; /* lru time (relative to server.lruclock) */
} redisObject;