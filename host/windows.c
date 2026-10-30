/* Optional Windows byte transport. Native OS imports only, no CRT or third-party
   libraries. The core is separately compiled and knows nothing about this file. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include "portable_shell.h"
#include "portable_shell_device.h"
#include "portable_terminal.h"
#include "process.h"
static unsigned use_device_commands;
static ps_device_profile profiles[4];
#include "filesystem.h"
#define CLIENTS 4
#define QUEUE 131072
typedef struct {
    SOCKET socket;
    HANDLE stream;
    unsigned used, failed;
    ps_byte queue[QUEUE];
} Channel;
typedef union {
    void *alignment;
    ps_byte data[40000];
} Storage;
static Storage storage[CLIENTS];
static Channel channels[CLIENTS];
static ps_session *sessions[CLIENTS];
static char arguments[8192];
static const char *argv[64];
static unsigned argc;
static HANDLE input, output;
static DWORD input_mode, output_mode;
static UINT old_cp;
static int console_in, console_out;
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
static void zero(void *p, unsigned n) {
    ps_byte *b = p;
    while (n--)
        *b++ = 0;
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
static void flush(Channel *c) {
    unsigned done = 0;
    while (done < c->used && !c->failed) {
        if (c->socket != INVALID_SOCKET) {
            int n = send(c->socket, (const char *)c->queue + done, (int)(c->used - done), 0);
            if (n == SOCKET_ERROR) {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    break;
                c->failed = 1;
                break;
            }
            if (!n) {
                c->failed = 1;
                break;
            }
            done += (unsigned)n;
        } else {
            DWORD n = 0;
            if (!WriteFile(c->stream, c->queue + done, c->used - done, &n, 0) || !n) {
                c->failed = 1;
                break;
            }
            done += n;
        }
    }
    if (done) {
        unsigned i;
        for (i = done; i < c->used; ++i)
            c->queue[i - done] = c->queue[i];
        c->used -= done;
    }
}
static void write_bytes(void *ctx, const ps_byte *p, unsigned n) {
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
static void say(const char *p) {
    DWORD n;
    WriteFile(output, p, length(p), &n, 0);
}
static unsigned utf8(unsigned cp, ps_byte *b) {
    if (cp < 128) {
        b[0] = (ps_byte)cp;
        return 1;
    }
    if (cp < 2048) {
        b[0] = (ps_byte)(192 | (cp >> 6));
        b[1] = (ps_byte)(128 | (cp & 63));
        return 2;
    }
    if (cp < 65536) {
        b[0] = (ps_byte)(224 | (cp >> 12));
        b[1] = (ps_byte)(128 | ((cp >> 6) & 63));
        b[2] = (ps_byte)(128 | (cp & 63));
        return 3;
    }
    b[0] = (ps_byte)(240 | (cp >> 18));
    b[1] = (ps_byte)(128 | ((cp >> 12) & 63));
    b[2] = (ps_byte)(128 | ((cp >> 6) & 63));
    b[3] = (ps_byte)(128 | (cp & 63));
    return 4;
}
static int get_args(void) {
    const WCHAR *p = GetCommandLineW();
    unsigned pos = 0;
    while (*p) {
        unsigned quote = 0;
        while (*p == ' ' || *p == '\t')
            ++p;
        if (!*p)
            break;
        if (argc == 64)
            return 0;
        argv[argc++] = arguments + pos;
        while (*p) {
            unsigned slash = 0, cp, n, i;
            ps_byte b[4];
            if (!quote && (*p == ' ' || *p == '\t'))
                break;
            while (*p == '\\') {
                ++slash;
                ++p;
            }
            if (*p == '"') {
                for (i = 0; i < slash / 2; ++i) {
                    if (pos + 1 >= sizeof arguments)
                        return 0;
                    arguments[pos++] = '\\';
                }
                if (slash & 1) {
                    if (pos + 1 >= sizeof arguments)
                        return 0;
                    arguments[pos++] = '"';
                } else
                    quote ^= 1;
                ++p;
                continue;
            }
            while (slash--) {
                if (pos + 1 >= sizeof arguments)
                    return 0;
                arguments[pos++] = '\\';
            }
            if (!*p || (!quote && (*p == ' ' || *p == '\t')))
                break;
            cp = *p++;
            if (cp >= 0xd800 && cp <= 0xdbff && *p >= 0xdc00 && *p <= 0xdfff)
                cp = 0x10000 + ((cp - 0xd800) << 10) + (*p++ - 0xdc00);
            n = utf8(cp, b);
            if (pos + n + 1 >= sizeof arguments)
                return 0;
            for (i = 0; i < n; ++i)
                arguments[pos++] = (char)b[i];
        }
        arguments[pos++] = 0;
    }
    return 1;
}
static void resize(ps_session *s) {
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(output, &info))
        ps_resize(s, (unsigned)(info.srWindow.Right - info.srWindow.Left + 1),
                  (unsigned)(info.srWindow.Bottom - info.srWindow.Top + 1));
}
/* Deliver one byte batch per console read, coalescing full-screen redraws while
   retaining every key/repeat in order. The core remains transport-independent. */
static void keyboard_append(ps_session *s, ps_byte *batch, unsigned *used, const ps_byte *bytes,
                            unsigned count) {
    if (*used + count > 512) {
        ps_feed(s, batch, *used);
        *used = 0;
    }
    while (count--)
        batch[(*used)++] = *bytes++;
}
static void keyboard(ps_session *s) {
    INPUT_RECORD events[32];
    ps_byte batch[512];
    unsigned used = 0;
    DWORD n, i;
    static unsigned high = 0;
    static DWORD mouse_buttons=0;
    if (!ReadConsoleInputW(input, events, 1, &n))
        return;
    for (i = 0; i < n; ++i) {
        KEY_EVENT_RECORD *k = &events[i].Event.KeyEvent;
        const char *seq = 0;
        unsigned repeat;
        if (events[i].EventType == WINDOW_BUFFER_SIZE_EVENT) {
            if (used) {
                ps_feed(s, batch, used);
                used = 0;
            }
            resize(s);
            continue;
        }
        if(PS_MOUSE && events[i].EventType==MOUSE_EVENT){
            MOUSE_EVENT_RECORD *m=&events[i].Event.MouseEvent;
            CONSOLE_SCREEN_BUFFER_INFO info;
            unsigned code,release=0;char packet[48];
            DWORD buttons=m->dwButtonState&0xffff;
            if(m->dwEventFlags==DOUBLE_CLICK)continue; /* Press already handled. */
            if(m->dwEventFlags==MOUSE_WHEELED)code=(SHORT)HIWORD(m->dwButtonState)>0?64:65;
            else if(m->dwEventFlags==MOUSE_HWHEELED)continue;
            else {
                DWORD held=buttons?buttons:mouse_buttons;
                code=(held&FROM_LEFT_1ST_BUTTON_PRESSED)?0:(held&RIGHTMOST_BUTTON_PRESSED)?2:1;
                release=buttons==0;
                if(m->dwEventFlags==MOUSE_MOVED){if(!buttons)continue;code|=32;}
                mouse_buttons=buttons;
            }
            if(m->dwControlKeyState&SHIFT_PRESSED)code|=4;
            if(m->dwControlKeyState&(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))code|=8;
            if(m->dwControlKeyState&(LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))code|=16;
            if(!GetConsoleScreenBufferInfo(output,&info))continue;
            {
                int x=m->dwMousePosition.X-info.srWindow.Left+1,y=m->dwMousePosition.Y-info.srWindow.Top+1;
                if(x<1||y<1)continue;
                ps_terminal_format(packet,sizeof packet,"\033[<%u;%u;%u%s",code,(unsigned)x,(unsigned)y,release?"m":"M");
                keyboard_append(s,batch,&used,(const ps_byte*)packet,length(packet));
            }
            continue;
        }
        if (events[i].EventType != KEY_EVENT || !k->bKeyDown)
            continue;
        switch (k->wVirtualKeyCode) {
        case VK_UP:
            seq = "\033[A";
            break;
        case VK_DOWN:
            seq = "\033[B";
            break;
        case VK_LEFT:
            seq = (k->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) ? "\033[1;5D"
                                                                                    : "\033[D";
            break;
        case VK_RIGHT:
            seq = (k->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) ? "\033[1;5C"
                                                                                    : "\033[C";
            break;
        case VK_HOME:
            seq = "\033[H";
            break;
        case VK_END:
            seq = "\033[F";
            break;
        case VK_DELETE:
            seq = "\033[3~";
            break;
        case VK_PRIOR:
            seq = "\033[5~";
            break;
        case VK_NEXT:
            seq = "\033[6~";
            break;
        case VK_F1:
            seq = "\033OP";
            break;
        case VK_F2:
            seq = "\033OQ";
            break;
        case VK_F3:
            seq = "\033OR";
            break;
        case VK_F4:
            seq = "\033OS";
            break;
        case VK_TAB:
            seq = (k->dwControlKeyState & SHIFT_PRESSED) ? "\033[Z" : "\t";
            break;
        default:
            break;
        }
        for (repeat = 0; repeat < k->wRepeatCount; ++repeat) {
            if (seq)
                keyboard_append(s, batch, &used, (const ps_byte *)seq, length(seq));
            else if (k->uChar.UnicodeChar) {
                unsigned cp = k->uChar.UnicodeChar;
                ps_byte b[4];
                if (cp >= 0xd800 && cp <= 0xdbff) {
                    high = cp;
                    continue;
                }
                if (cp >= 0xdc00 && cp <= 0xdfff && high) {
                    cp = 0x10000 + ((high - 0xd800) << 10) + (cp - 0xdc00);
                    high = 0;
                } else
                    high = 0;
                if ((k->dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) &&
                    !(k->dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
                    keyboard_append(s, batch, &used, (const ps_byte *)"\033", 1);
                keyboard_append(s, batch, &used, b, utf8(cp, b));
            }
        }
    }
    if (used)
        ps_feed(s, batch, used);
}
static int ip4(const char *p, ULONG *address) {
    unsigned part = 0, count = 0, value = 0;
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
    *address = htonl(value);
    return 1;
}
static int server(unsigned port, const char *bind_addr, unsigned flags) {
    WSADATA data;
    SOCKET listener;
    struct sockaddr_in addr;
    unsigned i;
    ps_u32 start = GetTickCount();
    if (WSAStartup(MAKEWORD(2, 2), &data))
        return 1;
    listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == INVALID_SOCKET)
        return 1;
    zero(&addr, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    if (!ip4(bind_addr, &addr.sin_addr.s_addr)) {
        say("Invalid IPv4 bind address\r\n");
        closesocket(listener);
        WSACleanup();
        return 2;
    }
    if (bind(listener, (struct sockaddr *)&addr, sizeof addr) || listen(listener, 4)) {
        say("Cannot bind listener\r\n");
        closesocket(listener);
        WSACleanup();
        return 1;
    }
    {
        u_long nonblock = 1;
        ioctlsocket(listener, FIONBIO, &nonblock);
    }
    for (i = 0; i < CLIENTS; ++i)
        channels[i].socket = INVALID_SOCKET;
    say("PortableShell Telnet listening on ");
    say(bind_addr);
    say(" (Ctrl+C stops host)\r\n");
    for (;;) {
        fd_set reads, writes;
        struct timeval timeout = {0, 20000};
        FD_ZERO(&reads);
        FD_ZERO(&writes);
        FD_SET(listener, &reads);
        for (i = 0; i < CLIENTS; ++i)
            if (sessions[i]) {
                FD_SET(channels[i].socket, &reads);
                if (channels[i].used)
                    FD_SET(channels[i].socket, &writes);
            }
        if (select(0, &reads, &writes, 0, &timeout) == SOCKET_ERROR)
            break;
        if (FD_ISSET(listener, &reads)) {
            SOCKET client = accept(listener, 0, 0);
            if (client != INVALID_SOCKET) {
                for (i = 0; i < CLIENTS && sessions[i]; ++i) {
                }
                if (i == CLIENTS)
                    closesocket(client);
                else {
                    u_long nonblock = 1;
                    int yes = 1;
                    ps_config cfg;
                    zero(&channels[i], sizeof(Channel));
                    channels[i].socket = client;
                    ioctlsocket(client, FIONBIO, &nonblock);
                    setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (const char *)&yes, sizeof yes);
                    cfg.write = write_bytes;
                    cfg.context = &channels[i];
                    cfg.columns = 100;
                    cfg.rows = 28;
                    cfg.flags = flags | PS_INTERACTIVE | PS_TELNET;
                    sessions[i] = ps_init(storage[i].data, sizeof storage[i].data, &cfg);
                    if (use_device_commands)
                        ps_device_attach(sessions[i], &profiles[i], 0, 0);
#ifdef PS_HOST_SESSION_READY
                    PS_HOST_SESSION_READY(sessions[i]);
#endif
                    ps_start(sessions[i]);
                }
            }
        }
        for (i = 0; i < CLIENTS; ++i)
            if (sessions[i]) {
                if (FD_ISSET(channels[i].socket, &reads)) {
                    ps_byte b[2048];
                    int n = recv(channels[i].socket, (char *)b, sizeof b, 0);
                    if (n > 0)
                        ps_feed(sessions[i], b, (unsigned)n);
                    else if (n == 0 || WSAGetLastError() != WSAEWOULDBLOCK)
                        channels[i].failed = 1;
                }
                ps_tick(sessions[i], GetTickCount() - start);
                flush(&channels[i]);
                if (channels[i].failed || (ps_exited(sessions[i]) && !channels[i].used)) {
                    closesocket(channels[i].socket);
                    channels[i].socket = INVALID_SOCKET;
                    sessions[i] = 0;
                }
            }
    }
    for (i = 0; i < CLIENTS; ++i)
        if (sessions[i])
            closesocket(channels[i].socket);
    closesocket(listener);
    WSACleanup();
    return 1;
}
static void history_load(ps_session *s, const char *path) {
    HANDLE h;
    DWORD n;
    char line[PS_LINE];
    unsigned pos = 0;
    ps_byte b;
    h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return;
    while (ReadFile(h, &b, 1, &n, 0) && n) {
        if (b == '\n') {
            line[pos] = 0;
            ps_history_add(s, line);
            pos = 0;
        } else if (b != '\r' && pos + 1 < PS_LINE)
            line[pos++] = (char)b;
    }
    if (pos) {
        line[pos] = 0;
        ps_history_add(s, line);
    }
    CloseHandle(h);
}
static void history_save(ps_session *s, const char *path) {
    HANDLE h;
    unsigned i;
    DWORD n;
    h = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return;
    for (i = 0; i < ps_history_count(s); ++i) {
        const char *p = ps_history_get(s, i);
        WriteFile(h, p, length(p), &n, 0);
        WriteFile(h, "\n", 1, &n, 0);
    }
    CloseHandle(h);
}
#if PS_HOST_EXEC
#include "execution.h"
#endif
int host_main(void) {
    unsigned flags = PS_COLOR | PS_UNICODE | PS_TRUECOLOR, port = 0, baud = 115200, i, board = 0,
             execution = PS_HOST_EXEC, batch = 0, force = 0, command_at = 0;
    const char *command = 0, *bind_addr = "127.0.0.1", *serial = 0, *history = 0;
    ps_config cfg;
    ps_session *s;
    ps_u32 start;
    int result;
    output = GetStdHandle(STD_OUTPUT_HANDLE);
    input = GetStdHandle(STD_INPUT_HANDLE);
    if (!get_args()) {
        say("Argument capacity exceeded\r\n");
        return 2;
    }
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
        else if (equal(argv[i], "--help")) {
            say("PortableShell [--device-commands] [--board|--controls|--menu|--edit|--files] [--ascii] [--no-color] [--no-bell] [--no-suggestions] [--history "
                "FILE]\r\n     "
                " "
                "[--command TEXT "
                "| -- ARGV...] [--batch | --interactive] [--no-exec] [--shortcuts]\r\n      [--listen PORT [--bind IPv4]] "
                "[--serial PORT [--baud N]]\r\n");
            return 0;
        } else if (equal(argv[i], "--")) {
            command_at = i + 1;
            break;
        } else if (i + 1 < argc && (equal(argv[i], "--command") || equal(argv[i], "--listen") ||
                                    equal(argv[i], "--bind") || equal(argv[i], "--serial") ||
                                    equal(argv[i], "--baud") || equal(argv[i], "--history"))) {
            const char *opt = argv[i++];
            if (equal(opt, "--command"))
                command = argv[i];
            else if (equal(opt, "--listen")) {
                if (!number(argv[i], &port) || port < 1 || port > 65535)
                    return 2;
            } else if (equal(opt, "--bind"))
                bind_addr = argv[i];
            else if (equal(opt, "--serial"))
                serial = argv[i];
            else if (equal(opt, "--history"))
                history = argv[i];
            else if (!number(argv[i], &baud) || !baud)
                return 2;
        } else {
            say("Unknown or incomplete option: ");
            say(argv[i]);
            say("\r\n");
            return 2;
        }
    }
    if (port)
        return server(port, bind_addr, flags);
    console_in = GetConsoleMode(input, &input_mode);
    console_out = GetConsoleMode(output, &output_mode);
    if (console_in && console_out && !batch && !command && !command_at)
        flags |= PS_INTERACTIVE;
    if (force)
        flags |= PS_INTERACTIVE;
    channels[0].socket = INVALID_SOCKET;
    channels[0].stream = output;
    if (serial) {
        DCB dcb;
        COMMTIMEOUTS timeouts = {MAXDWORD, 0, 20, 0, 1000};
        HANDLE h = CreateFileA(serial, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
        if (h == INVALID_HANDLE_VALUE) {
            say("Cannot open serial port\r\n");
            return 1;
        }
        zero(&dcb, sizeof dcb);
        dcb.DCBlength = sizeof dcb;
        if (!GetCommState(h, &dcb)) {
            CloseHandle(h);
            return 1;
        }
        dcb.BaudRate = baud;
        dcb.ByteSize = 8;
        dcb.Parity = NOPARITY;
        dcb.StopBits = ONESTOPBIT;
        dcb.fBinary = 1;
        dcb.fOutxCtsFlow = dcb.fOutxDsrFlow = dcb.fOutX = dcb.fInX = 0;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        if (!SetCommState(h, &dcb) || !SetCommTimeouts(h, &timeouts)) {
            CloseHandle(h);
            return 1;
        }
        input = h;
        channels[0].stream = h;
        console_in = console_out = 0;
        flags |= PS_INTERACTIVE;
    }
    cfg.write = write_bytes;
    cfg.context = &channels[0];
    cfg.columns = 100;
    cfg.rows = 28;
    cfg.flags = flags;
    s = ps_init(storage[0].data, sizeof storage[0].data, &cfg);
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
        result = command ? ps_execute(s, command)
                         : ps_execute_argv(s, (int)(argc - command_at), argv + command_at);
        ps_finish(s);
        flush(&channels[0]);
        return result ? result : ps_status(s);
    }
    if (history)
        history_load(s, history);
    if (console_out && (flags & PS_INTERACTIVE)) {
        SetConsoleTitleW(L"PortableShell - byte-native workbench");
        old_cp = GetConsoleOutputCP();
        SetConsoleOutputCP(CP_UTF8);
        if (!SetConsoleMode(output, output_mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            say("VT output unavailable; use --batch\r\n");
            return 1;
        }
        resize(s);
    }
    if (console_in && (flags & PS_INTERACTIVE))
        SetConsoleMode(input, (input_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT |
                                              ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE)) |
                                  ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS | (PS_MOUSE?ENABLE_MOUSE_INPUT:0));
    ps_start(s);
    if (board)
        ps_execute(s, board == 5 ? "files" : board == 4 ? "edit" : board == 3 ? "menu" : board == 2 ? "controls" : "board");
    start = GetTickCount();
    for (;;) {
        if (ps_exited(s) || channels[0].failed)
            break;
        if (console_in && (flags & PS_INTERACTIVE)) {
            if (WaitForSingleObject(input, 20) == WAIT_OBJECT_0)
                keyboard(s);
        } else {
            DWORD n;
            ps_byte b[1024];
            if ((flags & PS_INTERACTIVE) && !serial && GetFileType(input) == FILE_TYPE_PIPE) {
                DWORD available = 0;
                if (!PeekNamedPipe(input, 0, 0, 0, &available, 0))
                    break;
                if (!available) {
                    ps_tick(s, GetTickCount() - start);
                    flush(&channels[0]);
                    Sleep(20);
                    continue;
                }
            }
            if (!ReadFile(input, b, execution && !serial ? 1 : sizeof b, &n, 0) || (!n && !serial))
                break;
            /* Local process batch input uses LF/CRLF records. Wait for LF before
               dispatch so a CRLF tail cannot become the child's first stdin byte.
               Raw serial/Telnet and execution-disabled byte sessions are unchanged. */
            if (execution && !serial && !(flags & PS_INTERACTIVE) && n == 1 && b[0] == '\r')
                continue;
            if (n)
                ps_feed(s, b, n);
        }
        ps_tick(s, GetTickCount() - start);
        flush(&channels[0]);
    }
    ps_finish(s);
    ps_close(s);
    flush(&channels[0]);
    if (history)
        history_save(s, history);
    if (console_in)
        SetConsoleMode(input, input_mode);
    if (console_out) {
        SetConsoleMode(output, output_mode);
        if (old_cp)
            SetConsoleOutputCP(old_cp);
    }
    if (serial)
        CloseHandle(input);
    result = ps_status(s);
    return channels[0].failed ? 1 : result;
}
#ifdef PS_HOSTED_CRT
int main(void) {
    return host_main();
}
#else
void host_entry(void) {
    ExitProcess((UINT)host_main());
}
#endif
