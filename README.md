# Loom - Lightweight HTTP Server in C

Loom is a lightweight and efficient HTTP server written in C, leveraging Linux's epoll for high-performance asynchronous I/O. Designed to handle multiple clients concurrently, it supports HTTP pipelining and is suitable for learning, experimentation, or as a minimal base for custom web server projects.

---

## Features

- **Event-driven**: Uses epoll for scalable multiplexing of client connections.
- **HTTP/1.1 support**: Handles standard HTTP requests and pipelined connections.
- **Custom routing**: Easily register custom handlers for different paths and HTTP methods.
- **Static file serving**: Built-in helper to serve static files.
- **Customizable responses**: Easily set status codes, headers, and body content.
- **Graceful shutdown**: Signal handling for clean server termination.
- **Simple configuration**: Specify host, port, backlog, and other options via command line.

---

## Getting Started

### Requirements

- Linux-based OS with epoll support
- GCC or Clang (C Compiler)

### Building the Library

Clone the repository and build the static library with:

```bash
git clone https://github.com/Ali-brarou/loom.git
cd loom
make         # Builds the library (lib/libloom.a) in release mode by default
```

For a debug build:

```bash
make debug
```

To clean build artifacts:

```bash
make clean
```

### Installing the Library

Install the library and headers to your system (default prefix is `/usr/local`):

```bash
sudo make install
```

Or specify a custom installation prefix:

```bash
sudo make PREFIX=/your/custom/prefix install
```

This will:
- Copy `lib/libloom.a` to `$PREFIX/lib`
- Copy all headers from `include/` to `$PREFIX/include`

---

## Usage

After building and installing, link your application against `-lloom` and include the headers from the installed path.

---

## Example: Custom HTTP Handler

You can define custom route handlers like this:

```c
#define BODY "<html><head><title>Loom works!</title></head><body>Hello, browser!</body></html>"

Http_handler_result_t handler(Http_request_t* req, Http_response_t* resp) {
    resp->status_code = HTTP_OK;
    resp->content_type = HTTP_CONTENT_TEXT_HTML;
    resp->body = BODY;
    resp->body_len = strlen(BODY);
    resp->body_mem = HTTP_MEM_STATIC;

    resp->headers[0].key = "X-Test";
    resp->headers[0].value = "test";
    resp->headers_count = 1;

    // Optionally close connection after response
    char* connection = http_request_search_header(req, "connection");
    resp->connection_close = !connection || strcasecmp(connection, "keep-alive");

    return HTTP_HANDLER_OK;
}
```

Register the handler for a route:

```c
http_route_register(&router, HTTP_METHOD_GET, "/", handler);
```

---

## Static File Handler

Serve static files with:

```c
Http_handler_result_t http_handler_static_file(Http_request_t* req, Http_response_t* resp, const char* file_path, Http_content_type_t content_type);
```

Or use the macro for defining static handlers:

```c
DEFINE_STATIC_HANDLER(my_handler, "/path/to/file.html", HTTP_CONTENT_TEXT_HTML)
```

---

## Architecture Overview

- `server/` - Core server logic (epoll loop, connection handling)
- `include/loom/` - Public API and internal data structures
- `test/` - Example server entry point and basic test routes

---

## Status

This project is under active development. Contributions, feedback, and bug reports are welcome!

---

## License

LGPL

---

## Author

Ali Brarou
