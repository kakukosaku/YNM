# Distributed Lock

## Overview

Ref:

1. https://zookeeper.apache.org/doc/current/recipes.html#sc_recipes_Locks
2. https://redis.io/topics/distlock

关于"锁"按各种维度的分类此处不再介绍, 参考本系列文章 [lock](../../concurrent/lock/README.md)

关于"ZooKeeper"(后称zk)更基础的介绍此处也不再介绍, 参考本系列文章 [ZooKeeper](../../middleware/zookeeper_overview.md)

- [zookeeper implementation](#zookeeper-implementation)
- [redis implementation](#redis-implementation)

### zookeeper implementation

先说思路: 各server以某个"znode"为"锁", 成功创建该节点者视为"抢占"到锁, 拥有者删除znode视为"释放锁". 其它创建znode失败者利用`watcher`机制实现"等待锁" + "锁可用的唤醒".
 
**Simple Exclusive Lock Implementation**

前置条件: 以 `/app_1/open_account/sync_status_lock_1` (后称lock znode) 为锁资源, 每分布式应用都向zk申请创建该节点(抢占同一把锁). 具体流程:

1. 多个client竞争创建 lock znode (此处创建临时节点, 避免因client异常退出时未释放锁)

(抢占上述znode的server连接同一个[zk cluster](https://zookeeper.apache.org/doc/current/zookeeperAdmin.html#sc_zkMulitServerSetup), 称为zk's client)

2. 其中某个client create success, 未成功创建的client设置对该lock znode的watch.

3. 持有锁的客户端完成自己的操作释放锁, try catch finally(delete lock znode) 或client下线, 临时节点由zk删除.

4. 其它watch lock znode 的 client 得到通知, 再从1重复, 直至no client watch lock znode.

![non_fair_zk_lock_znode](https://blog-pictures.oss-cn-shanghai.aliyuncs.com/%e7%8b%ac%e5%8d%a0%e9%94%81%e6%b5%81%e7%a8%8b%e5%9b%be.png)

问题: 1. 非公平锁, 每个锁被释放后所有client再次抢占 2. 羊群效应 - 一个znode的`删除/创建`事件被通知所有client.

**zk Documentation Recommend Implementation**

> There now exists a Lock implementation in ZooKeeper recipes directory. This is distributed with the release -- zookeeper-recipes/zookeeper-recipes-lock directory of the release artifact.
  
实际上zk提供了一个分布式锁的实现, 但出于知其然, 知其所以然的精神, 有如下介绍...

以下实际为exclusive lock实现, 未翻译; shared lock 有总结式翻译.

Client wishing to obtain a lock do the following:

1. Call `create()` with a pathname of "locknode/guid-lock-" and sequence and ephemeral flags set. The guid(global unique id) is needed in case the `creat()` is missed.

https://zookeeper.apache.org/doc/current/recipes.html#sc_recipes_errorHandlingNote

> When implementing the recipes you must handle recoverable exceptions (see the FAQ).
> In particular, several of the recipes employ sequential ephemeral nodes.
> When creating a sequential ephemeral node there is an error case in which the create() succeeds on the server but the server crashes before returning the name of the node to the client.
> When the client reconnects its session is still valid and, thus, the node is not removed.
> The implication is that it is difficult for the client to know if its node was created or not. The recipes below include measures to handle this.

当创建 临时 顺序 znode时, server create znode success but server crashes before return the name of the node to client.

当clent reconnects its session is still valid, 之前创建成功的znode仍然存在. 这表明 it is difficult for the client to know if its node was created or not.

**If a recoverable error occurs calling `create()` the client should call `getChildren()` and check for a node containing the guid used in the path name.**

2. Call `getChildren()` on the locknode without set watch flag (this is important to avoid the herd effect, 羊群效应)

3. If the pathname created in step 1 has the lowest sequence number suffix, the client has the lock and the client exists the protocol.

4. else the client calls `exists()` with the watch flag set on the path in the lock directory with the next lowest sequence number.

5. if `exists()` return null, go the `step 2`. Otherwise, wait for a notification for the pathname from the previous step before going to step 2.

The unlock protocol is very simple: clients wishing to release a lock simply delete the node they created in step 1.

Here are a few things to notice:

- The removal of a node will only cause one clent to wake up since each node is watched by exactly one client. In this way, you avoid herd effect.
- There is no polling or timeouts.
- Because of the way you implement locking, it is ease to see the amount of lock contention, break locks, debug locking problems, etc.

总结成一句话: **关注刚创建znode 是否是队列中最先创建的, 如果不, 仅关注比自己小一号的, 避免羊群效应**

以上为原文, 太过直白 no need to translate. 下面关于共享锁的实现, 中文译之...

**Shared Locks**

You can implement shared locks by with a few changes to the lock protocol.

获取读锁流程:

1. 首先仍然是调用 `create()` 创建一个临时&顺序节点, 节点名 `guid-app-1/read-`. 拿到返回的真正创建的 znode name.
2. 再调用 `getChildren()` 不要设置 watch flag(Important to avoid herd effect).
3. 再判断, 如果没有节点为 `guid-app-1/write-` prefix **且** 序号低于上面创建read znode. client 拿到读锁, 并退出抢锁逻辑.
4. 否则(如果有write znode)调用 `exist()` with watch flag 在 `guid-app-1/write-` prefix 的znode (having next lowest sequence number).
5. 如果 `exist()` 返回 false, goto step 2.

6. 否则wait for a notification for the pathname from previous step before going to step 2.

额外注意! 需仔细理解:

1. 第3步很好理解, 对读锁, 写锁申请的客户端, 在同一znode下共享递增的`sequential ID`, 没有比读锁更早申请的写, 读锁申请成功!
2. 第4,5,6步实际是为了确定一件事: 再重复一次**有没有比此次读锁申请更早申请的写**, 即3步的退出条件.

如果3判断不成功, 也即有比此次读锁更早申请的写锁, 我们就观察**第二小的写锁**就好了, 如果直接返回不存在第2小的写锁(没有等待获取写锁的client, 以sequential ID来判断先后). 再回2,3步判断**有没有比此次读锁申请更早申请的写**

如果有写者(write client)等待, 关注该写者, 发生变化时, 再走2,3判断...

获取写锁流程:

1. 首先仍...仍然是调用 `create()` 创建一个临时&顺序节点, 节点名 `guid-app-1/write-`. 拿到返回的真正的znode name.
2. 再调用 `getChildren()` on lock node(`guid-app-1/`) without set watch flag(important to avoid herd effect, summary 中有对herd effect的解释)
3. 如果没有节点比刚创建的节点序号(同一parent node下共享sequential ID)更小, 则 client 获取写锁成功, 退出抢锁逻辑.
4. 否则以 watch 模式call `exist()`在 **次小序号**的znode上.
5. 如果返回false, 则goto step 2, 否则, 等通知, 然后goto step 2.

**Summary**

1. znode 的节点有无, 为锁被获取与否.
2. znode 顺序节点中序列号, 为锁尝试获取的顺序
3. 关注次小号避免羊群效应 && 实现公平锁.

4. ephemeral flag 是为防止client(进程, server)下线, 导致分布式锁"死锁". 而client拿到锁之后的进程内超时, 那是"传统, 单机锁"需要关注的事情.
5. no-sequential znode create 即为获取锁成功的方式, 实际是非公平锁(伴随着羊群效应)
6. herd effect: 是指实际只能有一只/少许几只羊 被释放, 但所有羊(一群)都被唤醒...

7. 总结下, 标准模式: 1. createNode -> 2. getChildren, no watch flag -> 3. judge lock condition -> 4. exist(on some znode) goto judge step.

### redis implementation

Redis 文档"声称"它为利用Redis实现分布式锁, 提示了""canonical algorithm" to implement distributed locks with Redis. 并称之为 `Redlcok`.

注意, 关于传统的 `setnx key ex time_to_live` [redis 文档](https://redis.io/commands/setnx) 特别强调了 is discouraged :) 

**Safety and Liveness guarantees**

以下内容, 总结式翻译吧...

redis redlock 算法以高效的方式提供3个properties(分布式锁所需要的最小保证):

1. Safety property. Mutual exclusion. 互斥操作, 任一给定时刻, 仅有一个client can hold a lock.
2. Liveness property A. Deadlock free. Eventually it is always possible to acquire a lock, even if the client that locked a resource crashes or gets partitioned.

liveness 可以理解为...活跃, 有生命力的意思吧, 意指 deadlock free...最终总能获取到锁, 即使原先锁的持有者crash 或get partitioned.

3. Liveness property B. Fault tolerance. As long as the majority of Redis nodes are up, clients are able to acquire and release locks.

**Why failover-based implementations are not enough**

这里分析, 传统的`setnx ex`方案有什么问题.

> The simplest way to use Redis to lock a resource is to create a key in an instance.
> The key is usually created with a limited time to live, using the Redis expires feature, so that eventually it will get released (property 2 in our list). 
> Then the client needs to release the resource, it deletes the key.
  
传统方案, 总结起来3个关键点.

1. 在一个实例上创建 lock key.
2. 以 created with a limited time to live 解决dead lock.
3. del lock key 视为锁释放.

表面上看起来工作的不错, 但有一个问题: 我们的架构里存在单点故障. 如果 redis master 宕机会发生什么? 我们可以加主从模式, 在master宕机里, 使用slave. 

This is unfortunately not viable. 但这样的方式并不可行. 由于redis replication is asynchronous, 我们无法保证互斥(在主节结点上写了lock key, 但未同步到slave上, slave上可再写lock key).

如果, 发现redis master 宕机时, 可能会有多个client持有lock key is ok, 那么基于replication base solution is fine. 否则, `redlock` 登场!

传统方案, 获取/释放锁:

`SET resource_name my_random_value NX PX 30000`

- NX: set the key only if it does not already exist
- PX: with an expire of 30000 milliseconds
- my_random_value: must be unique across all clients and all lock requests.

basically the random value is used in order to release the lock in a safe way, with a script that tells Redis:
remove the key only if it exists and the value stored at the key is exactly the one i expect to be.

Lua script:

```lua
if redis.call("get",KEYS[1]) == ARGV[1] then
    return redis.call("del",KEYS[1])
else
    return 0
end
```

这(LUA脚本的删除或类似的)很重要, 在以下场景: client A 获取了锁, 但处理超时, 锁过期; 被client B再次获取; 此时A处理完, 意欲del(释放)此时由B持有的锁, error.

**The Redlock algorithm**

在该算法的分布式版本中, 我们假设我们有N个Redis master. 这些节点完全独立, 我们也不使用replication or any other implicit coordination system. 

我们上面已经说过如果 acquire / release a lock in single instance. 再假设N为5, 5个Redis master运行在5个不同的虚拟机or something, 并且they'll fail in a mostly independent way.

为了获取锁, client perform the following perations:

1. 获取当前时间戳, 毫秒计
2. 顺序的从N个instances获取锁, 使用相同的 lock key name & random value.

需要注意的是, 客户端使用一个相较锁自动释放时间较小的"超时时间"(获取锁的超时时间)去获取锁. 这样可以避免client还继续与已经down掉的instance继续talk with.

If an instance is not available, we should try to talk with the next instance ASAP.

3. 客户端通过当前时间戳与 step1 中的时间戳差值来计算出获取锁所需要的时间.

If and only if the client was able to acquire the lock in the majority of the instances (at least 3), and the total time elapsed to acquire the lock is less than lock validity time, the lock is considered to be acquired.

只有当client获取多数instance锁 且 此时耗时比锁的有效期短时, 才认为成功获取到了锁.

4. 如果锁被成功获取到, 那么锁的"有效期"应该被认为是锁原先的有效期 - 获取锁耗时.

5. 如果client未能成功获取到锁, 由于不够N/2 + 1的多数instance 或新的有效期成了负数, it will try to nlock all the instances(even the instance it believed it was not able to lock).

需要强调的是, 基于single instance 的`setnx ex`操作获取锁/release lock with random check是OK的.

以下是关于如上算法特性, 有效性的一些解释:

- Is the algorithm asynchronous?

讲了些这么做的基本假设, 和时钟漂移(clock drift between processes). for more info, this paper:

[Leases: an efficient fault-tolerant mechanism for distributed file cache consistency](http://dl.acm.org/citation.cfm?id=74870)

- Retry on failure

Ideally the client should try to send the SET commands to the N instances at the same time using multiplexing. more info pass...

- Releasing the lock

释放锁很简单, just release the lock in all instances, 无论client是否能成功release 给定的instance's lock key.

- Safety arguments

关于是否安全的讨论...

- Liveness arguments

关于是否能提供 liveness A, B 特性的讨论...

- Performance, crash-recovery and fsync

关于性能, 崩溃恢复

- Making the algorithm more reliable: Extending the lock

关于该算法可靠性的补充...措施
