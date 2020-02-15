# Efficient Redis

本篇介绍了Redis的五种基本数据类型及其C数据结构表示, 并简要(非全部)总结其运行&资源(内存)利用的高效性.

## 基本数据类型

1. [Simple Dynamic String](simple_dynamic_string.h)

- 额外属性(len, free)记录当前总长及剩余空间
- 在上述属性帮助下, 实现二进制安全(\0, 特殊字节) & C 原生string相关函数复用
- 数组预分配&懒回收
- 由于Redis data type 与 implement data struct 解耦的好处, 带来在处理不同"类型"的string的使用不同数据结构, 共三种底层数据结构(int: 针对number类型的string, 显然节省空间&带来了方便一套操作, embstr: 较小string, 一次内存操作申请/销毁redis_object+string_data_struct_obj, row如前所述的好处)

2. [double Link List](double_link_list.h)

- 头尾节点(head, tail), 前驱后劲(prev, next), 额外属性(len)包含总体信息.

3. [Hash Table Dict](hash_table_dict.h)

- 额外属性(table_size, used)包含总体信息.
- 拉链法解决hash collision & 渐进式rehash
- hash_table的存储元素指针

4. [Skip List](skip_list.h)

- 跨度信息(span)等的应用

5. [IntSet](int_set.h)

- 使用encoding(how many bit for every elem), 带来底层数组实现在内存分配(every elem)的灵活&高效
- "升级", 底层数组添加"大"元素时, 需要对每个元素进行升级操作(从尾端移位操作)

6. [Zip List](zip_list.h)

- encoding使用, 使得`C Array`得以存储变长"元素"
- 细节优化, 根据总长确定特定字段(previous_len, content)应占空间, 详见头文件注释

7. [Quick List]()

- 使用zipList + linkList, 缓解linkList占用空间过多的问题

8. [Object](redis_obj.h)

- 使用type&encoding 解耦Redis data type 与 implement data struct.
- 根据不同使用场景, 使用不同的implement data type e.g:

a. string object: int, embstr, sds在不同场景下的使用;  
b. list object: zip list, linked list在不同场景下的使用(3.2以下, 3.2引入了quick list);  
d. set object: intset, hashtable在不同场景下的使用;  
e. sorted set: zip list, skip list在不同场景下的使用;  
c. hash object: zip list, hash table在不同场景下的使用;  

## 引用计数内存回收&对象共享&LRU(Last recent used)

引用计数: 因C并不具体自动的垃圾回收机制, Redis通过引用计数机制, 回收内存. 

对象创建时, 引用初始化为1; 有程序使用+1; 程序不再使用-1; 当计数为0时, 对象被释放

对象共享: 对于数值型字符串的对象, Redis共享对象, 增加引用计数(**2.9 version**); 对于非数字的字符串对象, 由于判断等复杂度O(n)得不偿失, 并不会共享.

LRU: redis_obj 对象有lru属于用于记录对象最后一次被命令程序访问的时间