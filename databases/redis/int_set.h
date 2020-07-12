// 整数集合

#include "predefine.h"

typedef struct intSet {
    // 编码方式, 用以决定下contents具体是多少位的数据类型, 节约内在空间
    uint32_t encoding;
    // 集合包含的元素数量
    uint32_t length;
    // 保存元素的数组, 有序排列, 无重复项
    int8_t contents[];
} intSet;