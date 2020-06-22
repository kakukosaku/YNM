# Garbage Collection, GC

## Overview

Understanding how GC works can help you write better and faster programs.

agnostic

- Reference Counting
- Generational gc
- Mark and Sweep Algorithm

Language Specific:

- Java
- Python: Know [Memory model](#python-memory-model) first, then [GC](#python-gc)

Ref:

1. https://en.wikipedia.org/wiki/Garbage_collection_(computer_science)
2. https://en.wikipedia.org/wiki/Reference_counting
3. https://www.geeksforgeeks.org/mark-and-sweep-garbage-collection-algorithm/
4. https://rushter.com/blog/python-memory-managment/
5. https://rushter.com/blog/python-garbage-collector/
6. https://stackify.com/python-garbage-collection/
7. https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
8. https://www.artima.com/insidejvm/ed2/gc.html
9. https://www.quora.com/How-does-garbage-collection-in-Python-work-What-are-the-pros-and-cons#

### Python

#### Python Memory Model

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

#### Python GC

以下关于GC的描述基于 Python 3.6.

**Memory management**

标准CPython实现的垃圾回收器主要有2部分, the [reference counting](https://en.wikipedia.org/wiki/Reference_counting) collector and the generational garbage collector, know as [gc module](https://docs.python.org/3.6/library/gc.html)

引用计数法(reference counting)非常高效, 但无法解决循环引用(reference cycles), 所以Python也提供了辅助算法 - 分代循环GC算法(generational cyclic GC, that specifically deals with reference cycles).

> The reference counting module is fundamental to Python and can't be disabled, whereas the cyclic GC is optional and can be invoked manually.

注意, reference counting module 是Python的基础功能, 无法禁用, 而 generational cyclic GC却可以手动禁用, 参见上 gc module.

**Reference counting**

Reference counting is a simple technique in which objects are deallocated when there is no reference to them in a program.

Every variable in Python is a reference (a pointer) to an object and not the actual value itself. For example, the assignment statement just adds a new reference to the right-hand side.

To keep track of references, every object (even integer) has an extra field called reference count that is increased or decreased when a pointer to the object is created or deleted. See Objects, [Types and Reference Counts section](https://docs.python.org/3.6/c-api/intro.html#objects-types-and-reference-counts), for a detailed explanation.

examples, where the reference count increase:

- assignment operator
- argument passing
- appending an object to a list(object's reference will be increased).

examples, where the reference count decrease:

- leave its scope
- del by manual

> If reference counting field reaches zero, CPython automatically calls the object-specific memory deallocation function.
>
> If an object contains references to other objects, then their reference count is automatically decremented too. Thus other objects may be deallocated in turn. 

当引用计数降为0, CPython 自动调用其deallocation func, 同时decrease它所引用的对象的引用(其降为0时, 同理)

> Variables, which are declared outside of functions, classes, and blocks are called globals. Usually, such variables live until the end of the Python's process.
>
> Thus, the reference count of objects, which are referred by global variables, never drops to zero.

全局对象live until the end of the Python's process. 因此被全局对象引用的对象, reference count never drops to zero.

It's important to understand that until your program stays in a block, Python interpreter assumes that all variables inside it are in use. To remove something from memory, you need to either assign a new value to a variable or exit from a block of code. 

In Python, the most popular block of code is a function; this is where most the of garbage collection happens. That is another reason to keep functions small and simple.

The main reason why CPython uses reference counting is historical. There are a lot of debates nowadays about the weaknesses of such a technique. Some people claim that modern garbage collection algorithms can be more efficient without reference counting at all. The reference counting algorithm has a lot of issues, such as circular references, thread locking and memory and performance overhead.

The main advantage of such approach is that the objects can be immediately and easily destroyed after they are no longer needed.

**Generational garbage collector**

![reference cycle](https://rushter.com/static/uploads/img/circularref.svg)

```python
import ctypes
import gc

# We use ctypes moule  to access our unreachable objects by memory address.
class PyObject(ctypes.Structure):
    _fields_ = [("refcnt", ctypes.c_long)]

gc.disable()  # Disable generational gc

lst = []
lst.append(lst)

# Store address of the list
lst_address = id(lst)

# Destroy the lst reference
del lst

object_1 = {}
object_2 = {}
object_1['obj2'] = object_2
object_2['obj1'] = object_1

obj_address = id(object_1)

# Destroy references
del object_1, object_2

# Uncomment if you want to manually run garbage collection process 
# gc.collect()

# Check the reference count
print(PyObject.from_address(obj_address).refcnt)
print(PyObject.from_address(lst_address).refcnt)

# output:
# 1
# 1
```

上述例子中, del 语句手动释放了对 lst所指对象的引用, 在Python code中, 已无法访问lst所指对象, 然而它们仍然存在在内存中, 通过 `address` 仍可访问. 且其reference count is 1.

通过 [objgraph](https://mg.pov.lt/objgraph/) module you can visually explore such relations

Reference cycles can only occur in container objects (i.e., in objects that can contain other objects), such as lists, dictionaries, classes, tuples. Garbage collector does not track all immutable types except for a tuple. Tuples and dictionaries containing only immutable objects can also be untracked depending on certain conditions. Thus, the reference counting technique handles all non-circular references.

**When does the generational GC trigger**

> Unlike reference counting, cyclic GC does not work in real-time and runs periodically. To reduce the frequency of GC calls and pauses CPython uses various heuristics.
>
> The GC classifies container objects into three generations. Every new object starts in the first generation. If an object survives a garbage collection round, it moves to the older (higher) generation. Lower generations are collected more often than higher. Because most of the newly created objects die young, it improves GC performance and reduces the GC pause time.
> 
> In order to decide when to run, each generation has an individual counter and threshold. The counter stores the number of object allocations minus deallocations since the last collection. Every time you allocate a new container object, CPython checks whenever the counter of the first generation exceeds the threshold value. If so Python initiates the сollection process.

1. 不像引用计数, 分代回收周期性执行(每一代都有触发条件, each generation has an individual counter and thrshold.)
2. 分代回收将Container objects分为三代. 新创建的对象在fisrt generation. 存活了一个GC周期, 移入下一代. 较年轻的代会被更频繁GC, because most of the newly created object die young.

more detail:

If we have two or more generations that currently exceed the threshold, GC chooses the oldest one. That is because oldest generations are also collecting all previous (younger) generations. To reduce performance degradation for long-living objects the third generation has [additional requirements](https://github.com/python/cpython/blob/051295a8c57cc649fa5eaa43526143984a147411/Modules/gcmodule.c#L94) in order to be chosen.

The standard threshold values are set to (700, 10, 10) respectively, but you can always check them using the gc.get_threshold function.

You can also adjust them for your particular workload by using the gc.get_threshold function.

**How to find reference cycles**

It is hard to explain the reference cycle detection algorithm in a few paragraphs. But basically, GC iterates over each container object and temporarily removes all references to all container objects it references. After full iteration, all objects which reference count lower than two are unreachable from Python's code and thus can be collected.

To fully understand the cycle-finding algorithm I recommend you to read an [original proposal from Neil Schemenauer](http://arctrix.com/nas/python/gc/) and [collect](https://github.com/python/cpython/blob/7d6ddb96b34b94c1cbdf95baa94492c48426404e/Modules/gcmodule.c#L902) function from CPython's source code. Also, the Quora answers and [The Garbage Collector blog post](https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/) can be helpful.

Note that, the problem with finalizers, which was described in the original proposal, has been fixed since Python 3.4. You can read about it in the [PEP 442](http://legacy.python.org/dev/peps/pep-0442/).

#### Performance tips

Cycles can easily happen in real life. Typically you encounter them in graphs, linked lists or in structures, in which you need to keep track of relations between objects. If your program has an intensive workload and requires low latency, you need to avoid reference cycles as possible.

To avoid circular references in your code, you can use weak references, that are implemented in the weakref module. Unlike the usual references, the weakref.ref doesn't increase the reference count and returns None if an object was destroyed.

In some cases, it is useful to disable GC and use it manually. The automatic collection can be disabled by calling gc.disable(). To manually run the collection process, you need to use gc.collect().

#### How to find and debug reference cycles

Debugging reference cycles can be very frustrating especially when you use a lot of third-party libraries.

The standard gc module provides a lot of useful helpers that can help in debugging. If you set debugging flags to DEBUG_SAVEALL, all unreachable objects found will be appended to gc.garbage list.

```python
import gc

gc.set_debug(gc.DEBUG_SAVEALL)

print(gc.get_count())
lst = []
lst.append(lst)
list_id = id(lst)
del lst
gc.collect()
for item in gc.garbage:
    print(item)
    assert list_id == id(item)
```

#### Dealing with reference cycles

ref: https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/

CPython for this maintains two double-linked lists: one list of objects to be scanned, and a tentatively unreachable list.

1. 需要明确的是会产生 cycle reference 问题的是container objects之间/本身的引用造成对象已 unreachable 而reference count still not zero. 所以当collection模块的GC开始时, 将所有container objects it wants to scan on a first list. (鉴于大多数对象应该是可达的, 所以先假定list中都是可达对象, 如何判定为不可达对象条件满足再移到 unreachable list更合理/高效). 除了有 reference_count 外, 每个对象还有 gc_ref(其被初始化为reference_count).

![step1](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-1-new-page.png)

2. 然后GC扫描每个container object并将其所引用的对象的gc_ref减1([decrements by 1](http://hg.python.org/cpython/file/eafe4007c999/Modules/gcmodule.c#l386)). 换句话说, 我们仅关心“object to scan”列表以外的引用(如变量),而不关心列表中其他容器对象的引用。

![step2](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-2-new-page.pngk)

3. 再次扫描每个container objects, 将gc_ref为0的标记为 GC_TENTATIVELY_REACHABLE, 移至 tentatively unreachable list.

![step3](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-3-new-page.png)

4. 当GC扫描(第二次过程中)到"link1"object时, 由于其gc_ref为1, 它被标记为 GC_REACHABLE.

![step4](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-4-new-page.png)

5. 当GC扫描(第二次过程中)遇到了被标记为 GC_REACHABLE 的对象traverses its references to find all the objects that are reachable from it也将其标记为GC_REACHABLE.(如link2, link3, link3就从 tentatively unreachable list移出)

![step5](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-5-new-page.png)

#### More info

Most of the garbage collection is done by reference counting algorithm, which we cannot tune at all. So, be aware of implementation specifics, but don't worry about potential GC problems prematurely.

Each implementation of Python uses its own collector. For example, Jyton uses standard Java's gc (since it running on the JVM) , and PyPy uses [Mark and Sweep algorithm](https://www.geeksforgeeks.org/mark-and-sweep-garbage-collection-algorithm/k). The PyPy's gc is more complicated than CPython's and has additional optimizations http://doc.pypy.org/en/release-2.4.x/garbage_collection.html.

#### Optimization #1: limiting the time for each collection

The idea is that most objects have a very short lifespan and can thus be collected shortly after their creation. The flip side is that the older an object, the less likely it is to be unreachable.

The concept of object generation is a common optimization nowadays. The garbage collectors for Java and V8 (the JavaScript engine used in Chrome and Node.js) use a similar concept: young generation / old generation for Java, new-space / old-space for V8.

#### Optimization #2: limiting the number of full collections

The next question is: what triggers garbage collection? Whenever the number of objects in a particular generation gets over a certain threshold (which can be configured through gc.set_threshold()), collection starts. Note that when the GC decides to collect a given generation, it also collects the younger generations (e.g. it would never collect only gen 1 but also gen 0)

As a second optimization, CPython also limits the number of “full” collections, i.e. that are going through all three generations of objects. A full collection is only run when the ratio of objects that have survived the last “non-full” collection (that is, collection of generations 0 or 1 objects) is greater than 25% the number of objects that have survived the last full collection. This 25% ratio is hard-coded and cannot be configured.

当generation 2 占比 generation 0+1 超过25%时, 触发Full GC. hard-code cannot be configured.

#### The danger of finalizers

There is however another issue. In any language, the garbage collector’s worst enemy is finalizers – user-defined methods which are called when the GC wants to collect an object (in Python a finalizer is created by defining the method __del__). What if a finalizer makes one or more objects reachable again?  This is why, up to Python 3.3, container objects inside a circular reference with a finalizer are never garbage-collected.

example:

```
>>> class MyClass(object):
...     def __del__(self):
...             pass
...
>>> my_obj = MyClass()
>>> my_obj.ref = my_obj
>>> my_obj
<__main__.MyClass object at 0x00000000025FCEF0>

>>> del my_obj
>>> gc.collect()
2
>>> gc.garbage
[<__main__.MyClass object at 0x00000000025FCEF0>]
```



