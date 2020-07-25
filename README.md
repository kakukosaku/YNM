# You Need Me(YNM),  you may need those keynotes.

## Overview

1. [Operating System](https://github.com/kakukosaku/OperatingSystem) link to another repo
2. [Databases](#databases): MySQL, Redis
3. [web components](#web-components): Nginx
4. [Middleware](#middleware): message queue

some conceptual topics

1. [Language related topics](#language-related-topics): gc, memory model and other language specific feature.
2. [Concurrent](#concurrent): multiple-thread, lock, I/O multiplexing, event-loop base solution, coroutine/goroutine etc.
3. [Distributed System](#distributed-system): microservice, CAP theory etc.

### Databases

MySQL

- [explain explain](databases/mysql/explain_explain.md)
- [autocommit commit rollback](databases/mysql/autocommit_commit_rollback.md)
- [isolation level](databases/mysql/isolation_level.md)
- [multi-version concurrent control](databases/mysql/mvcc.md)
- [InnoDB](databases/mysql/innodb_lock.md)
- [Replication](databases/mysql/replication.md)

Redis

- [Efficient Data Type](databases/redis/README.md#efficient-data-type)
- [Persistence: RDB & AOF](databases/redis/README.md#persistence-rdbaof)
- [Replication](databases/redis/README.md#replication)
- [Sentinel](databases/redis/README.md#sentinel)
- [Cluster](databases/redis/README.md#cluster)

### Web components

Nginx
    
- [How nginx process a request](web_components/nginx/process_request.md)
- [load balance](web_components/nginx/load_balancer.md)
- [location priority](web_components/nginx/location_priority.md)

### Middleware

[RabbitMQ](middleware/mq/rabbitmq/README.md)

- [Publish Confirm & Consumer ACK](middleware/mq/rabbitmq/publish_confirm_consumer_ack.md)

[kafka](middleware/mq/kafka/README.md)

[zookeeper](middleware/zookeeper_overview.md)

### Language related topics

Demonstration with Java, Python or Go

- [Garbage Collection, GC](language_topics/gc.md)
- [Memory model](language_topics/memory_model.md) & [Memory Management(or layout)](language_topics/memory_management.md)

shell

- [QuickStart]()

### Concurrent

Multiple thread

Lock

[I/O multiplexing](concurrent/multiplxing/README.md) more info: [I/O Models](https://github.com/kakukosaku/OperatingSystem/blob/master/topics/linux_5_io_model.md)

Event loop + multiplexing base solution, eg [Tornado](https://www.tornadoweb.org/en/stable/)

Coroutine

Goroutine

### Distributed System

[Introduction](distributed_system/README.md)

[microservice](distributed_system/microservice/README.md)

[Distributed Consistency](distributed_system/consistency/README.md)

[Distributed Lock](distributed_system/lock/READEME.md)
