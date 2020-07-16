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

[MySQL](databases/mysql/README.md)

- [explain explain](databases/mysql/explain_explain.md)
- [autocommit commit rollback](databases/mysql/autocommit_commit_rollback.md)
- [isolation level](databases/mysql/isolation_level.md)
- [multi-version concurrent control](databases/mysql/mvcc.md)
- [InnoDB lock](databases/mysql/innodb_lock.md)
- [Mysql High Availability, MHA](databases/mysql/mha.md)

[Redis](databases/redis/README.md)

- [Redis](databases/redis/README.md)

### Web components

Nginx
    
- [How nginx process a request](web_components/nginx/process_request.md)
- [load balance](web_components/nginx/load_balancer.md)
- [location priority](web_components/nginx/location_priority.md)

### Middleware

[RabbitMQ](middleware/mq/rabbitmq/README.md)

- [Publish Confirm & Consumer ACK](middleware/mq/rabbitmq/publish_confirm_consumer_ack.md)

[kafka](middleware/mq/kafka/README.md)

### Language related topics

- [garbage collection](language_topics/gc/README.md)

shell

- [QuickStart]()

### Concurrent

Multiple thread

Lock

[I/O multiplexing](concurrent/multiplxing/README.md) more info: [I/O Models](https://github.com/kakukosaku/OperatingSystem/blob/master/topics/linux_5_io_model.md)

Event loop + multiplexing base solution

Coroutine

Goroutine

### Distributed System

[Introduction](distributed_system/README.md)

[microservice](distributed_system/microservice/README.md)
