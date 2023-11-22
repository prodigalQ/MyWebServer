# MyWebServer
linux环境下用C++11实现的轻量级web服务器，目前实现了主干的网络通讯与http解析以及线程池和缓冲区部分，后续可添加日志、定时器、mysql连接等功能

# 项目结构说明
- bin下为项目生成的可执行文件
- build下为cmake生成的makefile和其他一些配置信息
- include下为头文件.h
- src下为源文件.cpp
- resources下为测试用的网页资源

# 使用方法
- 在根目录下执行cmake
- 进入build目录下执行make
- 进入bin目录下执行生成的可执行文件启动服务器
- 一些配置参数可在main.cpp中修改

# 参考
[C++ Linux WebServer服务器](https://github.com/markparticle/WebServer/) by markparticle
