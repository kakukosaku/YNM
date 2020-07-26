# Replication

Ref: https://dev.mysql.com/doc/refman/8.0/en/replication.html

MySQL Replication is asynchronous be default...Advantages of replication include:

## Overview

1. Scale-out solution: 读写分离, 提升throughout.
2. Data security: 从库提供一定的数据容灾能力.
3. Analytics: 从库做数据分析 without affecting the performance of the source.
4. Long-distance data. local copy of the remote site to use.

### 17.1 Configuring Replication

这一部分描述了怎样配置不同类型的 replication 及它们所需要的环境, 主要包括了以下几个 major components:

- 通过 binary log file positions 的replication. deal with the configuration of the servers and provides methods for coping data between the source and replicas.
- 通过 GTID(global transaction identifiers) transactions 的replication.
- bin log 中 event 的几种formats. 3种: 1. statement-base replication (SBR) 2. row-based replication(RBR) 3. mixed-format replication(MIXED)
- Replication机制的提供的各种配置项细节
- Common Replication Administration Tasks

more info pass

### 17.2 Replication Implementation

Replication 基于 source server 记录其数据变化于binary log中.

从节点(Replicas)向主节点(source server)请求binary log. Pull data from the source, rather than source pushing to replicas.

Source servers and replicas report their status in respect of the replication process regularly so that you can monitor them.

The source's binary log is written to a local relay log on the replica before it is processed.

从节点(replicas) 可以过滤database changes 基于配置的各种rule....

more info pass

### 17.3 Replication Security

三个维度的安全:

1. encrypted connection over TLS
2. encrypted binary log file and relay log files on sources and replicas
3. privilege check, ACL security.

### 17.4 Replication Solutions

Replication可以用于许多不同目的, 此部分provides general notes and advice. e.g:

- backup
- scale-out
- improve replication performance
- swich source during failover
- replication security

![failover_replication](https://dev.mysql.com/doc/refman/8.0/en/images/redundancy-after.png)

more info pass

### 17.5 Replication Notes and Tips

- Replication Features and Issues
- Replication Compatibility between MySQL Version
- Upgrading Replication Setup
- Troubleshooting Replication
- How to Report Replication Bugs or Problems

more info pass
