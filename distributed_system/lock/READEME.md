# Distributed Lock

## Overview

Ref:

1. https://zookeeper.apache.org/doc/current/recipes.html#sc_recipes_Locks
2. https://github.com/code4wt/distributed_lock

关于"锁"按各种维度的分类此处不再介绍, 参考本系列文章 [lock](../../concurrent/lock/README.md)

关于"ZooKeeper"(后称zk)更基础的介绍此处也不再介绍, 参考本系列文章 [ZooKeeper](../../middleware/zookeeper_overview.md)

- [zookeeper implementation](#zookeeper-implementation)

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
- 
