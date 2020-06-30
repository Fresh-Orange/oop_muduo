## 项目简介

**oop_muduo是linux下基于Reactor模式的多线程高并发网络库**。 应用层实现了基础的echo服务和简单的http服务用于对该网络库的测试。

本项目参考了陈硕的muduo库，并用面向对象编程思想（oop，object oriented programming）改写而成。陈硕的muduo库是基于对象的而非面向对象的，其大量使用回调的方法。陈硕在其《Linux多线程服务器编程》中阐述了基于对象的好处以及使用虚函数作为公开接口的坏处。虚函数作为公开接口会导致动态库的二进制兼容问题，这一观点我很认同，但是我认为**除了对外接口以外，库的内部完全可以用面向对象方式实现，这样的实现方法比大量回调的方法逻辑上会更清晰，更容易理解**（因为我当时看muduo库源码时看得实在是烦躁，但这不妨碍我对作者的崇拜）。

对muduo库的面向对象设计还参考了[muduo库的源代码分析2--简化方案](https://blog.csdn.net/adkada1/article/details/54893541)中的设计方案和部分实现方法，在此表示感谢。



## 技术原理

- **采用reactor模式**。reactor事件收集器是使用IO多路复用，具体是epoll对多个文件描述符进行监听。一个eventloop对象负责管理poll，以及提供描述符的事件注册，它运行时循环调用epoll的wait监听事件，当有事件返回时，逐个调用事件对应的回调函数，这就实现了reactor的事件分发机制。
- **支持单线程reactor模式**。即监听socket和已连接socket都在同一线程，注册在同一个eventloop当中
- **支持reactor线程池模式** 。采用 one eventloop per thread + 线程池 的多线程模型。监听socket在主线程，由一个eventloop管理。另外创建一个固定线程数的reactor线程池，里面的每个线程都有自己的eventloop对象。 当监听socket的线程得到一个已连接socket时，就用round-robin的形式从线程池选一个线程，唤醒该线程并让其把这个已连接socket注册到该线程的eventloop中。
- **reactor线程池+计算线程池模式**。【待实现】



## 核心类及类间关系





## 压力测试

