# [MTcpServer](https://github.com/cm-MMK-2/MTcpServer/)
A C++ tcp server library built on top of [libuv](https://github.com/libuv/libuv)(current version:1.20.0)

## Build
Compiler supporting C++ 14 is required.

## Example
Please also check the example.cpp file
```
MTcpServer server;
server.Setup("0.0.0.0", 9000);
//server.SetShortConnection(true);//if you want to close connection after response(like http server)
server.on_new_session = [](MClientSession& session) {
  //new session connected
};
server.on_new_packet = [](MClientSession& session, char * data, ssize_t len) {
  //new message from session received
};
server.on_session_close = [](MClientSession& session) {
  //client disconnected
};
server.Start();
```

## Design
![simple_design](https://github.com/cm-MMK-2/MTcpServer/blob/master/pics/simple_design.png)</br>
</br>
Further explanations:</br>
[English](https://www.bakako.com/build-a-scalable-tcp-server-based-on-libuv)</br>
[简体中文](https://zhuanlan.zhihu.com/p/35358510)

## Test
Tested by using apache bench (short-term http request) and EchoServerTest (long-term connection)

## Author
[cm](https://www.bakako.com)

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details.
The origin libuv project is not included.
