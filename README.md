# You Need Me(YNM),  you may need those keynotes.

## Overview

1. [Operating System](https://github.com/kakukosaku/OperatingSystem) link to another repo
2. [Databases](#databases): MySQL, Redis
3. [Middleware](#middleware): message queue, coordinator.
4. [web components](#web-components): Nginx

some conceptual topics

1. [Concurrent](#concurrent): multiple-thread, lock, I/O multiplexing, event-loop base solution, coroutine/goroutine etc.
2. [Distributed System](#distributed-system): microservice, CAP theory etc.
3. [Language related topics](#language-related-topics): gc, memory model and other language specific feature.

### Databases

**MySQL**

InnoDB Related

- [InnoDB and the ACID Model](databases/mysql/innodb_and_acid_model.md)

High level summary for InnoDB's ACID feature and its implementation.

- [multi-version concurrent control](databases/mysql/mvcc.md)
- [InnoDB Locking and Transaction Model](databases/mysql/innodb_locking_and_transaction_model.md)

Include 1. lock; 2. isolation level;  3. consistent nonlocking read; 4. locking read; 5. deadlocks; 6. transaction scheduling, etc...

- Locks Set by Different SQL Statements in InnoDB
- Deadlocks in InnoDB
- Transaction Scheduling

High Available

- [Replication](databases/mysql/replication.md)
- [InnoDB Cluster](databases/mysql/innodb_cluster.md)

Others

- [explain explain](databases/mysql/explain_explain.md)
- [autocommit commit rollback](databases/mysql/autocommit_commit_rollback.md)

**Redis**

- [Efficient Data Type](databases/redis/README.md#efficient-data-type)
- [Persistence: RDB & AOF](databases/redis/README.md#persistence-rdbaof)
- [Replication](databases/redis/README.md#replication)
- [Sentinel](databases/redis/README.md#sentinel)
- [Cluster](databases/redis/README.md#cluster)

### Middleware

**Rabbit MQ**

- [Reliability](middleware/mq/rabbitmq/reliability_guide.md)

Include 1. Ack and Confirms; 2. Clustering; 3. Queue Mirroring; 4. Publishers & Consumers; and alert, Monitoring, Metrics and health check.

**kafka**

- [Overview](middleware/mq/kafka/README.md)

**zookeeper**

- [Overview](middleware/zookeeper_overview.md)

### Web components

**Nginx**
    
- [How nginx process a request](web_components/nginx/process_request.md)
- [load balance](web_components/nginx/load_balancer.md)
- [location priority](web_components/nginx/location_priority.md)

### Distributed System

[Introduction](distributed_system/README.md)

[microservice](distributed_system/microservice/README.md)

[Distributed Consistency](distributed_system/consistency/README.md)

[Distributed Lock](distributed_system/lock/READEME.md)

### Concurrent

**Multiple thread**

**Lock**

**High Concurrent I/O**

- [I/O multiplexing](concurrent/multiplxing/README.md) more info: [I/O Models](https://github.com/kakukosaku/OperatingSystem/blob/master/topics/linux_5_io_model.md)

Event loop + multiplexing base solution, eg [Tornado](https://www.tornadoweb.org/en/stable/)

Coroutine

Goroutine

### Language related topics

- [Garbage Collection, GC](language_topics/gc.md)
- [Memory model](language_topics/memory_model.md) & [Memory Management(or layout)](language_topics/memory_management.md)

**[Should be familiar to shell]()**
