// 简单动态字符串
struct sdshdr {
    // 已使用的长度
    int len;
    // buf数组中未使用的字节的长度
    int free;
    // "字符"数组
    char buf[];
};
