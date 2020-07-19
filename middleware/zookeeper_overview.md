# ZooKeeper Introduction

## Overview

主要是官网文档和其它一些内容的总结...主要内容:

- [ZooKeeper Overview](#zookeeper-overview)
- [Getting Started Guide](#getting-started-guide)

more detail:

- [The ZooKeeper Data Model](#the-zookeeper-data-model)

### ZooKeeper Overview

ZooKeeper: A Distributed Coordination Service for Distributed Applications. ZK称自己是为分布式应用提供 distributed coordination services.

**Design Goals**

- ZooKeeper is simple.

ZK allow distributed process to coordinate with each other 通过一个共享的, 有层级namespace which is organized 简化为standard file system.

The namespace 由 data registers(在ZK中被称为 znodes)组成.

与典型的文件系统(被设计用来存储)不同, zk data is kept in-memory. 这意味着zk高吞吐, 低延时.

- ZooKeeper is replicated.

![zk_service_demonstration](https://zookeeper.apache.org/doc/current/images/zkservice.jpg)

> The servers that make up the ZooKeeper service must all know about each other. 
> They maintain an in-memory image of state, along with a **transaction logs and snapshots** in a persistent store.
> As long as a majority of the servers are available, the ZooKeeper service will be available.

组成zk服务的 servers 必须相互感知. 每个servers都在内存维护了image of state, 并且persistent store(持久化存储)上存有 transaction logs and snapshots.

> Clients connect to a single ZooKeeper server. The client maintains a TCP connection through which it sends requests, gets responses, gets watch events, and sends heart beats. 
> If the TCP connection to the server breaks, the client will connect to a different server.

客户端连接单个zk server, 通过TCP连接发送请求; 得到响应; get watch event; 发送心跳包. If TCP连接断开, 连接另一个zk server.

- ZooKeeper is ordered.

zk对每个更新都 stamps each update with a number that 反映着zk事务的顺序.

- ZooKeeper is fast.

对读为主的负载性能很好...(以10:1的读写比时, 性能最好)

**Data model and the hierarchical namespace**

![zk_data_model_namespace](https://zookeeper.apache.org/doc/current/images/zknamespace.jpg)

**Nodes and ephemeral nodes**

结点与临时结点

znodes 上可以存有数据, 前面说 zk namespace 类似于file system, 但并不完全类似. 可以理解为每个 `/app1/p_1` 结点都是"文件夹", 其内存有数据:)

> Znodes maintain a stat structure that includes version numbers for data changes, ACL changes, and timestamps, to allow cache validations and coordinated updates.
> Each time a znode's data changes, the version number increases. For instance, whenever a client retrieves data it also receives the version of the data.

znodes 实际上还维护 `stat structure` 信息, 保护数据版本, ACL(access control list), timestamps ... 不再翻译, 太过直白 :)

> The data stored at each znode in a namespace is read and written atomically...

存储在znodes中的数据, 读写均为原子操作.

> ZooKeeper also has the notion of ephemeral nodes. These znodes exists as long as the session that created the znode is active. When the session ends the znode is deleted.
  
zk同时还有临时结点, 在创建结点的session有效期内存活. session end, znode is deleted.

**Conditional updates and watches**

有条件的更新 和 watch 机制

> ZooKeeper supports the concept of watches. Clients can set a watch on a znode. 
> A watch will be triggered and removed when the znode changes. When a watch is triggered, the client receives a packet saying that the znode has changed. 
> If the connection between the client and one of the ZooKeeper servers is broken, the client will receive a local notification.
 
zk支持 watch 机制, 该机制允许client 监听一个znode, 当该 znode 发生变化时会触发 watch, 客户端会被通知znode has changed. 如果连接断开, client 也会收到本地的通知(断开的通知)

**Guarantees**

zk 被设计用来构建更复杂服务的基础设施. 它提供了一系列的"保证" guarantees:

- Sequential Consistency - Updates from a client will be applied in the order that they were sent. (顺序一致性)
- Atomicity - Updates either succeed or fail. No partial results. (原子操作)
- Single System Image - A client will see the same view of the service regardless of the server that it connects to. 无论client连接到哪个server都能看到始终如一的system state.
- Reliability - Once an update has been applied, it will persist from that time forward until a client overwrites the update. 更新持久性
- Timeliness - The clients view of the system is guaranteed to be up-to-date within a certain time bound. 客户端视角的 system 在一定时间内保证为最新.

**Simple API**

zk 的设计目标是指代简单的API...它提供如下操作:

- create
- delete
- exists
- get data
- set data
- get children
- sync: waits for data to be propagated

**Implementation**

> ZooKeeper Components shows the high-level components of the ZooKeeper service.
> With the exception of the request processor, each of the servers that make up the ZooKeeper service replicates its own copy of each of the components.

如图示, 除了request processor 每个zk services的组成组件都有replication.

![zk_implementation](https://zookeeper.apache.org/doc/current/images/zkcomponents.jpg)

- The replicated database 是内存数据库, 更新都被写入磁盘以备还原. 写请求先落盘, 再applied to the in-memory database.
- 每个zk server都能服务client. client 连接exactly one server to submit requests. 读请求由每个zk server local replication服务. Requests that change the state of the service, write requests are processed by an agreement protocol.

As part of the agreement protocol all write requests from clients are forwarded to a single server, called the leader.

The rest of the ZooKeeper servers, called followers, receive message proposals from the leader and agree upon message delivery.

The messaging layer takes care of replacing leaders on failures and syncing followers with leaders.

作为一致性协议的一部分, 写请求被发往leader(其它server称为follower); follower最终对修改达成一致(意会式翻译...). message layer处理leader下线后的重选举/再同步leader.

> ZooKeeper uses a custom atomic messaging protocol. Since the messaging layer is atomic, ZooKeeper can guarantee that the local replicas never diverge. 
> When the leader receives a write request, it calculates what the state of the system is when the write is to be applied and transforms this into a transaction that captures this new state.

zk server 的local replicas never diverge.

**Uses**

**Performance**

**Reliability**

zk the leader election algorithm 允许系统快速恢复(以防止吞吐量突然下降), less than 200ms to elect a new leader.

### Getting Started Guide

**pre-requisites; download; standalone operation; manage zookeeper storage; Connecting to ZooKeeper**

so ... easy to pass :)

### The ZooKeeper Data Model

TODO...
