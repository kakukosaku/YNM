# InnoDB Cluster

## Overview

Ref: https://dev.mysql.com/doc/refman/8.0/en/mysql-innodb-cluster-userguide.html

- [Introducing InnoDB Cluster](#introducing)
- Creating an InnoDB Cluster
- Upgrading an InnoDB Cluster
- Using MySQL Router with InnoDB Cluster
- Working with InnoDB Cluster
- InnoDB ReplicaSet
- Tagging the Metadata
- Known Limitations

### Introducing

MySQL InnoDB cluster provides a complete high available solution for MySQL. 组成集群至少需要3个MySQL server instance. each instance runs MySQL Group Replication.

> MySQL Group Replication 在MySQL Replication基础之上提供了1. single-primary mode with automatic primary election. 2. multi-master concurrently update.
>
> 这里额外提一下的是: if group member becomes unavailable, 连接其上的client需要redirect, or failed over, to a different server in the group. MySQL Group Replication并未提供该能力,
> 需要使用其它组件配合, 如 connector, load balancer, router, or some form of middleware.
>
> (关于Group Replication就没有单独的文章介绍了就...)

MySQL Router 会根据你部署的cluster自动配置自己, 连接其上的application对真正的MySQL instance透明. 集群中instance故障时, 集群自动reconfigure. 各组件有如下图示关系:

![innodb_cluster_overview](https://dev.mysql.com/doc/refman/8.0/en/images/innodb_cluster_overview.png)
