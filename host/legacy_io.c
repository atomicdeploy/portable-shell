/* Optional native transports for the legacy callbacks. No OS calls in core.
   Socket calls have bounded timeouts. UDP pins the first peer per context:
   never share a shell's command state between unrelated datagram senders. */
#define _POSIX_C_SOURCE 200809L
#include "ps_io.h"
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET socket_type;
typedef int address_length;
static int network_start(void) {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data);
}
static void network_end(void) {
    WSACleanup();
}
static void socket_close(socket_type fd) {
    closesocket(fd);
}
static int interrupted(void) {
    return WSAGetLastError() == WSAEINTR;
}
#else
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int socket_type;
typedef socklen_t address_length;
static int network_start(void) {
    return 0;
}
static void network_end(void) {}
static void socket_close(socket_type fd) {
    close(fd);
}
static int interrupted(void) {
    return errno == EINTR;
}
#endif
static int socket_ready(socket_type fd) {
    int rc;
#ifdef _WIN32
    DWORD timeout = 2000;
#else
    struct timeval timeout = {2, 0};
#endif
    rc = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    rc |= setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
#ifdef SO_NOSIGPIPE
    {
        int yes = 1;
        rc |= setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
    }
#endif
    return rc;
}
static int bounded_connect(socket_type fd, const struct sockaddr_in *addr) {
    int result, socket_error = 0;
    address_length error_size = sizeof(socket_error);
    fd_set writable;
    struct timeval wait = {2, 0};
#ifdef _WIN32
    u_long nonblocking = 1;
    if (ioctlsocket(fd, FIONBIO, &nonblocking))
        return -1;
#else
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fd >= FD_SETSIZE || fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        return -1;
#endif
    result = connect(fd, (const struct sockaddr *)addr, sizeof(*addr));
    if (result) {
#ifdef _WIN32
        int pending = WSAGetLastError() == WSAEWOULDBLOCK;
#else
        int pending = errno == EINPROGRESS;
#endif
        if (pending) {
            FD_ZERO(&writable);
            FD_SET(fd, &writable);
            result = select((int)fd + 1, NULL, &writable, NULL, &wait);
            result =
                result == 1 &&
                        !getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&socket_error, &error_size) &&
                        socket_error == 0
                    ? 0
                    : -1;
        }
    }
#ifdef _WIN32
    nonblocking = 0;
    if (ioctlsocket(fd, FIONBIO, &nonblocking))
        result = -1;
#else
    if (fcntl(fd, F_SETFL, flags))
        result = -1;
#endif
    return result;
}
static int send_bytes(intptr_t fd, const char *p, size_t n, int datagram) {
    unsigned retries = 0;
    int flags = 0;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    if (fd == -1 || n > INT_MAX)
        return -1;
    while (n) {
        int k = (int)send((socket_type)fd, p, (int)n, flags);
        if (k < 0 && interrupted() && retries++ < 8)
            continue;
        if (k <= 0 || (datagram && (size_t)k != n))
            return -1;
        p += k;
        n -= (size_t)k;
    }
    return 0;
}
static int socket_read_char(void *ctx) {
    PSSocketContext *c = (PSSocketContext *)ctx;
    unsigned retries = 0;
    if (c->socket_fd == -1)
        return -1;
    if (c->read_pos >= c->read_len) {
        do {
            c->read_len =
                (int)recv((socket_type)c->socket_fd, c->read_buffer, sizeof(c->read_buffer), 0);
        } while (c->read_len < 0 && interrupted() && retries++ < 8);
        c->read_pos = 0;
        if (c->read_len <= 0)
            return -1;
    }
    return (unsigned char)c->read_buffer[c->read_pos++];
}
static int socket_write_char(void *ctx, char c) {
    return send_bytes(((PSSocketContext *)ctx)->socket_fd, &c, 1, 0);
}
static int socket_write_str(void *ctx, const char *s) {
    return send_bytes(((PSSocketContext *)ctx)->socket_fd, s, strlen(s), 0);
}
int ps_io_socket_init(PSConfig *config, PSSocketContext *ctx, const char *host, int port) {
    struct sockaddr_in addr;
    if (!config || !ctx)
        return -1;
    ctx->socket_fd = -1;
    ctx->read_pos = ctx->read_len = 0;
    if (!host || port < 1 || port > 65535)
        return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1 || network_start())
        return -1;
    ctx->socket_fd = (intptr_t)socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->socket_fd == -1) {
        network_end();
        return -1;
    }
    if (socket_ready((socket_type)ctx->socket_fd) ||
        bounded_connect((socket_type)ctx->socket_fd, &addr)) {
        ps_io_socket_close(ctx);
        return -1;
    }
    config->read_char = socket_read_char;
    config->write_char = socket_write_char;
    config->write_str = socket_write_str;
    config->io_ctx = ctx;
    return 0;
}
void ps_io_socket_close(PSSocketContext *ctx) {
    if (ctx && ctx->socket_fd != -1) {
        socket_close((socket_type)ctx->socket_fd);
        ctx->socket_fd = -1;
        network_end();
    }
}
static int udp_read_char(void *context) {
    PSUDPContext *c = (PSUDPContext *)context;
    struct sockaddr_in peer;
    address_length len = sizeof(peer);
    unsigned retries = 0;
    if (c->socket_fd == -1)
        return -1;
    if (c->read_pos >= c->read_len) {
        do {
#ifdef _WIN32
            c->read_len = (int)recvfrom((socket_type)c->socket_fd, c->read_buffer,
                                        sizeof(c->read_buffer), 0, (struct sockaddr *)&peer, &len);
#else
            struct iovec buffer = {c->read_buffer, sizeof(c->read_buffer)};
            struct msghdr message;
            memset(&message, 0, sizeof(message));
            message.msg_name = &peer;
            message.msg_namelen = sizeof(peer);
            message.msg_iov = &buffer;
            message.msg_iovlen = 1;
            c->read_len = (int)recvmsg((socket_type)c->socket_fd, &message, 0);
            len = message.msg_namelen;
            if (c->read_len >= 0 && (message.msg_flags & MSG_TRUNC)) {
                c->read_len = 0;
                return -1;
            }
#endif
        } while (c->read_len < 0 && interrupted() && retries++ < 8);
        c->read_pos = 0;
        if (c->read_len <= 0)
            return -1;
        if (!c->peer_connected) {
            if (connect((socket_type)c->socket_fd, (struct sockaddr *)&peer, len))
                return -1;
            c->peer_connected = true;
        }
    }
    return (unsigned char)c->read_buffer[c->read_pos++];
}
static int udp_write_char(void *ctx, char c) {
    PSUDPContext *p = (PSUDPContext *)ctx;
    return p->peer_connected ? send_bytes(p->socket_fd, &c, 1, 1) : -1;
}
static int udp_write_str(void *ctx, const char *s) {
    PSUDPContext *p = (PSUDPContext *)ctx;
    return p->peer_connected ? send_bytes(p->socket_fd, s, strlen(s), 1) : -1;
}
int ps_io_udp_init(PSConfig *config, PSUDPContext *ctx, int port) {
    struct sockaddr_in addr;
    if (!config || !ctx)
        return -1;
    ctx->socket_fd = -1;
    ctx->read_pos = ctx->read_len = 0;
    ctx->peer_connected = false;
    if (port < 0 || port > 65535 || network_start())
        return -1;
    ctx->socket_fd = (intptr_t)socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->socket_fd == -1) {
        network_end();
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (socket_ready((socket_type)ctx->socket_fd) ||
        bind((socket_type)ctx->socket_fd, (struct sockaddr *)&addr, sizeof(addr))) {
        ps_io_udp_close(ctx);
        return -1;
    }
    config->read_char = udp_read_char;
    config->write_char = udp_write_char;
    config->write_str = udp_write_str;
    config->io_ctx = ctx;
    return 0;
}
void ps_io_udp_close(PSUDPContext *ctx) {
    if (ctx && ctx->socket_fd != -1) {
        socket_close((socket_type)ctx->socket_fd);
        ctx->socket_fd = -1;
        ctx->peer_connected = false;
        network_end();
    }
}
static int serial_read(void *context) {
    PSSerialContext *c = (PSSerialContext *)context;
    unsigned char byte;
#ifdef _WIN32
    DWORD count = 0;
    return c->serial_handle && ReadFile((HANDLE)c->serial_handle, &byte, 1, &count, NULL) &&
                   count == 1
               ? byte
               : -1;
#else
    struct pollfd p = {(int)(intptr_t)c->serial_handle - 1, POLLIN, 0};
    return c->serial_handle && poll(&p, 1, 2000) == 1 && read(p.fd, &byte, 1) == 1 ? byte : -1;
#endif
}
static int serial_char(void *context, char byte) {
    PSSerialContext *c = (PSSerialContext *)context;
#ifdef _WIN32
    DWORD count = 0;
    return c->serial_handle && WriteFile((HANDLE)c->serial_handle, &byte, 1, &count, NULL) &&
                   count == 1
               ? 0
               : -1;
#else
    struct pollfd p = {(int)(intptr_t)c->serial_handle - 1, POLLOUT, 0};
    return c->serial_handle && poll(&p, 1, 2000) == 1 && write(p.fd, &byte, 1) == 1 ? 0 : -1;
#endif
}
static int serial_string(void *ctx, const char *s) {
    while (*s)
        if (serial_char(ctx, *s++) < 0)
            return -1;
    return 0;
}
int ps_io_serial_init(PSConfig *config, PSSerialContext *ctx, const char *port, int baud) {
    if (!config || !ctx)
        return -1;
    ctx->serial_handle = NULL;
    if (!port || baud <= 0)
        return -1;
#ifdef _WIN32
    {
        DCB dcb = {0};
        COMMTIMEOUTS times = {0};
        HANDLE h = CreateFileA(port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE)
            return -1;
        dcb.DCBlength = sizeof(dcb);
        if (!GetCommState(h, &dcb)) {
            CloseHandle(h);
            return -1;
        }
        dcb.BaudRate = (DWORD)baud;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = TRUE;
        dcb.fParity = FALSE;
        dcb.fOutxCtsFlow = FALSE;
        dcb.fOutxDsrFlow = FALSE;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutX = FALSE;
        dcb.fInX = FALSE;
        times.ReadIntervalTimeout = MAXDWORD;
        times.ReadTotalTimeoutConstant = 2000;
        times.WriteTotalTimeoutConstant = 2000;
        if (!SetCommState(h, &dcb) || !SetCommTimeouts(h, &times)) {
            CloseHandle(h);
            return -1;
        }
        ctx->serial_handle = h;
    }
#else
    {
        int fd;
        struct termios t;
        speed_t speed;
        switch (baud) {
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        default:
            return -1;
        }
        fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd < 0)
            return -1;
        if (tcgetattr(fd, &t)) {
            close(fd);
            return -1;
        }
        t.c_iflag = 0;
        t.c_oflag = 0;
        t.c_lflag = 0;
        t.c_cflag = CS8 | CREAD | CLOCAL;
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 20;
        cfsetispeed(&t, speed);
        cfsetospeed(&t, speed);
        if (tcsetattr(fd, TCSANOW, &t)) {
            close(fd);
            return -1;
        }
        ctx->serial_handle = (void *)(intptr_t)(fd + 1);
    }
#endif
    ctx->baud_rate = baud;
    config->io_ctx = ctx;
    config->read_char = serial_read;
    config->write_char = serial_char;
    config->write_str = serial_string;
    return 0;
}
void ps_io_serial_close(PSSerialContext *ctx) {
    if (!ctx || !ctx->serial_handle)
        return;
#ifdef _WIN32
    CloseHandle((HANDLE)ctx->serial_handle);
#else
    close((int)(intptr_t)ctx->serial_handle - 1);
#endif
    ctx->serial_handle = NULL;
}
