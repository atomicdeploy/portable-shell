#include "ps_io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET test_socket;
typedef int test_socklen;
#define close_socket closesocket
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int test_socket;
typedef socklen_t test_socklen;
#define close_socket close
#endif
int main(void) {
    PSConfig udp_config = {0}, tcp_config = {0};
    PSUDPContext udp;
    PSSocketContext tcp;
    PSSerialContext serial;
    struct sockaddr_in addr;
    test_socklen addrlen = sizeof(addr);
    test_socket client, listener, peer;
    char buf[32] = {0};
    assert(!ps_io_udp_init(&udp_config, &udp, 0));
    assert(!getsockname((test_socket)udp.socket_fd, (struct sockaddr *)&addr, &addrlen));
    client = socket(AF_INET, SOCK_DGRAM, 0);
    assert(client != (test_socket)-1);
    assert(udp_config.write_str(&udp, "before-peer") == -1);
    assert(sendto(client, "hello", 5, 0, (struct sockaddr *)&addr, addrlen) == 5);
    assert(udp_config.read_char(&udp) == 'h');
    assert(udp_config.read_char(&udp) == 'e');
    assert(!udp_config.write_str(&udp, "reply"));
    assert(recv(client, buf, sizeof(buf), 0) == 5 && !memcmp(buf, "reply", 5));
    /* An oversized datagram must not become a partial command. */
    assert(udp_config.read_char(&udp) == 'l');
    assert(udp_config.read_char(&udp) == 'l');
    assert(udp_config.read_char(&udp) == 'o');
    {
        char oversized[300];
        memset(oversized, 'x', sizeof(oversized));
        assert(sendto(client, oversized, sizeof(oversized), 0, (struct sockaddr *)&addr, addrlen) ==
               sizeof(oversized));
        assert(udp_config.read_char(&udp) == -1);
        assert(sendto(client, "Z", 1, 0, (struct sockaddr *)&addr, addrlen) == 1);
        assert(udp_config.read_char(&udp) == 'Z');
    }
    /* TCP callbacks handle buffered reads and real response bytes. */
    listener = socket(AF_INET, SOCK_STREAM, 0);
    assert(listener != (test_socket)-1);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(!bind(listener, (struct sockaddr *)&addr, sizeof(addr)) && !listen(listener, 1));
    addrlen = sizeof(addr);
    assert(!getsockname(listener, (struct sockaddr *)&addr, &addrlen));
    assert(!ps_io_socket_init(&tcp_config, &tcp, "127.0.0.1", ntohs(addr.sin_port)));
    peer = accept(listener, NULL, NULL);
    assert(peer != (test_socket)-1);
    assert(send(peer, "test", 4, 0) == 4);
    assert(tcp_config.read_char(&tcp) == 't' && tcp_config.read_char(&tcp) == 'e');
    assert(!tcp_config.write_str(&tcp, "reply"));
    assert(recv(peer, buf, sizeof(buf), 0) == 5 && !memcmp(buf, "reply", 5));
    ps_io_socket_close(&tcp);
    ps_io_socket_close(&tcp);
    close_socket(peer);
    close_socket(listener);
    close_socket(client);
    ps_io_udp_close(&udp);
    ps_io_udp_close(&udp);
    assert(ps_io_socket_init(&tcp_config, &tcp, "bad address", 42) == -1 && tcp.socket_fd == -1);
    ps_io_socket_close(&tcp);
    assert(ps_io_serial_init(&tcp_config, &serial, "nonexistent-port-for-test", 115200) == -1);
    ps_io_serial_close(&serial);
    puts("Legacy transports: TCP/UDP loopback, peer reply, invalid address and idempotent close "
         "passed");
    return 0;
}
