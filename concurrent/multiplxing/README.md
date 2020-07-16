# I/O multiplexing

ref: 

1. https://wiki.c2.com/?IoMultiplexing wiki, 少量引用
2. http://davmac.org/davpage/linux/async-io.html 重要参考
3. https://notes.shichao.io/unp/ch6/#select-function 重要参考
4. https://xuri.me/2017/08/06/io-multiplexing-in-linux.html 很多前置概念的阐述
5. https://man7.org/linux/man-pages/man2/select.2.html wiki 少量引用
6. http://lse.sourceforge.net/io/aio.html AIO wiki
7. https://blog.cloudflare.com/io_submit-the-epoll-alternative-youve-never-heard-about/ AIO 的介绍, io_submit

> I/O multiplexing means what it says - allowing the programmer to examine and block on multiple I/O streams (or other "synchronizing" events), being notified whenever any one of the streams is active so that it can process data on that stream.

IO多路复用实际是允许程序等待在多个I/O操作, 直到关心的I/O就绪得到通知...

(野生翻译, 不够精准, 强烈推荐扫过一遍"UNIX 网络编程 卷1", 与此篇的阅读前后均可, 始终是需要综合理解的; 脱离书本能准确的讲述, 才能称之为学会了! 🙃)

There are 3 options you can use in Linux:

- select(2)
- poll(2)
- epoll

select(2) 2是指 select is system call, 其它还有1: user command; 在 `> man man` 中全部枚举

> All the above methods serve the same idea, create a set of file descriptors, tell the kernel what would you like to do with each file descriptor (read, write, ..)
> and use one thread to block on one function call until at least one file descriptor requested operation available.

以上三种实际上都是告诉kernel对哪些fd感"兴趣"(或者叫监听), 然后用一个线程阻塞在function call, 直至至少一个fd请求已就绪.

实际上 `select` 之类的system call, 可以:

```markdown
- wait "forever" timeout is specified as null pointer, Return until at least one file descriptor requested operation available.
- wait up to a fixed amount of time(timeout points to a `timeval` structure) 或者有就绪fd.
- do not wait at all (timeout points to a timeval structure and the timer value is 0, i.e. the number of seconds and microseconds specified by the structure are 0). 或者有就绪fd.
```

1. 一直等待, 直到至少一个fd就绪
2. 等待一定时间, 期满返回或期间有fd就绪返回
3. 或者如果没有就绪fd, 干脆就不等待

第三种也被称为polling的方式

### Select Function

The select() system call provides a mechanism for implementing synchronous multiplexing I/O

```c
int select(int maxfdp1, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```

对 `select()` 的调用将block until指定的 file descriptor are ready to perform I/O 或者到达指定的timeout

监听的file descriptors 被分为三类:

- `readfds` set are watched to see if data is available for reading.
- `writefds` set are watched to see if write operation is ready without blocking.
- `exceptionfds` are watched to see if an exception has occurred...

各file descriptor sets can be NULL, 此时 `select()` does not watch for that event.

> On return, select() replaces the given descriptor sets with subsets consisting of those descriptors that are ready for the requested operation.  select() returns the total number of ready descriptors in all the sets.

select 返回就绪的fd数量, 并标记入参中相应的 `fdsets` 中"就绪"的file descriptor. 清除其它fds. fdsets 实质是bit map, 可通过一些内置的宏操作之: 

```c
void FD_ZERO(fd_set *fdset);         /* clear all bits in fdset */
void FD_SET(int fd, fd_set *fdset);  /* turn on the bit for fd in fdset */
void FD_CLR(int fd, fd_set *fdset);  /* turn off the bit for fd in fdset */
int FD_ISSET(int fd, fd_set *fdset); /* is the bit for fd on in fdset ? */
```

from `select(2)` manual page.

> readset, writeset, and exceptset as value-result arguments *

readset, writeset and exceptset 都是"结果变量", 用更高级的语言来说, 是reference var, select函数对其修改, 外部可见.

Return value of select:

> The return value from this function indicates the total number of bits that are ready across all the descriptor sets
> If the timer value expires before any of the descriptors are ready, a value of 0 is returned.
> A return value of –1 indicates an error (which can happen, for example, if the function is interrupted by a caught signal).
  
select 返回值是说, 就绪描述符数量. 

When an error occurs on a socket, it is marked as both readable and writable by select.

当异常发生时, select 的调用是读写均就绪状态, 此时调用non block.

**Maximum Number of Descriptors for `select`**

> select uses descriptor sets, typically an array of integers, with each bit in each integer corresponding to a descriptor. For example, using 32-bit integers, the first element of the array corresponds to descriptors 0 through 31, the second element of the array corresponds to descriptors 32 through 63, and so on. All the implementation details are irrelevant to the application and are hidden in the fd_set datatype and the following four macros:

```c
// 仅供示意 https://moythreads.com/wordpress/2009/12/22/select-system-call-limitation/
typedef struct  {
    long int fds_bits[32];
} fd_set;
```

`[32]int, go datatype syntax` 来表示fd 的bit map. 对应位的bit被置为1, 即表明该fd需要监听...而int早期一般为32bit, 32*32 = 1024 的由来...被记录为 FD_SETSIZE, 该值可改在后续的 linux 版本中. (早期需要重新编译kernel)


> When select was originally designed, the OS normally had an upper limit on the maximum number of descriptors per process (the 4.2BSD limit was 31),
> and select just used this same limit. But, current versions of Unix allow for a virtually unlimited number of descriptors per process
> (often limited only by the amount of memory and any administrative limits), which affects select.

系统一般会限制每个进程所能持有的最大文件描述符, 这个值实际来说也影响select, 一般select也使用相同的限制, 但现代操作系统也提供了修改该值的方法, 这同样会影响select.

Many implementations have declarations similar to the following, which are taken from the 4.4BSD <sys/types.h> header:

```c
/*
 * Select uses bitmasks of file descriptors in longs. These macros
 * manipulate such bit fields (the filesystem macros use chars).
 * FD_SETSIZE may be defined by the user, but the default here should
 * be enough for most uses.
 */
#ifndef FD_SETSIZE
#define FD_SETSIZE      256
#endif
```

但起来好像可以直接通过 `#define FD_SETSIZE` 来增加该值(used by select), but...unfortunately, this normally does not work..

> The three descriptor sets are declared within the kernel and also uses the kernel's definition of FD_SETSIZE as the upper limit.
> The only way to increase the size of the descriptor sets is to increase the value of FD_SETSIZE and then recompile the kernel.

Changing the value without recompiling the kernel is inadequate.

以上为UNIX Network Programming, UNP中的原话...

熟悉C语言, 应该知道, 数组作为参数时, 实质是指向数组第一个元素的地址(丢失了长度信息), 所以需要额外告诉函数数组长度. 而fd_set *readfds之类却没有. 原因在于`FD_SETSIZE`, 内核以预定义的宏来设置readfds中的位(前已说明, readfds实质是数组形式的bit map)

**select-summary**

- int select(int maxfdp1, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout)

在Mac OS上, select function signature as follow:

```c
int select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds, fd_set *restrict errorfds, struct timeval *restrict timeout);
```

- maxfdp1 则是告诉内核最大描述符的"数量"(或者叫最大"监听"描述符的索引值) 这样, 对于fd_set中超过的部分, 内核不再关心. 比如:

(Example: If you have set two file descriptors "4" and "17", nfds should  not be "2", but rather "17 + 1" or "18".) From `man select` page.

- 函数返回时, 将相应 fd_set 中未就绪的 fd 位置0, 意味着你1.需要iterate over the bit map to check 关心的fd已就绪; 2.并且再次调用前设置"关心"(与监听同义这里)的fd

- 实际 select 可监听的 fd 数量不仅与 `FD_SETSIZE` 有关, 还受系统允许单个进程最多可持有的fd限制.

### poll Function

```c
#include <poll.h>

int poll (struct pollfd *fdarray, unsigned long nfds, int timeout);

/* Returns: count of ready descriptors, 0 on timeout, –1 on error */
```

参数:

The first argument (fdarray) is a pointer to the first element of an array of structures.

Each element is a pollfd structure that specifies the conditions to be tested for a given descriptor, fd. 

```c
struct pollfd {
  int     fd;       /* descriptor to check */
  short   events;   /* events of interest on fd */
  short   revents;  /* events that occurred on fd */
};
```

events, revents 分别作为参数值和结果值, 避免了像 select "原地" 修改参数. 一些常用于events, revents的标志的如下图:

![events_marks](https://notes.shichao.io/unp/figure_6.23.png)

The second argument (nfds): The number of elements in the array of structures is specified by the nfds argument.
                           
第二个参数指定数组中元素的个数.

The timeout argument specifies how long the function is to wait before returning. 

A positive value specifies the number of milliseconds to wait. The constant INFTIM (wait forever) is defined to be a negative value.

第三个参数, `int timeout`

Return values from poll:

- –1 if an error occurred
- 0 if no descriptors are ready before the timer expires
- Otherwise, it is the number of descriptors that have a nonzero revents member.

**此时再看 select FD_SETSIZE 的限制**

由于分配了一个 `pollfd` 结构的数组, 且将其数目通知内核成了调用者的责任, 内核也不再需要知道类似 fd_set 的固定固定大小的数据类型, 自然也就不存在像 `select` 中那样的限制.

**poll compare to select, poll summary**

- int poll(struct pollfd fds[], nfds_t nfds, int timeout);

显然, 更...传统的C函数定义, 通过将数组&数组长度构造/传递的责任交给 调用方, kernel 不再需要fd_set & FD_SETSIZE. 自然也就没了 FD_SETSIZE的限制, 但仍受进程可打开最大fd数量限制

- fds 是 pollfd 的数组, 里面只有需要"监听"的fd, 提高了效率(原先是只告诉最高位fd以下需要 iterate)

- `struct pollfd` 的定义上面也写了, 带来的优势便是无需每次 poll 返回后, 重新设置"关心"的fd.

- 缺点就是 poll 不一定都支持...select is more portable :)

### Epoll Function 

While working with select and poll we manage everything on user space and we send the sets on each call to wait. 

To add another socket we need to add it to the set and call select/poll again.

首先需要说明的是 epoll 是linux实现, mac OS上没有...mac OS的 `kqueue`, 有空再介绍...

与select, poll由我们指定 fd sets 然后再wait on call不同, epoll help us to create and manage the context in the kernel. We divide the task to 3 steps:

- create a context in the kernel using epoll_create
- add and remove file descriptors to/from the context using epoll_ctl
- wait for events in the context using epoll_wait

以下描述来自ubuntu 16.04 man page

```markdown
DESCRIPTION
       The  epoll  API performs a similar task to poll(2): monitoring multiple file descriptors to see if I/O is possible on any of them.  The epoll API can be used either as an edge-triggered or a level-
       triggered interface and scales well to large numbers of watched file descriptors.  The following system calls are provided to create and manage an epoll instance:

       *  epoll_create(2) creates an epoll instance and returns a file descriptor referring to that instance.  (The more recent epoll_create1(2) extends the functionality of epoll_create(2).)

       *  Interest in particular file descriptors is then registered via epoll_ctl(2).  The set of file descriptors currently registered on an epoll instance is sometimes called an epoll set.

       *  epoll_wait(2) waits for I/O events, blocking the calling thread if no events are currently available.
```

关于 edge-triggered and level-triggered man page 如下描述:

```markdown
Level-triggered and edge-triggered
   The epoll event distribution interface is able to behave both as edge-triggered (ET) and as level-triggered (LT).  The difference between the two mechanisms can be described  as  follows.   Suppose
   that this scenario happens:

   1. The file descriptor that represents the read side of a pipe (rfd) is registered on the epoll instance.

   2. A pipe writer writes 2 kB of data on the write side of the pipe.

   3. A call to epoll_wait(2) is done that will return rfd as a ready file descriptor.

   4. The pipe reader reads 1 kB of data from rfd.

   5. A call to epoll_wait(2) is done.

   If  the  rfd  file  descriptor  has been added to the epoll interface using the EPOLLET (edge-triggered) flag, the call to epoll_wait(2) done in step 5 will probably hang despite the available data
   still present in the file input buffer; meanwhile the remote peer might be expecting a response based on the data it already sent.  The reason for this is that edge-triggered mode  delivers  events
   only  when  changes  occur  on the monitored file descriptor.  So, in step 5 the caller might end up waiting for some data that is already present inside the input buffer.  In the above example, an
   event on rfd will be generated because of the write done in 2 and the event is consumed in 3.  Since the read operation done in 4 does not consume the whole buffer data, the call  to  epoll_wait(2)
   done in step 5 might block indefinitely.

   An  application  that  employs  the EPOLLET flag should use nonblocking file descriptors to avoid having a blocking read or write starve a task that is handling multiple file descriptors.  The sug‐
   gested way to use epoll as an edge-triggered (EPOLLET) interface is as follows:

          i   with nonblocking file descriptors; and

          ii  by waiting for an event only after read(2) or write(2) return EAGAIN.

   By contrast, when used as a level-triggered interface (the default, when EPOLLET is not specified), epoll is simply a faster poll(2), and can be used wherever the latter is used since it shares the
   same semantics.

   Since  even with edge-triggered epoll, multiple events can be generated upon receipt of multiple chunks of data, the caller has the option to specify the EPOLLONESHOT flag, to tell epoll to disable
   the associated file descriptor after the receipt of an event with epoll_wait(2).  When the EPOLLONESHOT flag is specified, it is the caller's responsibility  to  rearm  the  file  descriptor  using
   epoll_ctl(2) with EPOLL_CTL_MOD.
```

level-triggered is ready until consume the whole buffer data.

edge-triggered is ready only  when  changes  occur  on the monitored file descriptor.

when used as a level-triggered interface (the default, when EPOLLET is not specified), epoll is simply a faster poll(2)

It differs both from poll and select in such a way that it keeps the information about the currently monitored descriptors and associated events inside the kernel, and exports the API to add/remove/modify those.

```c
#include <sys/epoll.h>

int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
```

**epoll vs. select/poll epoll summary**

- We can add and remove file descriptor while waiting
- 与select, poll 仅返回就绪fd个数, 原地置空"监听"的fd_set中未就绪的fd不同, epoll_wait将仅将就绪的fd添加进 epoll_wait 参数 events中, 下有示例
- 正因如下原因, epoll has better performance - O(1) instead of O(n) (select/poll 仍需要iterate n 的 fd_set中的fd)
- 缺点是 epoll 是linux 中特有的实现 not portable

```c
struct epoll_event events[5];
int epfd = epoll_create(10);
// ...
// ...
for (i=0;i<5;i++) 
{
    static struct epoll_event ev;
    memset(&client, 0, sizeof (client));
    addrlen = sizeof(client);
    ev.data.fd = accept(sockfd,(struct sockaddr*)&client, &addrlen);
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev); 
}

while(1){
	puts("round again");
	nfds = epoll_wait(epfd, events, 5, 10000);
	
	for(i=0;i<nfds;i++) {
			memset(buffer,0,MAXBUF);
			read(events[i].data.fd, buffer, MAXBUF);
			puts(buffer);
	}
}
```
