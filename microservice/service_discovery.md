Service Discovery in a Microservices Architecture

Ref:

https://www.nginx.com/blog/service-discovery-in-a-microservices-architecture/

文章主要内容:

1. [服务发现](#服务发现)
2. [服务注册](#服务注册)

### 服务发现

分为Client-side / Server-side

**Client-side**

> When using client‑side discovery, the client is responsible for determining the network locations of available service instances and load balancing requests across them. The client queries a service registry, which is a database of available service instances. The client then uses a load‑balancing algorithm to select one of the available service instances and makes a request.

客户端需要从 `service registry` (服务注册中心)获取服务列表, 将请求均衡到不同的服务实例上, 如下图示

![Client-side services discovery](https://www.nginx.com/wp-content/uploads/2016/04/Richardson-microservices-part4-2_client-side-pattern.png)

> The network location of a service instance is registered with the service registry when it starts up. It is removed from the service registry when the instance terminates. The service instance’s registration is typically refreshed periodically using a heartbeat mechanism.

服务起动, 将自己的network location(IP)注册到服务注册中心, 停止时主动通知服务注册中心下线(或心跳检测下线). 通过心跳检测机制判活(详见后服务注册部分)

> Netflix OSS provides a great example of the client‑side discovery pattern. Netflix Eureka is a service registry. It provides a REST API for managing service‑instance registration and for querying available instances. Netflix Ribbon is an IPC client that works with Eureka to load balance requests across the available service instances. We will discuss Eureka in more depth later in this article.

[Netflix Eureka](https://github.com/Netflix/eureka)(服务注册中心) + [Netflix Ribbon](https://github.com/Netflix/ribbon) 提供了示范实现

Pros:

- 客户端由于知晓可用服务实例, 客户端对于负载算法有更多操作空间

Cons:

- 客户端实现, 加重了客户端耦合的逻辑, 且不同的(语言)客户端需要各自实现服务发现, 增加维护/接入成本

**Server-side**

> The client makes a request to a service via a load balancer. The load balancer queries the service registry and routes each request to an available service instance. As with client‑side discovery, service instances are registered and deregistered with the service registry.

客户端请求服务通过负载均衡器(load balancer), 而负载均衡器通过服务注册中心更新自己的可用服务列表, 如下图示

![Server-side services discovery](https://www.nginx.com/wp-content/uploads/2016/04/Richardson-microservices-part4-3_server-side-pattern.png)

此外, 原文也介绍了通过NGINX Plus + NGINX作为load balancer, 通过[Consul Template](https://github.com/hashicorp/consul-template)动态配置NGINX Conf的机制; [Kubernetes](https://github.com/kubernetes/kubernetes/)这样部署环境  此处不赘述.

> Some deployment environments such as Kubernetes and Marathon run a proxy on each host in the cluster. The proxy plays the role of a server‑side discovery load balancer. In order to make a request to a service, a client routes the request via the proxy using the host’s IP address and the service’s assigned port. The proxy then transparently forwards the request to an available service instance running somewhere in the cluster.


Pros:

- 简化了Client-side服务发现的臃肿, 降低不同(语言)客户端接入维护成本

Cons:

>  Unless the load balancer is provided by the deployment environment, it is yet another highly available system component that you need to set up and manage.

- 如上所述, 如果非deployment environments内嵌了(Load Balancer + service registry)的功能, 你需要确保维护这套机制的高可用

### 服务注册

也分2种, 服务自注册(self-registration pattern), 三方注册(third-part registration pattern)

**self-registration**

> When using the self‑registration pattern, a service instance is responsible for registering and deregistering itself with the service registry. Also, if required, a service instance sends heartbeat requests to prevent its registration from expiring

服务启动注册, 下线通知, 定期发送心跳, 以维护自己在服务注册中心的服务列表上, 如下图示:

![self-registration](https://www.nginx.com/wp-content/uploads/2016/04/Richardson-microservices-part4-4_self-registration-pattern.png)

> A good example of this approach is the Netflix OSS Eureka client. The Eureka client handles all aspects of service instance registration and deregistration. The Spring Cloud project, which implements various patterns including service discovery, makes it easy to automatically register a service instance with Eureka. You simply annotate your Java Configuration class with an @EnableEurekaClient annotation.

原文同样也介绍了一些现有实现, 不再赘述.

Pros:

- 无需引入额外依赖系统

Cons:

> You must implement the registration code in each programming language and framework used by your services.

- 加重了服务本身与服务管理(此外为服务注册)的耦合, 每个服务都需要实现一套这样的逻辑

**third-part registration**

> When using the third-party registration pattern, service instances aren’t responsible for registering themselves with the service registry. Instead, another system component known as the service registrar handles the registration. The service registrar tracks changes to the set of running instances by either polling the deployment environment or subscribing to events. When it notices a newly available service instance it registers the instance with the service registry. The service registrar also deregisters terminated service instances.

在这种模式下, 第三方服务, 负责维护 registry, deregistry, heartbeat逻辑, 通过(polling the deployment environment or subscribing to events) tracks changes to the set of running instances, 如下图示

![third-part registration](https://www.nginx.com/wp-content/uploads/2016/04/Richardson-microservices-part4-5_third-party-pattern.png)

> One example of a service registrar is the open source Registrator project. It automatically registers and deregisters service instances that are deployed as Docker containers. Registrator supports several service registries, including etcd and Consul.

> Another example of a service registrar is NetflixOSS Prana. Primarily intended for services written in non‑JVM languages, it is a sidecar application that runs side by side with a service instance. Prana registers and deregisters the service instance with Netflix Eureka.

同样, 原文也介绍了些已有实现, 同样对于cloud base (内嵌三方注册中心功能)的部署环境也做了些介绍.

Pros:

- 解耦了服务本身与服务注册

Cons:

> One drawback of this pattern is that unless it’s built into the deployment environment, it is yet another highly available system component that you need to set up and manage.

- 同样, 如非部署环境内嵌, 同样需要额外维护一套系统的高可用
