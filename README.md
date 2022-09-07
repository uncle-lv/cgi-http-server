# CGI HTTP Server

![license](https://img.shields.io/github/license/uncle-lv/cgi-http-server)  ![stars](https://img.shields.io/github/stars/uncle-lv/cgi-http-server)  ![issues](https://img.shields.io/github/issues/uncle-lv/cgi-http-server)  ![fork](https://img.shields.io/github/forks/uncle-lv/cgi-http-server)

一个参考自tiny-httpd、使用了[libev](http://software.schmorp.de/pkg/libev)、[http-parser](https://github.com/nodejs/http-parser)、[hashmap](https://github.com/tidwall/hashmap.c)、[log.c](https://github.com/rxi/log.c)等第三方库进行了功能增强的CGI Web Server。它可以帮助你大致地了解Web Server的工作原理。

## 已实现的功能

- [x] HTTP请求的解析与封装
- [x] GET、POST请求处理
- [x] 支持CGI协议
- [x] 日志输出

## 使用

1.安装libev
```bash
sudo apt install libev-dev
```

2.将项目代码拉取到本地
```bash
git clone https://github.com/uncle-lv/cgi-http-server.git
```

3.进入`src`目录，使用`make`命令构建项目
```bash
make
```

4、运行服务器（端口号是可选参数）
```bash
./server {port}
```

<br>

> 与tiny-httpd类似，cgi http server也提供了`/index.html`、`/login.html`等几个url路由供使用者测试。

## 贡献

期待来自你的issue或pull request。

## 其他

**也许**会在后续计划中实现完整的HTTP/1.0与CGI/1.1协议。

## 协议

[MIT](https://github.com/uncle-lv/cgi-http-server/blob/main/LICENSE)

## 鸣谢

- [tiny-httpd](http://tinyhttpd.sourceforge.net)
- [libev](http://software.schmorp.de/pkg/libev)
- [http-parser](https://github.com/nodejs/http-parser)
- [hashmap](https://github.com/tidwall/hashmap.c)
- [bjoern](https://github.com/jonashaag/bjoern)
- [log.c](https://github.com/rxi/log.c)

