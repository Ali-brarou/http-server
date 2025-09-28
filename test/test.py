import socket

HOST = "127.0.0.1"
PORT = 6969

def send_request(sock, path):
    req = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {HOST}\r\n"
        f"Connection: keep-alive\r\n"
        f"\r\n"
    )
    sock.sendall(req.encode("utf-8"))

def recv_response(sock):
    # read until end of headers
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Connection closed by server")
        data += chunk

    headers, rest = data.split(b"\r\n\r\n", 1)
    headers_text = headers.decode("utf-8", errors="replace")
    print("=== Response Headers ===")
    print(headers_text)

    # parse Content-Length (simplest way)
    content_length = 0
    for line in headers_text.split("\r\n"):
        if line.lower().startswith("content-length:"):
            content_length = int(line.split(":", 1)[1].strip())
            break

    # read body
    body = rest
    while len(body) < content_length:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("Connection closed before full body received")
        body += chunk

    print("=== Response Body ===")
    print(body.decode("utf-8", errors="replace"))
    print("=====================")

    return headers_text, body


def main():
    with socket.create_connection((HOST, PORT)) as sock:
        paths = ["/"] * 100000000

        for path in paths:
            print(f"\n>>> Sending request for {path}")
            send_request(sock, path)
            headers, body = recv_response(sock)


if __name__ == "__main__":
    main()

