Cross-Platform C++ SDK

Base module
Service Module

Rose是Requrest Online Service中的四个首字母，翻译成中文是“请求在线服务”，词语含义就是要提供各样在线服务，而这正体现了Rose要实现的核心。
Rose stands for Request Online SErvice.
Rose发源于《王国战争》，后者则源于《韦诺之战》，它们都是开源战棋游戏。伴随Rose发布，《王国战争》成了基于Rose的一个应用。由于两者历史，Rose中不少代码能看《王国战争》影子，这个会随着时间慢慢消除。《王国战争》是基于Rose一个应用，可毕竟已商业化存在一段时间，它的代码对如何使用Rose有着极好的参考价值。
Rose is a project derived from "Kingdom Wars", which is a module of "Battle of Wesnoth". Both "Kingdom Wars" and "Battle of Wesnoth" are open source strategy games. With the official release of Rose, Kingdom wars has become an application based on Rose. For now, Rose still has a lot of "Kingdom Wars"-style code, which is being refactored. However, being a mature commercial application in Rose, Kingdom Wars is expected to be a good demonstration in reference to developing with Rose.  
一、Rose是什么
Rose是个跨平台C/C++ SDK，它向C/C++开发者提供应用须要的通用功能，借用这些功能，把开发者从机械的重复性劳动中解放出来，专注于实现自个应用的专门模块。Rose提供的模块按功能大小分为两种级别：基本模块、服务模块。
Rose is a cross-platform C/C++ SDK (software development kit). By providing general functions for C/C++ application development, Rose helps developers save time and efforts in writing trivial codes and lets them focus on the major functionalities of the application better.

基本模块。它是程序中常用的基本操作。
Basic Module

窗口系统。各个操作系统以不同方式处理窗口，在这方面差别可说是非常大。Rose自提供了一种窗口系统实现，这个系统一个特点是可支持动态变化/自适应各样分辨率（最低480x320）。窗口系统是Rose极重要组件，它几乎占去一半代码。
Window UI. UI is adopted differently in different operating system. Managing
处理配置。应用的配置数据包括发布时自带的资源包和运行时生成的数据，Rose以WML格式统一处理静态配置，动态配置推荐使用Lua。
Configuration Processing. Configurations such as Resource package and runtime data are processed by Rose. Rose processes static configurations with WML format, and it is recommended to use Lua to deal with dynamic configurations.
处理图像。除了基本读、写图像操作，有些应用可能要处理数千图像，这时为提高效率需用内存换速度，Rose提供了种管理众多图像方法。由于要使用Alpha分量，Rose默认的图像压缩格式是png。
Graphic Processing. Basic features include reading and writing images. In case of that more than thousands of images being processed, Rose provides many tools for optimization. As alpha compositing is needed, Rose's default image format is png.
播放声音。按声音持续时间长短，Rose把声音分为音乐、音效。Rose默认的声音压缩格式是ogg。
Audio Output. Typically, Longer audios are soundtracks and shorter are sound effects. Rose's default audio format is ogg.
播放动画。开发者只要编写动画配置，然后调用几个函数便可在应用播放动画。
Video. Functions and cinfigurations are available for video play.
字符串及图文混排。Rose内部基本是以UTF-8格式处理字符，有须要才转换为UNICODE去处理。
String and formatting. Rose's default encoding is UTF-8. It may convert to UNICODE in case of needed.
多国语言。Rose内置了gettext去支持多国语言。
Locale. Gettext provides supports multiple languages.
网络传输。应用提供数据处理函数，它们以“钩子”形式融入进Rose提供的机制。
Networking. Rose provides hook functions for network packets handling.

服务模块。它针对于一般程序都须要、其功能又大同小异的应用。（基本模块都已有初步实现，服务模块则待实现，写在此处是为全面。）
Service Module. It provides general features as listed below.

平台特性。提供和平台相关的功能，像统一ID、统一计算积分。
Platform. It provide management for player id, player scores, and etc.
聊天。网上有一个聊天服务器，Rose去连这个服务器实现聊天。为支持聊天，开发者要做的就是在界面中给出一可弹出聊天窗口的“按钮”，接下Rose就会去处理所有细节。
访问WEB。应用一般有WEB网站、论坛。Rose会去连应用指定的网站、论坛，得到应用信息显示到用户，有论坛的话，Rose可做到读写论坛，让应用和论坛互访数据。
Chat. Rose acts as a chat client if there is a chat server. For enabling chatting, in the GUI, developers can just make a "button", which invokes chat window. Rose can also synchronizing user data with online forum( Discuz! X).
帮助。WEB服务已属于帮助，除去WEB访问，Rose提供统一的书籍格式。
Help Text. Rose supports web browsing and local help text formatting. Both ways are choices for help text.
在线搜索。像针对应用问题在相关网站、论坛搜索。
Online Search. Searching application problems in the associated site/forum.
内购。提供对常见支付平台的支持。
In-App purchase. Supports common payment methods.


二、Rose特点

基于SDL，封装常用功能，向上提供更接近应用的接口。
支持常用平台，应用只须写一份代码就可实现“完全”跨平台。
不是以二进制（像dll）而是以源码的形式被链入最终工程。
Rose库有近500个hpp/cpp文件。相比于二进制，源码形式在第一次编译时会多消耗时间。
主推Visual Studio调试器，其它调试器只是辅助，像iOS的XCode。个人认为Visual Studio是最强C/C++调试器，应用主要的开发时间要花在Widnows平台，一旦Window平台通过，其它平台基本就已通过。
提供了编写框架，程序员可基于它编写整个应用架构。也可以放弃这个框架，只是把Rose当工具箱，从中调用些自个需要的功能，像处理配置、读写图像、网络机制。
涉及到开源项目包括SDL、gettext、bzip2、zlib；SDL自身则涉及到libpng、libogg、libfreetype。Rose包含了这些项目源码。
Boost以源码形式被融入进Rose，对于有cpp实现的Boost组件，Rose只使用了“iostreams”、“regex”。考虑到Boost的大量头文件会增大Rose容量，官方提供的Rose只包括了本SDK须要部分，如果自个喜欢组件没有被包括，把它们加入到相应位置即可。
