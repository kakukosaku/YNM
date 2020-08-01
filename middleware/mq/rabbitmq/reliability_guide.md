# Reliability Guide

网络, Client, Rabbit Server, 甚至于代码逻辑导致的问题, 都可能导致各种各样的 failure. 

## Overview

Ref: https://www.rabbitmq.com/reliability.html

RabbitMQ 为了保证服务的稳定性, 作了很多方面的努力, 这些也一个消息中件基本都会涉及的话题:

- What can Fail?
- Connection Failures
- Acknowledgements and Confirms
- Detecting Dead TCP Connection with Heartbeats
- Data Safety on the Broker Side
- Clustering and Message Replication
- Data Safety on the Publisher Side
- Ensuring that Messages are Routed
- Unprocessable Deliveries
- Consumer Cancel Notification
- Monitoring and Health Checks

1. [Acknowledgements and Confirms](#ack-and-confirm)
2. [Clustering](#clustering-guide)
3. [Queue Mirroring](#classic-mirrored-queues)
4. Publishers and Consumers
5. Others: Alarms, Monitoring, Metrics and Health Check

### Ack and Confirm

Ref: https://www.rabbitmq.com/confirms.html

该部分包含以下话题:

- 为什么需要ACK模式
- 手动ACK, 自动ACK模式
- ACK, NACK, REJECT and Requeue
- 自动的Requeue
- Channel Prefetch(Consumer流量控制), 对吞吐量的影响
- 常见的Client error
- Publisher Confirms 及其数据安全的话题

**为什么需要应用层(RabbitMQ)需要ACK?**

当Client and RabbitMQ node之间的连接故障发生时: msg可能正在"路上"; 或者正在2方的TCP stack buffers正等待装包/解包中. 此时这种状态的消息是没有投递成功的!

如何检测这样的故障? 并且重发呢? 受TCP 基于ACK的重传机制(启发), RabbitMQ也使用基于ACK的消息确认, 只是上述故障可能发生在 Publisher(P) --> RabbitMQ(MQ) --> Consumer(C) 侧, 分别给了不同的名字:

Publish confirm and Consumer ACK!

P跟MQ之间, MQ跟C之间都是建立TCP连接通信, 我们知道TCP安全可靠传输, 保证TCP包的有序且必达. 那为什么还需要Publish confirm and Consumer ACK呢?
需要明确的是, TCP 的ACK仅处理网络层的故障(only handle failure at the network layer). 它仅保证TCP包的到达目的主机, 由OS的TCP实现接收响应ACK(TCP层)为止. 但是否以达应用程序内存?
是否被业务逻辑正确处理, 都是没有保证的, 这期间的故障, 也是不可控的! 这是2个维度的事情!

同时, 需要注意到的是: 如果P在发送后, MQ响应的ACK由于某些原因(网络, 硬件故障, P down掉等), P没有收到该ACK, P的重传会事实上导致消息重复投递, deliver! 所以消费方, 需要做到消息消费的幂等, idempotent.

**Delivery Identifiers: Delivery Tags**

首先, 我们需要明白的是: 如何标识所要ACK(动词, 确认已达)的消息.

When a Consumer is registered, messages will be delivered(pushed) by RabbitMQ using the `basic.deliver` method. 这个方法携带一个 `delivery tag` 用以标识channel中被投递的消息. **因此, delivery tag scoped per channel**

通过 `delivery tag` 以ACK(确认)消息的投递结果. 非本Channel的`delivery tag`会导致 `unknow delivery tag` protocol exception and close the channle.

**Consumer Ack Modes and Data Safety Considerations**

当Node deliver a message to a consumer, 它必须要考虑消息是否已被处理(或者, 至少已达). Message Protocol提供了这样的机制: ACK 允许Consumer向Node确认消息投递结果.

但这基于Consumer是否选择启用...

对于RabbitMQ来讲, 它有2种投递方式: 1. 当消息发出(written to a TCP socket)即结束投递; 2. 当显示("manual")的收到来自Client的ACK才结束投递.

而对于Manual ACK有如下几种分类:

- `basic.ack`: is used for positive acknowledgements, you can ack multiple Deliveries at once!
- `basic.nack`: is used for negative ack, (negative ack 可以向Node指明是 discard or requeue消息)
- `basic.reject`: 与`basci.nack`功能一致, 实际上reject才是 AMQP协议中规定的, 但它不支持批量reject, 所以RabbitMQ提供了nack的拓展.

另外, 需要介绍的是, 当reject or nack并且requeue设置为true时, 消息会被置于在队列中原先的位置(只要有可能, 当消息被消费而导致事实上队列顺序发生变化时, 会置于最靠近队列的一侧).
同时, 需要强调的是, 基于上述策略, 有可能导致被requeue的消息陷入requeue/redelivery loop. 这会很消费network bandwidth and CPU resource.
所以, 基于discard 的 reject/nack或者 requeue after a delay会实际更有用!

先说站在RabbitMQ立场的 auto ack(这么讲, 是因为部分Client的实现如Java, 把应用程序收到消息自动回复ACK定义为auto ack, 程序员手动调用ack称为manual ack):

1. 消息理论上可能丢的, 但显然对于提升吞吐量是有帮助的, 这是个性能与安全的 trade off.
2. 消费可能面临的 overwhelm (被消息淹没). 而manual ack(同样是站在RabbitMQ立场的manual, 上已说明...)时, 通常通过`prefetch`来控制消息投递速度(或者叫投递带宽?)

而过多的消息, 会大量占用内存缓冲, 可能会OOM killed by OS. 一般仅应用在消费者消费速度很快, 且消息丢失可授受的场景(事实上很少用)

**Channel Prefetch Setting(QoS)**

借鉴TCP滑动窗口, 限制未ACK的消息数量, 避免overwhelm. `basic.qos`: 定义了max number of unacknowledged deliveries that permitted on the channel.

同时, 也可以开启batch ack以节省network bandwidth. 参考`TCP累计确认模式`

**Per-channel, Per-consumer and Global Prefetch**

> The QoS setting can be configured for a specific channel or specific consumer.

QoS的unk ack消息数量的限制可以是channel级的, 也可以是consumer级的.

**Prefetch and Polling Consumers**

The QoS prefetch setting has no effect no message fetched by using the `basic.get` (pull API), even in manual confirmation.

**Consumer Acknowledgement Modes, Prefetch and Throughput**

开启ACK模式以及prefetch的设置都会很大影响consumer的吞吐量, throughput. 无论是使用auto ACK(站在MQ立场上的, 实质指no ack), 还是增大prefetch的大小都无可避免的增大的Consumer的RAM使用.(大量的未ACK消息也会占用Node大量的内存)

所以: on ack 或者 有ack但不限制prefetch的做法, 都必须非常小心!

**When Consumers Fail or Lose Connection: Automatic Requeue**

在ACK开启的模式下, 任务channel or connection is closed 没有被ACK的消息都自动"requeue"! 主要包括以下三种情形:

1. TCP connection loss by clients.
2. Consumer Application(process) failures(dead or something else).
3. Channel-level protocol exceptions(covered below).

其中关于1的TCP connection loss, 我们知道OS(以Linux为例, 默认为11min)对于TCP的连接不可用有较长的判定时间.
由于此, Node与Client之间维护心跳包, `HeartBeat`以阻塞TCP连接由于IDLE被terminate. 同时`heartbeat timeout`value是一个可配置的值:
该值由Client与Node协商得出(如果有一方为0, 取2者大值; 否则取2者小值), 一般来说Client都必须要配置此值!

HeartBeat Frames 每 `heartbeat timeout / 2` seconds. (This value sometimes referred to as the `heartbeat internal`).
在Missed 2个heartbeats, the peer is considered to be unrechable. 

这里解释下peer的意义, 这个单词经常出现在网络应用/协议中, 它指2方或多方中, 对等的那一方, 比如A-B连接, A,B都可以称对方为peer, 著名的TCP连接的`reset by peer`.

这里需要额外指出的是: 基于消费了, 但未实际ACK的requeue; 以及前面提到的实际send, 但未ACK的Publisher的resend. 消费者需要: **准备好处理redelivery msg 或者 做到幂等消费 msg**

当消息是被node因未ACK而重复delivery时, 其property, `redeliver` set to `true` by RabbitMQ.

**Client Errors: Double Acking and Unknow Tags**

前面说过 delivery tag 的事情, ACK(消息确认, 名词...)实际是在ACK(确认, 动词...) delivery tag. 如果重复确认一个tag, 或确认一下unk tag(delivery tag is channel scope), 都会引起channel raise exception.

**Publisher Confirms**

如前所述, 网络的故障检出, 是需要时间的. 所以: A client written a protocol frame or a set of frames(e.g. a published message) to its scoket **connot assume** that the message has reached the server and was successfully processed

类似于前端Consumer ACK中提到的机制, Publisher也使用相同的机制, 称为Publisher confirm(以下简称 confirm). (或者使用transactional channel相较ACK mode会有更糟糕的性能问题, 这里就是再详细介绍)

开启confirm, 需要客户端发送`confirm.select`method(站在row protocol的角度描述, 不同语言的client可能有不同的API or concept) blalala...就开启了. 

在开启confirm模式的channel中, both broker(本文中, 共出现RabbitMQ server, Node, Broker三种表述, 均指同一物. 文档即如此) and the client count messages(从一开始的`confirm.select`为1开始计起)
在`delivery-tag`字段中包含 the sequence number of the confirmed message. Broker也可使用累计确认. 一次确认多个.

**Negative Acknowledgement for Publishers**

Broker也可能会NACK消息, 这种情况一般是Broker无法处理消息并拒绝对消息做回应...这时publisher可以选择re-publish the messages. 

Broker可以保证一个消息最多被 ACK 或 NACK 一次, 但多久, 并不保证 :(

**When Will Published Messages Be Confirmed by the Broker?**

- 对于无法路由的消息, Broker在exchange一旦确认消息无法route to any queue就会发出ACK. 当消息被标记为必须routed时, 还会先发出 `basic.return`, 然后再 `basic.ack`. 对于NACK遵循一样的规则.
- 对于可以路由的消息, Broker在所有queue都接收了消息后发出ACK; 对于persistent messages routed to durable queues, 是在persisting to disk(持久到磁盘后再ACK); 对于Mirrored queue, 在所有mirrors 接受了消息后ACK.

**Ack Latency for Persistent Messages**

对于存储在持久化的Queue中的持久化的消息, ACK是在"落盘"后才发出的. The RabbitMQ 写磁盘是以batch的形式进行, a few hundred milliseconds 以减少`fsync(2)`的系统调用(或在queue is idle时进行).

这意味着, 在constant load下ACK可能会有数百毫秒的延时. 为了改善throughput, App(Client)非常建议以异常的方式处理ACK消息(as a stream) 或者以batch的方式发出消息, 然后等待ACK. 不同客户的AP不同.

**Ordering Considerations for Publisher Confirms**

大多数情况下, RabbitMQ以Publisher发出的顺序ACK消息, 但实际上ACK消息的发出为异步模式, 也有可能是指ACK. 具体的ACK发出时刻也不尽相同, 如前提到(persistent queue, mirrored queue, 以及msg是否有route)

这意味着ACK的数据有可能与Publisher的发出顺序不同!

**Publisher Confirms and Guaranteed Delivery**

RabbitMQ Node实际可能会丢失persistent messages在未写磁盘前的fails, 例如: 考虑如下场景:

1. A client publishes a persistent message to a durable queue.
2. A client consumes the message from the queue, 但并未开启ACK模式.
3. The broker node fails and is restarted, and
4. the client reconnects and starts consuming messages.

总结一句话, 未写盘前的持久消息, 被未开启ACK的消费者申请消费, 但Broker在发送完成时restart! Client reconnect Broker 却浑然不知!

如何避免? 使用ACK模式的Publisher, 因为在消息未持久化到磁盘时, ACK是不给到Publisher的.

**Limitation**

Delivery tag is a 64bit long value, and thus its maximum value is `922 3372 0368 5477 5807`(900多亿亿, 相当大的数!). 但由于delivery tag is channel scope. 几乎不可能publisher or consumer 达到这个限制.

### Clustering Guide

该部分主要的话题:

- RabbitMQ 如何区分不同的节点(node): node name
- Requirements for clustering
- 什么数据在Nodes间不复制
- What clustering means for client
- How cluster are formed
- Nodes间如何认证
- 为什么使用奇数的Nodes很重要以及为什么2个节点非常不推荐
- Nodes重启后, 如何加入集群
- Node readliness probes and how they can affect rolling cluster restarts
- 如何移除集群节点
- 如何重置节点状态

More info ref to [Cluster Formation And Peer Discovery](https://www.rabbitmq.com/cluster-formation.html) 这篇文章专注于节点发现及集群formation automation-related topics.

RabbitMQ cluster 是一个或多个节点的逻辑分组, 它们共享 users, virtual hosts, queues, exchanges, bindings, runtime parameters and other distributed state.

**组建集群**

A RabbitMQ cluster can formed in a number of ways. 包括直接在配置文件上写明nodes list; 以及通过其它分布式组件发现节点等方式. 这里不赘述, 参见 [Cluster Formation Guide](https://www.rabbitmq.com/cluster-formation.html)

需要注意的是, 一个群集都是从一个节点开始, 其它节点逐渐加入形成集群. 反之也可.

**Node Names(Identifiers)**

RabbitMQ节点名称由2部分组成, prefix@hostname, e.g: `rabbit@node1.messaging.svc.local`. 集群中的node name必须唯一.

同时, 集群中Node通过hostname contact each other, 这意味着hostname必须可解析. 启动时未通过 `RABBITMQ_NODENAME` 环境变量指定, 自动生成`rabbit@computer_hostname`.

其次, 如果hostname使用 qualified domain name (这称之为long node name), 必须设置`RABBITMQ_USE_LONGNAME` to true.

**Cluster Formation Requirements**

1. Hostname Resolution: 如前介绍
2. Port Access:

- 4369: epmd, a helper discovery daemon used by RabbitMQ nodes and CLI tools.
- 5672,5671: used by AMQP 0-9-1 and 1.0 clients without and with TLS
- 25672: used for inter-node and CLI tools communication与Redis sentinel默认规则类似, 6379+20000 :)

... 一堆端口, 不赘述

**Nodes in a Cluster**

What is replicated?

所有RabbitMQ Broker运行需要的数据/状态is replicated across all nodes. 除了message queues, 它们默认仅留存在一个节点内, 但它们对整个集群节点都是可见, 可达的.

如果想要在节点间也复制queues, 需要特定类型的queues, 如[Quorum Queues](https://www.rabbitmq.com/quorum-queues.html) RabbitMQ 3.8.0后可用; [Classic Mirrored Queues](https://www.rabbitmq.com/ha.html) 文档称前者为下一代高可用Queues, 可推荐前者.

Nodes are Equal Peers

我愿称之为节点间地位相当...一些分布式系统有leader and follower nodes之分. RabbitMQ中节点均为地位对等的节点, 这在 [queue mirroring](https://www.rabbitmq.com/ha.html) and plugins 被考虑进来时才会有细微的不同, 这里不做过多延伸.

How CLI Tools Authenticate to Nodes

所有集群使用同样的cookie(alphanumeric characters up to 255 characters in size)相互鉴权, 一般存储在配置文件中(使用0600的权限, 保证仅所主可读, 或其它类似的安全方法). 文档上推荐使用分布式组件来完成部署.

其它: Docker中使用`RABBITMQ_ERLANG_COOKIE`去判断config file location, 在UNIX like OS上它一般在 `/var/lib/rabbitmq/.erlang.cookie`, CLI Tools 使用`$HOME/.erlang.cookie`

**Node Counts and Quorum**

由于一些特性: quorum queue(前端介绍过, 替代mirroring queue的下一代高可用queue), client tracking in MQTT(不做介绍) 需要节点间达成一致, 使用奇数个cluster nodes are highly recommended: 1,3,5,7 and so on.

2个节点非常不推荐, 因为它们无数达到多数胜出的一致...更多解释pass, 类型于Redis 2个sentinel在2个sentinel之间发生网络分区master down时无法达到多数, 以开启failover, 而事实上使sentinel机制失效.

From the consensus point of view, 4 或 6 个节点也3或5有同样的可用性. The [Quorum Queues guide](https://www.rabbitmq.com/quorum-queues.html) more detail.

**Clustering and Clients**

在集群所有节点可用时, client可以连接任意节点, 执行任意操作. Nodes will route operations to the [quorum queue leader](https://www.rabbitmq.com/quorum-queues.html) or [queue master replica](https://www.rabbitmq.com/ha.html#master-migration-data-locality) transparently to clients.

client一次实际只连接一个node! 在node failure时, client应该能重连接到不同的节点. 因此, 许多Client API接受 a list of endpoints(hostname or ip address) as a connection option. 这些Host会在init connection故障时使用.

对于mirrored queue与quorum queue这里不做过多说明.

**Clustering and Observability**

Client connections, channels and queues will be distributed across cluster nodes. Operators need to be able to inspect and [monitor](https://www.rabbitmq.com/monitoring.html) such resource across all cluster nodes.

CLI tools and web UI 提供了一些方便的集群信息查询能力.

**Node Failure Handling**

RabbitMQ brokers能容忍单个Node的故障, failure. Nodes can be started and stoped at will, as long as they can contact a cluster member node known at the time of shudown.

mirroring Queue允许queue contents在节点间复制. 单个节点down机, 不受影响.

而对于[non-mirroring queue failure](https://www.rabbitmq.com/ha.html#non-mirrored-queue-behavior-on-node-failure):

1. 如果是持久化的queue, master node of the queue down机, 导致queue不可用, 任何写入/消费都导致channel exception.
2. 非持久化的queue, will be deleted. queue 被删, 意味着non-route message if msg come.

**Metrics and Statistics**

Every node stores and aggregates its own metrics and stats, and provides an API for other nodes to access it. Some stats are cluster-wide, others are specific to individual nodes.

Node that responds to an HTTP API request contacts its peers to retrieve their data and then produces an aggregated result.

**Disk and RAM Nodes**

pass

**Clustering Transcript with rabbitmqctl**

pass

**Starting Independent Nodes**

pass

**Creating a Cluster**

pass

**Restarting Cluster Nodes**

pass

**Schema Syncing from Online Peers**

pass

**Restarts and Health Checks (Readiness Probes)**

pass

**Hostname Changes Between Restarts**

pass

**Cluster Node Restart Example**

pass

**Forcing Node Boot in Case of Unavailable Peers**

pass


### Classic Mirrored Queues

Ref: https://www.rabbitmq.com/ha.html


