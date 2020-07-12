# Efficient Redis

本篇介绍了Redis的五种基本数据类型及其C数据结构表示, 并简要(非全部)总结其运行&资源(内存)利用的高效性.

## Overview

1. [基本数据类型](#基本数据类型)
2. [引用计数内存回收&对象共享&LRU(Last recent used)](#引用计数内存回收&对象共享&LRU(Last-recent-used))
3. [RDB&AOF](#RDB&AOF)
4. [SLAVEOF](#SLAVEOF)
5. [Sentinel](#Sentinel)
6. [Cluster](#Cluster)

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

a. string object: int, embstr(redis obj & sds 通过一次内存分配申请/释放), sds在不同场景下的使用;  
b. list object: zip list, linked list在不同场景下的使用(3.2以下, 3.2引入了quick list);  
d. set object: intset, hashtable在不同场景下的使用;  
e. sorted set: zip list, skip list在不同场景下的使用;  
c. hash object: zip list, hash table在不同场景下的使用;  

## 引用计数内存回收&对象共享&LRU(Least recently used)

引用计数: 因C并不具体自动的垃圾回收机制, Redis通过引用计数机制, 回收内存. 

对象创建时, 引用初始化为1; 有程序使用+1; 程序不再使用-1; 当计数为0时, 对象被释放

对象共享: 对于数值型字符串的对象, Redis共享对象, 增加引用计数(**2.9 version**); 对于非数字的字符串对象, 由于判断等复杂度O(n)得不偿失, 并不会共享.

LRU: redis_obj 对象有lru属于用于记录对象最后一次被命令程序访问的时间

## RDB&AOF

RDB: `SAVE`可对Redis DB当前的状态持久化(persistence), 由于有大量磁盘I/O, 操作会阻塞"单线程"的redis服务器, 使用`BGSAVE`可通过fork子进程的方式"后台"执行. 此外, 可以配置相应的RDB持久化策略, 自动执行. e.g:

```config
save 900 1  # 900s内对数据库进行了至少一次修改即触发BGSAVE(有一个条件满足即触发), 下类比
ave 300 10
save 60 1000
```

额外注意, SAVE时, 已有过期时间设置的过期键, 在从RDB中恢复时会主动过期之.

AOF(Append only File): 实质为对键的修改命令的记录(文本记录方式), 需要注意的是AOF实际过程有三: append, write, sync. 分别对应写AOF缓冲区, 写文件缓冲区, 写磁盘, 后2步相关内容参考OS File Manage(AOF记录写磁盘的时机, 也可通过配置`appendfsync: always/everysec/no`).

额外注意, 随着AOF的增长, 其中会多很多可以合并的日志项比如(`set keyA valueA; del keyA`), 通过rewriteaof 完成对AOF文件的重写, 此外重写并非分析AOF操作记录, 而是对当前DB状态的写入AOF中, 此为阻塞操作. 同样有 `BGREWRITEAOF`, 同样是fock子进程执行.

**关于`BGSAVE & BGREWRITEAOF` fork子进程方式的额外探讨**

`BGSAVE & BGREWRITEAOF` 均采用子进程而非线程的原因在于, 本身基于事件循环驱动的Redis, 为单线程模型, 增加新的线程, 需要考虑数据的竞态条件(增加锁的使用, 以保护数据安全). 而由于OS在子进程fork出来写时复制Copy on Write(COW)子进程模型, 避免了这一问题.

同时, 对于RDB而言, 保持为某一时刻的DB状态, COW的子进程是安全的; 对于AOF而言, 当fork出子进程执行rewrite aof时, Redis设置了重写缓冲区, 当服务器开始`BGREWRITEAOF`时, 开始使用该缓冲区记录键的修改命令(原缓冲区继续使用), 最后将新AOF文件改名, 覆盖原AOF文件.

## SLAVEOF

主从同步: 1. 异步同步, 但客户端可设置等待主从同步完成(指定确认数量); 2. 从节点还有可从节点; 3. 连接断开的自动重连, 检测上次同步位置(或重新同步)

需要注意的是, redis的从节点不具有key过期能力, key过期由主节点过期(发送DEL命令), 通知从节点; 由于惰性删除原因, 可能存在key实际过期, 但因未在master访问触发删除, 从节点返回了已过期的key. (更详细参见 https://redis.io/topics/replication - How Redis replication deals with expires on keys)

## Sentinel

sentinel模式的一些特点:

1. sentinel节点自动注册从服务器, 自动从主服务配置信息中获取;
2. 无需配置其它sentinel节点, 所有监测同一个主服务的节点可相互感知;
3. 通过配置sentinel节点配置`sentinel down-after-milliseconds master-name 10000`设置10S内未收到有效回复则sentinel节点主观认为master已下线; 通过配置sentinel节点配置`sentinel monitor master-name 127.0.0.1 6379 2(quote-num)`设置当2(quote-num)个sentinel节点主观认为master下线时, 客观判断master下线!
4. 不同sentinel节点的客观下线标准可能不能(quote-num)设置不同, 客观认为master下线的节点, 要求其它sentinel节点推举自己为局部领头的leader(每个节点"一次投票机会, 先到者先得票"), 当收到半数以上sentinel选票的成为领头sentinel开始执行master故障转移.
5. 从master服务器的从服务器slaver中挑选一个从服务器, 升为master, 将其它从服务改为同步新的master. 上旧的master重新上线时, 成为新的master的从服务器.
6. 选从也有一套规则, 更详线参照 https://redis.io/topics/sentinel
7. 为保证系统的鲁棒性, 常用sentinel配置模式详细介绍也参照 https://redis.io/topics/sentinel

## Cluster

> The use cases for Cluster evolve around either spreading out load (specifically writes) and surpassing single-instance memory capabilities. If you have 2T of data, do not want to write sharding code in your access code, but have a library which supports Cluster then you probably want Redis Cluster. If you have a high write volume to a wide range of keys and your client library supports Cluster, Cluster will also be a good fit.

Cluster集群用以拓展单机Redis的负载能力, 而Sentinel机制提供的是故障处理(automated failover)HA(high availability)方案. 粗浅的理解是这样, 更详细参照 https://redis.io/topics/cluster-tutorial