# Garbage Collection, GC

## Overview

Understanding how GC works can help you write better and faster programs.

### 基础知识

对于"不可达数据(对象/内存)"的回收, 称为GC. 问题在于如何寻找"不可达对象", 主要有以下2种基本方法:

1. 捕获可达对象转变为不可达对象的时刻.

典型方法: 引用计数

2. 周期性的定位出所有可达对象, 其它即为不可达对象.

典型方法: 基于跟踪的方法. 具体如下: (语言无关的分类说明)

四种状态的说明: free(空闲), unreached(未被访问), un-scanned(待扫描), scanned(已扫描)

a. 基本的标记-清扫式回收器 (在标记了所有可达对象后, 要扫描堆区所有对象, 释放不可达对象)  
b. Baker的标记-清扫式回收器 (对 a. 方法优化, 多记录了已分配的对象, 与可达对象取补, 释放不可达对象. 避免了扫描整个堆区)  
c. 标记并压缩回收器 (进一步优化, 在标记完成后, 扫描堆区将可达对象移至一头. 保留了时空的局部特性; 解决了内存碎片化问题)  
d. 拷贝回收 (再进一步优化, 将内存分A,B区. 标记后, 将可达对象复制到另一个区, 然后2区交换使用)  

另外, 为解决GC时, STW(stop the world, 长时间的停顿GC)问题. 又出现了短停顿GC, 主要有以下2种基本方法:

1. 按时间来分割任务, GC与增变者(正常的程序运行)交替进行, 称为增量式回收.

可达集合会随着程序运行发生变化, 这种方法较复杂.

2. 按空间来分割任务, 每次只完成一部分垃圾的回收, 称为部分回收.

典型方法:

a. 世代垃圾回收 (根据已分配对象时间的长短来划分对象, 并频繁回收新创建的对象)  
b. 列车算法 (对世代垃圾回收需要进行全面的垃圾回收, 才能解决不同世代间的循环引用问题的优化)  

Language agnostic

- Reference Counting
- Generational gc
- Mark and Sweep Algorithm

Language Specific:

- [Java GC](#java-gc)
- [Ptyhon GC](#python-gc)

Ref:

1. https://en.wikipedia.org/wiki/Garbage_collection_(computer_science)
2. https://en.wikipedia.org/wiki/Reference_counting
3. https://www.geeksforgeeks.org/mark-and-sweep-garbage-collection-algorithm/

### Java GC

Ref:

1. https://www.artima.com/insidejvm/ed2/gc.html
2. Understanding the JVM, Advance Feature and Best Practices. 3th

Garbage collection algorithm must do two basic things:

1. detect garbage objects.
2. reclaim the heap space used by the garbage objects and make the space available again to the program.

发现garbage objects一般通过define a set of roots and determining reachability from the roots.

GC root一般包括本地变量表, 栈帧操作数以及any object references in any class variables. 此外还有:

Another source of roots are any object references, such as strings, in the constant pool of loaded classes. The constant pool of a loaded class may refer to strings stored on the heap, such as the class name, superclass name, superinterface names, field names, field signatures, method names, and method signatures. 

Another source of roots may be any object references that were passed to native methods that either haven't been "released" by the native method.

2个基本的区分live objects from garbage的方法是reference counting(也称: 直接垃圾收集) & tracing(也称: 间接垃圾收集)

JVM的主流实现均使用以"分代回收"理论为基础的 tracing 垃圾回收算法. 分代回收基于以下二个假设:

- 弱分代假说: 绝大多数对象"朝生夕死"

导出设计方案: 每次回收仅关注如何保留少量存活而不标记大量将要回收的对象, GC Root出发的可达性发析

- 强分代假说: 熬过越多次垃圾收集过程的对象就越难消亡

导出方案: 以较低频率回收老年代对象

以及为解决分代之前引用带来的对老年代频繁GC的假说:

- 跨代引用假说: 跨代引用相对于同代引用来说仅占极少数

在新生代中建立的全局记录数据结构(记忆集), 将老年代划分为若干小块, 标识出老年代的哪一块内存会存在跨代引用. 并在GC时, 将其加入GC Root进行扫描.

在分代回收中, 具体的tracing algorithm有:

- tracing collectors:

mark and sweep

In the mark phase, the garbage collector traverses the tree of references and marks each object it encounters. In the sweep phase, unmarked objects are freed, and the resulting memory is made available to the executing program.

标记交换算法会引用内存碎片化, 在申领大对象时无连续的内存导致OOM(out of memory).

改进: Two strategies commonly used by mark and sweep collectors are compacting and copying. 都通过移动对象达到减少内存碎片化. 

- compacting collectors:
 
移动到一端堆内存的一端

> Compacting collectors slide live objects over free memory space toward one end of the heap. 

- copying collectors: 

不需要mark&sweep, 将活跃对象直接复制到额外单独的区域, s0, s1区的使用.

### Python GC

Ref:

1. https://rushter.com/blog/python-memory-managment/
2. https://rushter.com/blog/python-garbage-collector/
3. https://stackify.com/python-garbage-collection/
4. https://pythoninternal.wordpress.com/2014/08/04/the-garbage-collector/
5. https://www.quora.com/How-does-garbage-collection-in-Python-work-What-are-the-pros-and-cons#

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

![step2](https://pythoninternal.files.wordpress.com/2014/07/python-cyclic-gc-2-new-page.png)

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
