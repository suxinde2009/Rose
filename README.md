Rose<br>
Rose is a corss-platform C++ SDK. It is based on [SDL](http://www.libsdl.org), and provide higher level interface to application. For more information on Rose please browse our [site](http://www.freeors.com).<br>

Rose is distributed under the [zlib license](http://www.gzip.org/zlib/zlib_license.html). This license allows you to use Rose freely in any software.<br>

Rose stands for Request Online SErvice.
Rose is a project derived from "Kingdom Wars", which is a module of "Battle of Wesnoth". Both "Kingdom Wars" and "Battle of Wesnoth" are open source strategy games. With the official release of Rose, Kingdom wars has become an application based on Rose. For now, Rose still has a lot of "Kingdom Wars"-style code, which is being refactored. However, being a mature commercial application in Rose, Kingdom Wars is expected to be a good demonstration in reference to developing with Rose.  

I. What's Rose?
Rose is a cross-platform C/C++ SDK (software development kit). By providing general functions for C/C++ application development, Rose helps developers save time and efforts in writing trivial codes and lets them focus on the major functionalities of the application better.

Basic Module
Window UI. UI is adopted differently in different operating system. Managing
Configuration Processing. Configurations such as Resource package and runtime data are processed by Rose. Rose processes static configurations with WML format, and it is recommended to use Lua to deal with dynamic configurations.
Graphic Processing. Basic features include reading and writing images. In case of that more than thousands of images being processed, Rose provides many tools for optimization. As alpha compositing is needed, Rose's default image format is png.
Audio Output. Typically, Longer audios are soundtracks and shorter are sound effects. Rose's default audio format is ogg.
Video. Functions and cinfigurations are available for video play.
String and formatting. Rose's default encoding is UTF-8. It may convert to UNICODE in case of needed.
Locale. Gettext provides supports multiple languages.
Networking. Rose provides hook functions for network packets handling.

Service Module.
It provides general features as listed below.
Platform. It provide management for player id, player scores, and etc.
Chat. Rose acts as a chat client if there is a chat server. For enabling chatting, in the GUI, developers can just make a "button", which invokes chat window. Rose can also synchronizing user data with online forum( Discuz! X).
Help Text. Rose supports web browsing and local help text formatting. Both ways are choices for help text.
Online Search. Searching application problems in the associated site/forum.
In-App purchase. Supports common payment methods.

II. More about Rose
Rose is based on SDL and provides general API.
Rose is good for Cross-OS programming. Source code does not need to be changed crossing the platform.That is because that no intermediate dll is generated. However, for more than 500 hpp/cpp files, it needs more time for compilation. Visual Studio is recommended for debugging. I personally think Visual Studio is the best C/C++ debugging tool. I spent the most time on coding on Windows OS, however, the code got compiled successfully on the other platforms. Rose provides the framework. Developers can either work with the framework, or start from fresh, using Rose's tools, such as Configuration Processing, Graphic Processing, Networking. The libraries which Rose depends on include SDL、gettext、bzip2、zlib. As SDL depends on libpng、libogg、libfreetype and etc., Rose has already integrated those libraries into its code. Boost's code in integrated into Rose as well. However, among many components that Boost has. Rose only uses "iostreams" and "regex". Other header files of Boost are not included. One can include them manually in case of needed.

V0.0.5<br>
1. Support IRC protocol. It can use as IRC Client<br>

V0.0.2<br>
1. Build new theme mechanism, use controller_base+display+tdialog.<br>
2. Add report widget. It can impletement context menu, multi-segment menu, etc.
