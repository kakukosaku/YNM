# Memory Model

## Overview

Ref: https://en.wikipedia.org/wiki/Memory_model_(programming)

In computing, a memory model describes the interactions of threads through memory and their shared use of the data.

- [Java Memory Model](#java-memory-model)
- [Go Memory Model](#go-memory-model)

### Go Memory Model

Ref: https://golang.org/ref/mem


### Java Memory Model

Ref: 

1. http://tutorials.jenkov.com/java-concurrency/java-memory-model.html
2. http://tutorials.jenkov.com/java-concurrency/volatile.html
3. http://tutorials.jenkov.com/java-concurrency/synchronized.html

The Java memory model specifies how the Java virtual machine works with the computer's memory (RAM). 

The Java memory model specifies how and when different threads can see values written to shared variables by other threads, and how to synchronize access to shared variables when necessary.

The Java memory model used internally in the JVM divides memory between thread stacks and the heap.

![thread_stack_and_jvm_heap](http://tutorials.jenkov.com/images/java-concurrency/java-memory-model-1.png)

每个运行中(在JVM)的线程都有自己的线程栈. The thread stack contains information about what methods the thread has called to reach the current point of execution. 也称为call stack. 

The thread stack also contains all local variables for each method being executed (all methods on the call stack). 每个线程仅能访问它自己的线程栈. 线程创建的本地变量仅自己可见. 即使2个线程执行相同的方法, the two threads will still create the local variables of that code in each their own thread stack. Thus, each thread has its own version of each local variable.

原始类型完全存储在线程栈空间内, One thread(A) may pass a copy of a primitive type to another thread(B), primitive local variable 本身并不共享!

堆中存储所有的 object(包括了原始类型对应的包装类型), 无论它在哪创建. 也无论它是否是成员变量或是创建并赋值给local variable. 

![object_in_jvm_heap](http://tutorials.jenkov.com/images/java-concurrency/java-memory-model-2.png)

如果在线程中创建原始类型的包装类型, 那么reference to the wrap object is stored in thread stack; while the wrap object is stored in heap.

An object's member variables are stored on the heap along with the object itself. 无论这个成员变量是primitive type or a reference to an object.

Static class variables are also stored on the heap along with the class definition.

鉴于存储器层次结构, CPU访问主存(如 jvm_heap)上的数据时如图示:

![cpu_access_primary_memory](http://tutorials.jenkov.com/images/java-concurrency/java-memory-model-5.png)

此时会有2个问题发生:

- 线程更新共享对象的可见性 (Visibility of thread updates (writes) to shared variables.)
- 竞态条件在读写共享对象时 (Race conditions when reading, checking and writing shared variables.)

**Visibility of Shared Objects**

如果多个线程共享一个对象, 如果没有适当的 `volatile` 声明或 `synchronized` 使用, 一个线程的更新可能对另一个线程不可见.

由于CPU缓存换出时, 才会将修改写回主存(write back).

> Imagine that the shared object is initially stored in main memory. A thread running on CPU one then reads the shared object into its CPU cache. There it makes a change to the shared object. As long as the CPU cache has not been flushed back to main memory, the changed version of the shared object is not visible to threads running on other CPUs. This way each thread may end up with its own copy of the shared object, each copy sitting in a different CPU cache.

解决方案是使用 `volatile` 关键字

The volatile keyword can make sure that a given variable is read directly from main memory, and always written back to main memory when updated.

**Race Conditions**

如果多个线程共享一个对象, 当多个线程updates variables in that shared object, `race condition` may occur.

2个线程同时在多核CPU中执行 `+1` 操作: 先从主存读至CPU cache, add one to variable, then write back to main memory.

> Imagine if thread A reads the variable count of a shared object into its CPU cache. Imagine too, that thread B does the same, but into a different CPU cache. Now thread A adds one to count, and thread B does the same. Now var1 has been incremented two times, once in each CPU cache.

结果可能是variable 仅+1, 而非+2

To solve this problem you can use a Java synchronized block.

A synchronized block guarantees that only one thread can enter a given critical section of the code at any given time. *Synchronized blocks also guarantee that all variables accessed inside the synchronized block will be read in from main memory, and when the thread exits the synchronized block, all updated variables will be flushed back to main memory again, regardless of whether the variable is declared volatile or not.*

