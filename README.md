# You Need Me(YNM),  you may need those keynotes.

## Overview

- [Operating System](https://github.com/kakukosaku/OperatingSystem) link to another repo
- [Distributed System](#distributed-system): microservice, CAP theory etc.
- [Databases](#databases): MySQL, Redis
- [Middleware](#middleware): message queue, coordinator.
- [Concurrency](#concurrency): multiple-thread, lock, I/O multiplexing, event-loop base solution, coroutine/goroutine etc.
- [web components](#web-components): Nginx
- [Language Related Topics](#language-related-topics): gc, memory model and other language specific feature.

## Distributed System

- [Introduction](distributed_system/README.md)
- [microservice](distributed_system/microservice/README.md)
- [Distributed Consistency](distributed_system/consistency/README.md)
- [Distributed Lock](distributed_system/lock/READEME.md)

## Databases

### MySQL

InnoDB Related

- [InnoDB and the ACID Model](databases/mysql/innodb_and_acid_model.md)
- [InnoDB Recovery](databases/mysql/innodb_recovery.md)

High level summary for InnoDB's ACID feature and its implementation.

- [multi-version concurrent control](databases/mysql/mvcc.md)
- [InnoDB Locking and Transaction Model](databases/mysql/innodb_locking_and_transaction_model.md) Included subtopics:
- [locking](databases/mysql/innodb_locking.md)
- InnoDB Transaction Model - [isolation level](databases/mysql/isolation_level.md)
- InnoDB Transaction Model - [autocommit, commit, and rollback](databases/mysql/autocommit_commit_rollback.md)
- InnoDB Transaction Model - Consistent Nonlocking Reads
- InnoDB Transaction Model -[locking read](databases/mysql/locking_read.md)
- [Locks Set by Different SQL Statements in InnoDB]()
- Phantom Rows
- Deadlocks in InnoDB
- Transaction Scheduling
- InnoDB On-Disk & In-Memory Structures

High Available

- [Replication](databases/mysql/replication.md)
- [InnoDB Cluster](databases/mysql/innodb_cluster.md)

Others

- [explain explain](databases/mysql/explain_explain.md)

Optimize

- [order by & limit _offset, _count](databases/mysql/order_by_limit_offset.md)

### Redis

- [Efficient Data Type](databases/redis/README.md#efficient-data-type)
- [Persistence: RDB & AOF](databases/redis/README.md#persistence-rdbaof)
- [Replication](databases/redis/README.md#replication)
- [Sentinel](databases/redis/README.md#sentinel)
- [Cluster](databases/redis/README.md#cluster)

## Middleware

Rabbit MQ

- [Reliability](middleware/mq/rabbitmq/reliability_guide.md)

Include 1. Ack and Confirms; 2. Clustering; 3. Queue Mirroring; 4. Publishers & Consumers; and alert, Monitoring, Metrics and health check.

kafka

- [Overview](middleware/mq/kafka/README.md)

zookeeper

- [Overview](middleware/zookeeper_overview.md)

## Concurrency

Introduction

- [The C10K Problem](http://www.kegel.com/c10k.html)

Glossary

- [Memory Barrier, 内存屏障](https://en.wikipedia.org/wiki/Memory_barrier)

> is a type of barrier instruction that causes a central processing unit (CPU) or compiler to enforce an ordering constraint
on memory operations issued before and after the barrier instruction. This typically means that operations issued prior 
to the barrier are guaranteed to be performed before operations issued after the barrier. Are typically used when build 
synchronization primitives and lock free data structures on multiprocessor systems, and device drivers that communicate 
with computer hardware.

Multiple Thread

Todo...

Lock Related

Todo...

High Concurrent I/O solution

- [I/O multiplexing](concurrent/multiplxing/README.md) more info: [I/O Models](https://github.com/kakukosaku/OperatingSystem/blob/master/topics/linux_5_io_model.md)
- Event loop + multiplexing base solution, eg [Tornado](https://www.tornadoweb.org/en/stable/)
- Coroutine
- Goroutine

## Web components

Nginx
    
- [How nginx process a request](web_components/nginx/process_request.md)
- [load balance](web_components/nginx/load_balancer.md)
- [location priority](web_components/nginx/location_priority.md)

### Language related topics

- [Garbage Collection, GC](language_topics/gc.md)
- [Memory model](language_topics/memory_model.md) & [Memory Management(or layout)](language_topics/memory_management.md)

**Should be familiar with** [shell](language_topics/shell/README.md)

