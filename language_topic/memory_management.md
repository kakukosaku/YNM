# Memory Management

## Overview

## Calalog

- [Python Memory Management](#ptyhon-memory-mangement)

### Ptyhon Memory Mangement

在了解GC之前, 需要首先明白它的内存分配模型.

Everything in Python is an object. Some objects can hold other objects, such as lists, tuples, dicts, classes, etc. Because of dynamic Python's nature, such an approach requires a lot of small memory allocations. To speed-up memory operations and reduce fragmentation Python uses a special manager on top of the general-purpose allocator, called PyMalloc.

https://github.com/python/cpython/blob/ad051cbce1360ad3055a048506c09bc2a5442474/Objects/obmalloc.c#L534

![hierarchical model](https://rushter.com/static/uploads/img/memory_layers.svg)

**Small object allocation**

> To reduce overhead for small objects (less than 512 bytes) Python sub-allocates big blocks of memory. Larger objects are routed to standard C allocator. Small object allocator uses three levels of abstraction — arena, pool, and block.

为了减少小对象分配的消耗(less than 512 bytes), Python会为大块内存分配子空间。 较大的对象将路由到标准C分配器。 小对象分配器使用三个抽象级别 arena, pool, and block.

Let's start with the smallest structure -- block

**Block**

Block is a chunk of memory of a certain size. Each block can keep only one Python object of a fixed size. The size of the block can vary from 8 to 512 bytes and must be a multiple of eight (i.e., use 8-byte alignment). For convenience, such blocks are grouped in 64 size classes.

too intuitive to no need translate 🙃.

| Request in bytes | Size of allocated block | size class idx |
|------------------|-------------------------|----------------|
| 1-8	           | 8	                     | 0              |
| 9-16	           | 16	                     | 1              |
| 17-24	           | 24	                     | 2              |
| 25-32	           | 32	                     | 3              |
| 33-40	           | 40	                     | 4              |
| 41-48	           | 48	                     | 5              |
| ...	           | ...	                 | ...            |
| 505-512	       | 512	                 | 63             |

**Pool**

A collection of blocks of the same size is called a pool. Normally, the size of the pool is equal to the size of a memory page, i.e., 4Kb.

Limiting pool to the fixed size of blocks helps with fragmentation. If an object gets destroyed, the memory manager can fill this space with a new object of the same size.

pool中存储的是相同大小的block!, pool 之间以double linked list连接.

Each pool has a special header structure, which is defined as follows:

```c
/* Pool for small blocks. */
struct pool_header {
    union { block *_padding;
            uint count; } ref;          /* number of allocated blocks    */
    block *freeblock;                   /* pool's free list head         */
    struct pool_header *nextpool;       /* next pool of this size class  */
    struct pool_header *prevpool;       /* previous pool       ""        */
    uint arenaindex;                    /* index into arenas of base adr */
    uint szidx;                         /* block size class index        */
    uint nextoffset;                    /* bytes to virgin block         */
    uint maxnextoffset;                 /* largest valid nextoffset      */
};
```

> 话外音: 计算机领域, 分层, 分块, 简单的方法背后是对问题深刻的认识...

需要额外关注的fields: nextpool & prevpool, ref.count, szidx, freeblock, arenaindex...

The freeblock field is described as follows:

```
Blocks within pools are again carved out as needed.  pool->freeblock points to
the start of a singly-linked list of free blocks within the pool.  When a
block is freed, it's inserted at the front of its pool's freeblock list.  Note
that the available blocks in a pool are *not* linked all together when a pool
is initialized.  Instead only "the first two" (lowest addresses) blocks are
set up, returning the first such block, and setting pool->freeblock to a
one-block list holding the second such block.  This is consistent with that
pymalloc strives at all levels (arena, pool, and block) never to touch a piece
of memory until it's actually needed.

 So long as a pool is in the used state, we're certain there *is* a block
available for allocating, and pool->freeblock is not NULL.  If pool->freeblock
points to the end of the free list before we've carved the entire pool into
blocks, that means we simply haven't yet gotten to one of the higher-address
blocks.  The offset from the pool_header to the start of "the next" virgin
block is stored in the pool_header nextoffset member, and the largest value
of nextoffset that makes sense is stored in the maxnextoffset member when a
pool is initialized.  All the blocks in a pool have been passed out at least
once when and only when nextoffset > maxnextoffset.
```

Therefore, If a block is empty instead of an object, it stores an address of the next empty block. This trick saves a lot of memory and computation.

freeblock是 singly-linked list, 通过nextoffset指向下一未使用block, block为空时, it stores an address of next empty block.

Each pool has three states:

- used
- full
- empty

In order to efficiently manage pools Python uses an additional array called usedpools. It stores pointers to the pools grouped by class.

图示:

![usedpool](https://rushter.com/static/uploads/img/usedpools.svg)

Note that pools and blocks are not allocating memory directly, instead, they are using already allocated space from arenas.

**Arena**

The arena is a chunk of 256kB memory allocated on the heap, which provides memory for 64 pools.

```c
struct arena_object {
    uintptr_t address;
    block* pool_address;
    uint nfreepools;
    uint ntotalpools;
    struct pool_header* freepools;
    struct arena_object* nextarena;
    struct arena_object* prevarena;
};
```

same as pools connect each, all arenas are linked using doubly linked list.

**Memory deallocation**

Python's small object manager rarely returns memory back to the Operating System.

An arena gets fully released If and only if all the pools in it are empty. For example, it can happen when you use a lot of temporary objects in a short period of time.

Speaking of long-running Python processes, they may hold a lot of unused memory because of this behavior.

**allocation statistics**

You can get allocations statistics by calling sys._debugmallocstats().

[tracemalloc - Trace memory allocations](https://docs.python.org/3/library/tracemalloc.html)