// 有序列表(list)

/**
 * 实质为一维数组, 通过限制不同标识(功能)位实现丰富信息的(元素变长的)列表
 * 
 * [ 
 *     1_zip_list_total_byte_len,
 *     2_zip_list_tail_ptr,
 *     3_zip_list_total_node_len,
 *     4_entrys, 
 *     5_zip_list_end_flag,
 * ]
 * 
 * strcut info of entry:
 * 
 * previous_entry_len.encoding.content
 * 
 * previous_entry_len: 变长字节保存, 小于254->1字节, 大于254->5字节.
 * encoding: 决定content实际存储内容类型&占用空间
 * content: 实际Payload
 * 
 * 关于连锁更新:
 * 
 * 前面介绍到, previous 为前一个entry占用的字节长度, 这样, 从zip_list_tail_ptr可以很方便的从尾端遍历, 但同时意味着: 
 * 在元素中插入*大元素*时, 需要维护后一entry的previous_entry_len属性, 试想, 如果连续多个entry大小介于254~257时, 
 * 由于某个*大元素*的插入, 导致后一个previous_entry_len变为5字节表示, 导致该元素大小也超过254, 得使需要维护再后一个
 * entry.previous_entry_len, 发生连锁更新.
 * 
 */