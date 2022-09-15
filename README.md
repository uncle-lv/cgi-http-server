# CGI HTTP Server

![license](https://img.shields.io/github/license/uncle-lv/cgi-http-server)  ![stars](https://img.shields.io/github/stars/uncle-lv/cgi-http-server)  ![issues](https://img.shields.io/github/issues/uncle-lv/cgi-http-server)  ![fork](https://img.shields.io/github/forks/uncle-lv/cgi-http-server)  ![platform](https://img.shields.io/badge/platform-only%20linux-orange)

[Chinese](https://github.com/uncle-lv/cgi-http-server/blob/main/README_zh.md) | [English](https://github.com/uncle-lv/cgi-http-server/blob/main/README.md)

A CGI Web Server which is inspired by tiny-httpd and enhanced with the third-party libraries, *[libev](http://software.schmorp.de/pkg/libev)*, *[http-parser](https://github.com/nodejs/http-parser)*, *[hashmap](https://github.com/tidwall/hashmap.c)*, *[log.c](https://github.com/rxi/log.c)*. It shows you how a web server works briefly.

## Functions

- [x] Parse HTTP request
- [x] Support GET, POST method
- [x] Support CGI
- [x] A simple logger

## Usage

1.Install libev and Python3.
```bash
sudo apt install -y libev-dev python3
```

2.Pull the source code.
```bash
git clone https://github.com/uncle-lv/cgi-http-server.git
```

3.Into the directory `src`, and run `make` to compile the code.
```bash
make
```

4、Run CGI Server (the argument `port` is optional)
```bash
./server {port}
```

<br>

> There are several urls for testing, `/index.html`、`/login.html` and etc.
> 
> You should have the execute permission of CGI scripts.

## Code Structure

```
src
├── hashmap.c
├── hashmap.h
├── http_parser.c
├── http_parser.h
├── http_request.c
├── http_request.h
├── log.c
├── log.h
├── Makefile
├── server
└── server.c
```

If you want to read the source code, please focus on `http_request.*` and `server.c`. The other files are third-party libraries.

[hashmap](https://github.com/tidwall/hashmap.c): Hashmap implementation in C.

[http_parser](https://github.com/nodejs/http-parser): HTTP request/response parser for C.

[log.c](https://github.com/rxi/log.c): A simple logging library implemented in C99.

## Contributions

Looking forward to Any issue or pull request from you.

## Others

**Maybe** implement the whole HTTP/1.0 and CGI/1.1 in the future.

## License

[MIT](https://github.com/uncle-lv/cgi-http-server/blob/main/LICENSE)

## Thanks

- [tiny-httpd](http://tinyhttpd.sourceforge.net)
- [libev](http://software.schmorp.de/pkg/libev)
- [http-parser](https://github.com/nodejs/http-parser)
- [hashmap](https://github.com/tidwall/hashmap.c)
- [bjoern](https://github.com/jonashaag/bjoern)
- [log.c](https://github.com/rxi/log.c)
