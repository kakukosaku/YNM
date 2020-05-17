# Kafaka documentation summary

1. Getting started

Apache kafka is a distributedd streaming platform. with three capabilities:

a. Pulish and subscribe to streams off records, similar to a message queue or enterprise messaging system.

b. Store streams of records in a fault-tolerant durable way.

c. Process streams of records as they occur.

kafka has five core APIs:

- Producer API allows an application to publish a stream of records to one or more kafka topics.

- Consumer API allows an applicatin to subscribe to one or more topics and process the stream of records produced to them.

- Streams API allows an application to act as a stream processor, consuming an input stream from one or more topics and producing an output stream to one or more output topics, effectively transforming the input streams to output streams.

- Connector API allows building and running reusable producers or consumers that connect kafka topics to existing applications or data systems. For example, a connector to a relational database might capture every change to a table.

- Admin API allows managinig and inspecting topics, brokers and other kafka objects.

Topics and Logs

A topic is a category or feed name to which records are published. Topics in Kafka are always multi-subscriber; that is, a topic can have zero, one, or many consumers that subscribe to the data written to it...

Distribution

The partitions of the log are distributed over the servers in the Kafka cluster with each server handling data and requests for a share of the partitions. Each partition is replicated across a configurable number of servers for fault tolerance.

Each partition has one server which acts as the "leader" and zero or more servers which act as "followers". The leader handles all read and write requests for the partition while the followers passively replicate the leader. If the leader fails, one of the followers will automatically become the new leader. Each server acts as a leader for some of its partitions and a follower for others so load is well balanced within the cluster.

Geo-Replication

Producers

Producers publish data to the topics of their choice. The producer is responsible for choosing which record to assign to which partition within the topic. This can be done in a round-robin fashion simply to balance load or it can be done according to some semantic partition function (say based on some key in the record).

Consumers

Consumers label themselves with a consumer group name, and each record published to a topic is delivered to one consumer instance within each subscribing consumer group. Consumer instances can be in separate processes or on separate machines.

Multi-tenancy

Guarantees

Messages sent by a producer to a particular topic partition will be appended in the order they are sent. That is, if a record M1 is sent by the same producer as a record M2, and M1 is sent first, then M1 will have a lower offset than M2 and appear earlier in the log.

A consumer instance sees records in the order they are stored in the log.

For a topic with replication factor N, we will tolerate up to N-1 server failures without losing any records committed to the log.

Use Cases

Messaging

Website Activity Tracking

Metrics

Log Aggregation

Stream Processing

Event Sourcing

Commit Log


