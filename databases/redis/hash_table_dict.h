// hash table 实现的key-value paris

// This define just for demonstration

#include "predefine.h"

typedef struct dictEntry {
  // key
  void *key;
  // value
  union {
    void *val;
    uint64_t u64;
    int64_t s64;
  } v;
} dictEntry;

typedef struct dictht {
  // dictEntry类型的指针数组
  dictEntry **table;
  // 哈希表大小
  unsigned long size;
  // 哈希表大小掩码, 用于计算索引值
  unsigned long sizeMask;
  // 该哈希表已有节点的数量
  unsigned long used;
} dictht;

typedef struct dictType {
    // 计算哈希值的函数
    unsigned int (*hashFunction)(const void *key);
    // ...
};

typedef struct dict {
    // 类型特定函数, 用以实现多态
    dictType *type;
    // 私有数据
    void *privData;
    // 哈希表
    dictht ht[2];
    // rehash索引
    int rehashidx; /*rehashing not in process if rehashidx == -1 */
} dict;
