# Efficient Redis

本篇介绍了Redis的五种基本数据类型及其C数据结构表示, 并简要(非全部)总结其运行&资源(内存)利用的高效性.

## Overview

1. [Efficient Data Type](#efficient-data-type)
2. [Persistence: RDB&AOF](#persistence-rdbaof)
3. [Replication](#replication)
4. [Sentinel](#Sentinel)
5. [Cluster](#Cluster)
6. [Transaction](#Transaction)

## Efficient Data Type

0. [Redis Object](redis_obj.h)

- 使用type&encoding 解耦Redis data type 与 implement data struct.
- 根据不同使用场景, 使用不同的implement data type e.g:

a. string object: int, embstr(redis obj & sds 通过一次内存分配申请/释放), sds在不同场景下的使用;  
b. list object: zip list, linked list在不同场景下的使用(3.2以下, 3.2引入了quick list);  
d. set object: intset, hashtable在不同场景下的使用;  
e. sorted set: zip list, skip list在不同场景下的使用;  
c. hash object: zip list, hash table在不同场景下的使用;  

1. [Simple Dynamic String](simple_dynamic_string.h)

- 额外属性(len, free)记录当前总长及剩余空间
- 在上述属性帮助下, 实现二进制安全(\0, 特殊字节) & C 原生string相关函数复用
- 数组预分配&懒回收
- 由于Redis data type 与 implement data struct 解耦的好处, 带来在处理不同"类型"的string的使用不同数据结构, 共三种底层数据结构

int: 针对number类型的string, 显然节省空间&带来了方便一套操作; embstr: 较小string, 一次内存操作申请/销毁 redis_object+string_data_struct_obj; row char array)

2. [double Link List](double_link_list.h)

- 头尾节点(head, tail), 前驱后劲(prev, next), 额外属性(len)包含总体信息.

3. [Hash Table Dict](hash_table_dict.h)

- 额外属性(table_size, used)包含总体信息.
- 拉链法解决hash collision & 渐进式rehash
- hash_table的存储元素指针
- BGSAVE 时子进程 COW存在, 提高rehash factor, 避免不必要的内存复制.

4. [Skip List](skip_list.h)

- 跨度信息(span)等的应用

5. [IntSet](int_set.h)

- 使用encoding(how many bit for every elem), 带来底层数组实现在内存分配(every elem)的灵活&高效
- "升级", 底层数组添加"大"元素时, 需要对每个元素进行升级操作(从尾端移位操作)

6. [Zip List](zip_list.h)

- encoding使用, 使得`C Array`得以存储变长"元素"
- 细节优化, 根据总长确定特定字段(previous_len, content)应占空间, 详见头文件注释

7. [Quick List](quick_list.h)

- 使用zipList + linkList, 缓解linkList占用空间过多的问题

8. Reference Count & LRU(Least recently used)

引用计数: 因C并不具体自动的垃圾回收机制, Redis通过引用计数机制, 回收内存. 

对象创建时, 引用初始化为1; 有程序使用+1; 程序不再使用-1; 当计数为0时, 对象被释放

对象共享: 对于数值型字符串的对象, Redis共享对象, 增加引用计数(**2.9 version**); 对于非数字的字符串对象, 由于判断等复杂度O(n)得不偿失, 
并不会共享.

LRU: redis_obj 对象有lru属于用于记录对象最后一次被命令程序访问的时间

## Persistence: RDB&AOF

RDB: `SAVE`可对Redis DB当前的状态持久化(persistence), 由于有大量磁盘I/O, 操作会阻塞"单线程"的redis服务器, 使用`BGSAVE`可通过fork
子进程的方式"后台"执行. 此外, 可以配置相应的RDB持久化策略, 自动执行. e.g:

```config
save 900 1  # 900s内对数据库进行了至少一次修改即触发BGSAVE(有一个条件满足即触发), 下类比
save 300 10
save 60 1000
```

额外注意, SAVE时, 已有过期时间设置的过期键, 在从RDB中恢复时会主动过期之.

AOF(Append only File): 实质为对键的修改命令的记录(文本记录方式), 需要注意的是AOF实际过程有三: append, write, fsync. 分别对应写AOF
缓冲区, 写文件缓冲区(flush到操作系统缓冲区), 写磁盘, 后2步相关内容参考OS File Manage(AOF记录写磁盘的时机, 也可通过配置`appendfsync: always/everysec/no`).

额外注意, 随着AOF的增长, 其中会多很多可以合并的日志项比如(`set keyA valueA; del keyA`), 通过rewriteaof 完成对AOF文件的重写, 此外重写
并非分析AOF操作记录, 而是对当前DB状态的写入AOF中, 此为阻塞操作. 同样有 `BGREWRITEAOF`, 同样是fock子进程执行.

**关于`BGSAVE & BGREWRITEAOF` fork子进程方式的额外探讨**

`BGSAVE & BGREWRITEAOF` 均采用子进程而非线程的原因在于, 本身基于事件循环驱动的Redis, 为单线程模型, 增加新的线程, 需要考虑数据的竞态条件
(增加锁的使用, 以保护数据安全). 而由于OS在子进程fork出来写时复制Copy on Write(COW)子进程模型, 避免了这一问题.

同时, 对于RDB而言, 保持为某一时刻的DB状态, COW的子进程是安全的; 对于AOF而言, 当fork出子进程执行rewrite aof时, Redis设置了重写缓冲区, 
当服务器开始`BGREWRITEAOF`时, 开始使用该缓冲区记录键的修改命令(原缓冲区继续使用), 最后将新AOF文件改名, 覆盖原AOF文件.

## Replication

Ref: https://redis.io/topics/replication

Replication system works using three main mechanisms:

1. 当连接正常时, master 通过发送 a stream of commands to the replica.

in order to replicate the effects on the dataset happening in the master side due to: client writes, keys expired or 
evicted, any other action changing the master dataset.

2. 当连接断开时, replica will automatically reconnect to the master and attempt to proceed with a partial resynchronization:
try to just obtain 连接断开期间的stream of command.

3. 当部分同步 is not possible, 从节点will ask for a full resynchronization. master创建snapshot发给replica, 然后继续发送stream of command.

默认执行异步复制, master 不等待每个command 被replica执行结果, replica 周期性ACK the amount of data they received.

通过 `WAIT num_of_ack_replicas timeout` 命令达到触发Redis server "同步复制"

The following are some very important facts about Redis replication:

- master 可以有多个 replicas; replicas can have replicas too; init sync & partial sync are non-block in both master and 
replicas(load init sync is block in replicas side).
- replication 机制可用于"拓容" or simple for improving data safety and HA.
- 也可以通过在relicas上实施 RDB&AOF 策略, 避免master writing the full dataset to disk. 但这很危险, master重启时is empty and can
sync empty to replicas.

其它值得一提的细节:

- each instance have two replication IDs;

replication ID可以被认为是dataset的标识, replicas通过该ID与master 通过offset作部分同步;

当master down, The promoted replica 需要: 1. 记住原先的master replication ID, 这样其它从节点不用再作全同步; 2. 提供新的replication
ID作新的dataset的标识.

- Diskless 的复制模式 fork sub-process do full sync without touch disk.

- A -> B -> C, B如果是可写从节点, B的写不会被同步到C!

- masterauth <master_passwd> or config set masterauth <master_passwd>

- How Redis replication deals with expires on keys?

Redis replicas 能正确的复制key with expires! 通过同步时钟是不可行的, 这会导致race condition and dataset diverge. Redis通过3个机制
使得replicas的过期key able to work!

1. Replicas 不过期key, 而是依赖于master to expire the keys.
2. 但由于过期key算法(非全量实时过期), master中会存在实际已过期, 但未删除的, 当然也未同步replicas DEL 命令. 所以replica 使用其logical
lock "仅在读操作"时, 以报告key过期
3. During lua scripts executions no key expires are performed! ...more explain pass.

## Sentinel

Ref: https://redis.io/topics/sentinel

Sentinel 机制可以提供的功能:

1. Monitoring. Sentinel 定期check master & replicas 是否工作正常.
2. Notification. Sentinel 能通过API通知administrator or other app 其monitored Redis instance异常状态.
3. Automatic failover. Sentinel 当master故障时可以自动开启failover process(故障切换), a replica 被升级为master, 其余replica被
配置为从新master同步. 并且apps are informed the new address to use when connecting.
4. Configuration Provider. Sentinel 实际充当了服务发现的功能: client 连接sentinels, 询问当前master redis instance's address;
并且在failover发生时, 通知client新master address.

sentinel模式的一些特点:

Redis Sentinel is a distributed system: Sentinel 模式设计就是多个 sentinel 共同协作. 使用多个Sentinel进程协作的优点有:

1. Failure detection is performed when sentinel agree about the fact a given master is no longer available.
2. Sentinel works even if not all the Sentinel processes are working, making the system robust against failures.

如果 failover system 本身存在单点故障, 其的存在就失去了意义...

2020/07/25 当前 sentinel 被称为 sentinel 2. 初次稳定版的 sentinel 于redis 2.8 release.

permannet split brain condition: 脑裂

仅设置2个sentinel也不可取, 考虑如下情形:

```markdown
+----+         +----+
| M1 |---------| R1 |
| S1 |         | S2 |
+----+         +----+

Configuration: quorum = 1
```

如果 M1 fails, R1 可以自动提升为master, 由于故障quorum为1, 且2个sentinel都能检测到错误, 可以达到多数授权, 开启failover.

但如果 M1, S1的机器挂掉, 此时另一个机器S2是拿不到多数授权以开始 failover, 所以此时系统是不可用的.

**需要强调的是, 获取大多数sentinel授权(be authorized)开启 "failover" 故障转移流程是必要的!**

试想网络发生分区, M1,S1 所在机器与 M2,S2所在机器不可达. 如果没有多数授权S2 也能开始failover -> 将R1提升为master, 将配置更新广播出去
"propagate"(只有S2自己, S1不知道R1已升级为master).
 
连接S1, S2的client, 在网络分区发生后通过S1,S2连向了不同的master! 网络分区恢复后, 无法区分哪个master的配置是正确的!

(no way to understand when the partition heals what configuration is the right one, in order to prevent a **permanent**
split brain condition.)

```
+----+           +------+
| M1 |----//-----| [M1] |
| S1 |           | S2   |
+----+           +------+
```

so Please**deploy as least three sentinels in three different boxes** always.

一主, 二从, 三哨兵的设置:

```
       +----+
       | M1 |
       | S1 |
       +----+
          |
+----+    |    +----+
| R2 |----+----| R3 |
| S2 |         | S3 |
+----+         +----+

Configuration: quorum = 2
```

> In every Sentinel setup, as Redis uses asynchronous replication, there is always the risk of losing some writes because
>a given acknowledged write may not be able to reach the replica which is promoted to master.

需要明白的是, 无论哪种 sentinel 配置, 总会有丢失数据的风险. 由于 1. replicas 基于异常复制. 2. 网络分区发生时, 发生在分区内成功的写入. 如下图所示:

下图所示 data lost 可以replica feature 配置拒绝写入缓解...

```
min-replicas-to-write 1  # 1个online replic
min-replicas-max-lag 10  # 最多10s未收到 ping(replic 1s/次)

# 同时, 需要注意到如果2个replicas 有不用, 则master直接不可写, it's a trade off!
```

```
         +----+
         | M1 |
         | S1 | <- C1 (writes will be lost)
         +----+
            |
            /
            /
+------+    |    +----+
| [M2] |----+----| R3 |
| S2   |         | S3 |
+------+         +----+
```

Example 3: Sentinel in the client boxes.

一些场景, 可能只有 1M, 1R, 这时, 我们可将sentinel与client同机部署(需要保证有多于3个client boxes, 原因如下所述), 图示:

```
            +----+         +----+
            | M1 |----+----| R1 |
            |    |    |    |    |
            +----+    |    +----+
                      |
         +------------+------------+
         |            |            |
         |            |            |
      +----+        +----+      +----+
      | C1 |        | C2 |      | C3 |
      | S1 |        | S2 |      | S3 |
      +----+        +----+      +----+

      Configuration: quorum = 2
```

Example 4: Sentinel client side with less than three clients.

example 3的补充. 如图:

```
            +----+         +----+
            | M1 |----+----| R1 |
            | S1 |    |    | S2 |
            +----+    |    +----+
                      |
               +------+-----+
               |            |
               |            |
            +----+        +----+
            | C1 |        | C2 |
            | S3 |        | S4 |
            +----+        +----+

      Configuration: quorum = 3
```

1. sentinel节点自动注册从服务器, 自动从主服务配置信息中获取;
2. 无需配置其它sentinel节点, 所有监测同一个主服务的节点可相互感知;
3. 通过配置sentinel节点配置`sentinel down-after-milliseconds master-name 10000`设置10S内未收到有效回复则sentinel节点主观认为master
已下线; 通过配置sentinel节点配置`sentinel monitor master-name 127.0.0.1 6379 2(quote-num)`设置当2(quote-num)个sentinel节点主观
认为master下线时, 客观判断master下线!
4. 不同sentinel节点的客观下线标准可能不能(quote-num)设置不同, 客观认为master下线的节点, 要求其它sentinel节点推举自己为局部领头的leader
(每个节点"一次投票机会, 先到者先得票"), 当收到半数以上sentinel选票的成为领头sentinel开始执行master故障转移.
5. 从master服务器的从服务器slaver中挑选一个从服务器, 升为master, 将其它从服务改为同步新的master. 上旧的master重新上线时, 成为新的master
的从服务器.
6. 选从也有一套规则, 更详线参照 https://redis.io/topics/sentinel
7. 为保证系统的鲁棒性, 常用sentinel配置模式详细介绍也参照 https://redis.io/topics/sentinel

High level 总结:

sentinel 解决2类问题(1.redis master instance die; 2. network partition occur.) 做自动的故障发现, 故障转移.

同时为保证不发生永久brain split(脑裂). 必须有多数sentinel获取投票, 才发起failover, 执行slave升级master操作. 为此, 必须至少有3个
sentinel节点.

关于 sentinel 更多细节参考[Redis文档](https://redis.io/topics/sentinel#more-advanced-concepts), 不再赘述.

## Cluster

> The use cases for Cluster evolve around either spreading out load (specifically writes) and surpassing single-instance
> memory capabilities. If you have 2T of data, do not want to write sharding code in your access code, but have a library
> which supports Cluster then you probably want Redis Cluster. If you have a high write volume to a wide range of keys
> and your client library supports Cluster, Cluster will also be a good fit.

Redis Cluster 提供以下"能力"

1. The ability to **automatically split your dataset among multiple nodes**.
2. The ability to **continue operations when a subset of the nodes are experiencing failures** or are unable to communicate
with the rest of the cluster.

**Redis Cluster master-slave model**

In our example cluster with nodes A,B,C, if node B fails the cluster is not able to continue. 

However, when the cluster is created (or at a later time) we add a slave node to every master,
so that the final cluster is composed of A, B, C that are masters nodes, and A1, B1, C1 that are slaves nodes,
the system is able to continue if node B fails.

**Redis Cluster consistency guarantees**

Redis cluster 并不提供强一致性保证: 是说, 即使已经被server ack 的写入, 由于replica异步复制, 在server instance 发生failover时,
已ack的但未同步到replica的写入仍会丢失!

more info pass...

## Transaction

Ref: https://redis.io/topics/transactions

Redis 通过`MULTI, EXEC, DISCARD`提供了"有限的事务体验", 并通过`WATCH, UNWATCH`提供了乐观锁.

They allow the the execution of a group of commands in a single step, with two important guarantees:

1. 所有命令以序列化的方式去执行, 且执行过程中, 不会执行来自其它client的命令.
2. 要么全部执行, 要么都不执行(指所有 1.正常进Queue, 2.正确的命令 关于2的额外解释见后). 所以Redis的事务命令可以认为是原子的(Atomic).
若执行过程中redis宕机, 通过AOF日志(AOF开启时, 先落disk, 再执行)

PS: `MULTI`后redis将之后的命令存入Queue中, 在`EXEC`后再执行命令.

2种可能的错误:

1. 入Queue时, 失败(即失败发生在`EXEC`之前): 这种失败是Syntactically Wrong(语法性的错误, 如错误的命令, 错误的参数数量). 或一些极端情况如内存不足.
2. 在`EXEC`之后, 失败: 这种失败是在key上执行不支持的命令(如在list类型的key上执行string类型的命令).

对于第1种错误, client是有感知的(server是否返回QUEUED), 一般client感知到这种错误时, 会直接abort掉事务的继续(`DISCARD`命令). 在2.6.5之后,
在进QUEUE累积command时的错误, server也会记录, 避免EXEC的执行.

**而对于第2种错误(`EXEC`之后)命令, redis会执行全部命令, 即使其中一些执行失败.** 有如下示例, 仅部分命令执行成功:

```text
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
MULTI
+OK
SET a abc
+QUEUED
LPOP a
+QUEUED
EXEC
*2
+OK
-ERR Operation against a key holding the wrong kind of value
```

这里, 再额外强调一下: 即使已入QUEUE的命令有执行失败, redis仍会继续执行剩下的命令!

对1, 也有个示例: 

```text
MULTI
+OK
INCR a b c
-ERR wrong number of arguments for 'incr' command
```

Syntactically error, 根本不会入QUEUE!

对于, redis 为何不支持rollback, 其[文档](https://redis.io/topics/transactions#why-redis-does-not-support-roll-backs)上也有说明.
总结一下: 经过分析, 会产生的错误, 是开发时错误; 保持redis的简单&快.

再简单介绍下`WATCH`的应用吧:

1. `WATCH` is used to provide a check-and-set (CAS) behavior to Redis transactions.
2. 被`WATCH`的key一旦发生修改, `EXEC`就会取消执行. (额外的: 1. 在事务的修改, 不会导致该事务abort; 2. key过期也不会导致事务abort)

另外: redis的script都以事务的模式执行~
