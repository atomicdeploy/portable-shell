/* Optional POSIX byte transport; OS calls are confined to this file.
   No third-party libraries. Core remains freestanding. */
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE 1
#endif
#define _POSIX_C_SOURCE 200809L
#include "portable_shell.h"
#include "portable_shell_device.h"
#include "process.h"
static unsigned use_device_commands;
static ps_device_profile profiles[4];
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <time.h>
#include "filesystem.h"
#define CLIENTS 4
#define QUEUE 131072
typedef struct {
    int fd;
    unsigned used, failed;
    ps_byte queue[QUEUE];
} Channel;
static Channel channels[CLIENTS];
static union {
    void *alignment;
    ps_byte data[40000];
} memory[CLIENTS];
static ps_session *sessions[CLIENTS];
static volatile sig_atomic_t stopping;
static unsigned length(const char *p) {
    unsigned n = 0;
    while (p[n])
        ++n;
    return n;
}
static int equal(const char *a, const char *b) {
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return *a == *b;
}
static int number(const char *p, unsigned *n) {
    *n = 0;
    if (!*p)
        return 0;
    while (*p) {
        unsigned d = (unsigned)(*p++ - '0');
        if (d > 9 || *n > (4294967295u - d) / 10)
            return 0;
        *n = *n * 10 + d;
    }
    return 1;
}
static void say(const char *p) {
    unsigned n = length(p);
    while (n) {
        ssize_t k = write(2, p, n);
        if (k <= 0)
            break;
        p += k;
        n -= (unsigned)k;
    }
}
static ps_u32 clock_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (ps_u32)t.tv_sec * 1000u + (ps_u32)(t.tv_nsec / 1000000);
}
static void flush(Channel *c) {
    unsigned done = 0;
    while (done < c->used && !c->failed) {
        ssize_t n = write(c->fd, c->queue + done, c->used - done);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            c->failed = 1;
            break;
        }
        if (!n) {
            c->failed = 1;
            break;
        }
        done += (unsigned)n;
    }
    if (done) {
        unsigned i;
        for (i = done; i < c->used; ++i)
            c->queue[i - done] = c->queue[i];
        c->used -= done;
    }
}
static void tx(void *ctx, const ps_byte *p, unsigned n) {
    Channel *c = ctx;
    while (n-- && !c->failed) {
        if (c->used == QUEUE)
            flush(c);
        if (c->used == QUEUE) {
            c->failed = 1;
            return;
        }
        c->queue[c->used++] = *p++;
    }
}
static void stop(int sig) {
    (void)sig;
    stopping = 1;
}
static void resize(ps_session *s, int fd) {
    struct winsize w;
    if (ioctl(fd, TIOCGWINSZ, &w) == 0 && w.ws_col && w.ws_row)
        ps_resize(s, w.ws_col, w.ws_row);
}
static int address(const char *p, unsigned *result) {
    unsigned part = 0, value = 0, count = 0;
    for (;;) {
        if (*p >= '0' && *p <= '9') {
            part = part * 10 + (unsigned)(*p - '0');
            if (part > 255)
                return 0;
        } else if (*p == '.' || !*p) {
            value = (value << 8) | part;
            part = 0;
            ++count;
            if (!*p)
                break;
        } else
            return 0;
        ++p;
    }
    if (count != 4)
        return 0;
    *result = value;
    return 1;
}
static int server(unsigned port, const char *host, unsigned flags) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    unsigned ip, i;
    struct sockaddr_in addr = {0};
    ps_u32 start = clock_ms();
    if (listener < 0 || !address(host, &ip))
        return 1;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(ip);
    if (bind(listener, (struct sockaddr *)&addr, sizeof addr) || listen(listener, 4)) {
        close(listener);
        say("Cannot bind listener\n");
        return 1;
    }
    fcntl(listener, F_SETFL, O_NONBLOCK);
    say("PortableShell Telnet listening on ");
    say(host);
    say("\n");
    while (!stopping) {
        struct pollfd fds[CLIENTS + 1];
        fds[0].fd = listener;
        fds[0].events = POLLIN;
        for (i = 0; i < CLIENTS; ++i) {
            fds[i + 1].fd = sessions[i] ? channels[i].fd : -1;
            fds[i + 1].events = POLLIN | (channels[i].used ? POLLOUT : 0);
        }
        if (poll(fds, CLIENTS + 1, 20) < 0 && errno != EINTR)
            break;
        if (fds[0].revents & POLLIN) {
            int fd = accept(listener, 0, 0);
            if (fd >= 0) {
                for (i = 0; i < CLIENTS && sessions[i]; ++i) {
                }
                if (i == CLIENTS)
                    close(fd);
                else {
                    ps_config cfg = {tx, &channels[i], 100, 28, flags | PS_INTERACTIVE | PS_TELNET};
                    channels[i].fd = fd;
                    channels[i].used = channels[i].failed = 0;
                    fcntl(fd, F_SETFL, O_NONBLOCK);
                    sessions[i] = ps_init(memory[i].data, sizeof memory[i].data, &cfg);
                    if (use_device_commands)
                        ps_device_attach(sessions[i], &profiles[i], 0, 0);
                    ps_start(sessions[i]);
                }
            }
        }
        for (i = 0; i < CLIENTS; ++i)
            if (sessions[i]) {
                if (fds[i + 1].revents & (POLLIN | POLLHUP | POLLERR)) {
                    ps_byte b[2048];
                    ssize_t n = read(channels[i].fd, b, sizeof b);
                    if (n > 0)
                        ps_feed(sessions[i], b, (unsigned)n);
                    else if (!n || (errno != EAGAIN && errno != EINTR))
                        channels[i].failed = 1;
                }
                ps_tick(sessions[i], clock_ms() - start);
                flush(&channels[i]);
                if (channels[i].failed || (ps_exited(sessions[i]) && !channels[i].used)) {
                    close(channels[i].fd);
                    sessions[i] = 0;
                }
            }
    }
    for (i = 0; i < CLIENTS; ++i)
        if (sessions[i])
            close(channels[i].fd);
    close(listener);
    return 0;
}
static void history_load(ps_session *s, const char *path) {
    int fd = open(path, O_RDONLY);
    char b, line[PS_LINE];
    unsigned n = 0;
    if (fd < 0)
        return;
    while (read(fd, &b, 1) == 1) {
        if (b == '\n') {
            line[n] = 0;
            ps_history_add(s, line);
            n = 0;
        } else if (b != '\r' && n + 1 < PS_LINE)
            line[n++] = b;
    }
    if (n) {
        line[n] = 0;
        ps_history_add(s, line);
    }
    close(fd);
}
static void history_save(ps_session *s, const char *path) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    unsigned i;
    if (fd < 0)
        return;
    for (i = 0; i < ps_history_count(s); ++i) {
        const char *p = ps_history_get(s, i);
        unsigned n = length(p);
        while (n) {
            ssize_t k = write(fd, p, n);
            if (k <= 0)
                break;
            p += k;
            n -= (unsigned)k;
        }
        (void)write(fd, "\n", 1);
    }
    close(fd);
}
#if PS_HOST_EXEC
#include "execution.h"
#endif
int main(int argc, char **argv) {
    unsigned flags = PS_COLOR | PS_UNICODE | PS_TRUECOLOR, port = 0, baud = 115200, board = 0,
             execution = PS_HOST_EXEC, batch = 0, force = 0;
    int i, command_at = 0, fd = 0, outfd = 1, raw = 0, result;
    const char *command = 0, *host = "127.0.0.1", *serial = 0, *history = 0;
    struct termios saved;
    ps_config cfg;
    ps_session *s;
    ps_u32 start;
    unsigned oldw = 0, oldh = 0;
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, stop);
    signal(SIGINT, stop);
    for (i = 1; i < argc; ++i) {
        if (equal(argv[i], "--ascii"))
            flags &= ~PS_UNICODE;
        else if (equal(argv[i], "--no-color") || equal(argv[i], "--mono"))
            flags &= ~PS_COLOR;
        else if (equal(argv[i], "--no-bell"))
            flags |= PS_NO_BELL;
        else if (equal(argv[i], "--no-suggestions"))
            flags |= PS_NO_SUGGESTIONS;
        else if (equal(argv[i], "--no-exec"))
            execution = 0;
        else if (equal(argv[i], "--shortcuts"))
            flags |= PS_EXTENDED_KEYS;
        else if (equal(argv[i], "--batch"))
            batch = 1;
        else if (equal(argv[i], "--interactive"))
            force = 1;
        else if (equal(argv[i], "--device-commands")) {
            use_device_commands = 1;
            flags |= PS_CASE_INSENSITIVE | PS_NO_BUILTINS | PS_LEGACY_KEYS;
        } else if (equal(argv[i], "--board"))
            board = 1;
        else if (equal(argv[i], "--edit"))
            board = 4;
        else if (equal(argv[i], "--files"))
            board = 5;
        else if (equal(argv[i], "--menu"))
            board = 3;
        else if (equal(argv[i], "--controls"))
            board = 2;
        else if (equal(argv[i], "--")) {
            command_at = i + 1;
            break;
        } else if (equal(argv[i], "--help")) {
            say("PortableShell [--device-commands] [--board|--controls|--menu|--edit|--files] [--ascii] [--no-color] [--no-bell] [--no-suggestions] [--history FILE]\n      "
                "[--command TEXT | "
                "-- ARGV...] [--batch | --interactive] [--no-exec] [--shortcuts]\n      [--listen PORT [--bind IPv4]] "
                "[--serial PATH [--baud N]]\n");
            return 0;
        } else if (i + 1 < argc && (equal(argv[i], "--command") || equal(argv[i], "--listen") ||
                                    equal(argv[i], "--bind") || equal(argv[i], "--serial") ||
                                    equal(argv[i], "--baud") || equal(argv[i], "--history"))) {
            const char *opt = argv[i++];
            if (equal(opt, "--command"))
                command = argv[i];
            else if (equal(opt, "--listen")) {
                if (!number(argv[i], &port) || !port || port > 65535)
                    return 2;
            } else if (equal(opt, "--bind"))
                host = argv[i];
            else if (equal(opt, "--serial"))
                serial = argv[i];
            else if (equal(opt, "--history"))
                history = argv[i];
            else if (!number(argv[i], &baud))
                return 2;
        } else {
            say("Unknown or incomplete option\n");
            return 2;
        }
    }
    if (port)
        return server(port, host, flags);
    if (serial) {
        fd = open(serial, O_RDWR | O_NOCTTY);
        if (fd < 0) {
            say("Cannot open serial port\n");
            return 1;
        }
        outfd = fd;
        force = 1;
    }
    if ((isatty(fd) && isatty(outfd) && !batch && !command && !command_at) || force)
        flags |= PS_INTERACTIVE;
    channels[0].fd = outfd;
    cfg.write = tx;
    cfg.context = &channels[0];
    cfg.columns = 100;
    cfg.rows = 28;
    cfg.flags = flags;
    s = ps_init(memory[0].data, sizeof memory[0].data, &cfg);
    if (!s)
        return 1;
    if (use_device_commands)
        ps_device_attach(s, &profiles[0], 0, 0);
    if (!serial && !use_device_commands && fs_attach(s)) return 2;
#if PS_HOST_EXEC
    if (execution && !serial && !use_device_commands && host_attach(s)) return 2;
#else
    (void)execution;
#endif
    if (command || command_at) {
        result =
            command ? ps_execute(s, command)
                    : ps_execute_argv(s, argc - command_at, (const char *const *)argv + command_at);
        ps_finish(s);
        flush(&channels[0]);
        return result ? result : ps_status(s);
    }
    if (history)
        history_load(s, history);
    if ((flags & PS_INTERACTIVE) && tcgetattr(fd, &saved) == 0) {
        struct termios t = saved;
        t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        t.c_oflag &= ~OPOST;
        t.c_cflag = (t.c_cflag & ~(CSIZE | PARENB)) | CS8 | CLOCAL | CREAD;
        t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        if (serial) {
            speed_t speed = baud == 9600     ? B9600
                            : baud == 19200  ? B19200
                            : baud == 38400  ? B38400
                            : baud == 57600  ? B57600
                            : baud == 115200 ? B115200
                                             : 0;
            if (!speed) {
                close(fd);
                return 2;
            }
            cfsetispeed(&t, speed);
            cfsetospeed(&t, speed);
        }
        if (tcsetattr(fd, TCSAFLUSH, &t) == 0)
            raw = 1;
    }
    if (flags & PS_INTERACTIVE)
        resize(s, outfd);
    ps_start(s);
    if (board)
        ps_execute(s, board == 5 ? "files" : board == 4 ? "edit" : board == 3 ? "menu" : board == 2 ? "controls" : "board");
    start = clock_ms();
    while (!stopping && !ps_exited(s) && !channels[0].failed) {
        struct pollfd p = {fd, POLLIN, 0};
        int ready = poll(&p, 1, 20);
        if (ready > 0 && (p.revents & (POLLIN | POLLHUP))) {
            ps_byte b[2048];
            ssize_t n = read(fd, b, execution && !serial ? 1 : sizeof b);
            if (n > 0)
                ps_feed(s, b, (unsigned)n);
            else if (n == 0 && !serial)
                break;
            else if (n < 0 && errno != EINTR && errno != EAGAIN)
                break;
        }
        if (flags & PS_INTERACTIVE) {
            struct winsize w;
            if (ioctl(outfd, TIOCGWINSZ, &w) == 0 && (w.ws_col != oldw || w.ws_row != oldh)) {
                oldw = w.ws_col;
                oldh = w.ws_row;
                if (oldw && oldh)
                    ps_resize(s, oldw, oldh);
            }
        }
        ps_tick(s, clock_ms() - start);
        flush(&channels[0]);
    }
    ps_finish(s);
    ps_close(s);
    flush(&channels[0]);
    if (history)
        history_save(s, history);
    if (raw)
        tcsetattr(fd, TCSAFLUSH, &saved);
    if (serial)
        close(fd);
    return channels[0].failed ? 1 : ps_status(s);
}
