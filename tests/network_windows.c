/* Brief loopback integration check for the native Windows adapter. */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#ifndef TEST_PORT
#define TEST_PORT 2323
#endif
static char output[65536];
static unsigned used;
static int has(const char *needle) {
    unsigned i, j;
    for (i = 0; i < used; ++i) {
        for (j = 0; needle[j] && i + j < used && output[i + j] == needle[j]; ++j) {
        }
        if (!needle[j])
            return 1;
    }
    return 0;
}
static void collect(SOCKET s, unsigned ms) {
    DWORD start = GetTickCount();
    while (GetTickCount() - start < ms) {
        fd_set f;
        struct timeval wait = {0, 20000};
        FD_ZERO(&f);
        FD_SET(s, &f);
        if (select(0, &f, 0, 0, &wait) > 0) {
            int n = recv(s, output + used, sizeof output - used - 1, 0);
            if (n <= 0)
                break;
            used += (unsigned)n;
            if (used + 1 == sizeof output)
                break;
        }
    }
    output[used] = 0;
}
static int send_all(SOCKET s, const char *p, int n) {
    while (n) {
        int k = send(s, p, n, 0);
        if (k <= 0)
            return 0;
        p += k;
        n -= k;
    }
    return 1;
}
static int check(void) {
    WSADATA w;
    struct sockaddr_in addr = {0};
    SOCKET s;
    unsigned i;
    const unsigned char negotiation[] = {255, 253, 1,  255, 253, 3,   255, 251, 3,   255, 251, 31,
                                         255, 251, 24, 255, 250, 31,  0,   100, 0,   28,  255, 240,
                                         255, 250, 24, 0,   'x', 't', 'e', 'r', 'm', 255, 240};
    const char commands[] = "echo 'wire test'\r\nboard\r\n";
    const char keys[] = "nTCP task\r\t\t\t";
    const char tail[] = "status\rexit\r";
    if (WSAStartup(MAKEWORD(2, 2), &w))
        return 1;
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TEST_PORT);
    addr.sin_addr.s_addr = htonl(0x7f000001);
    if (connect(s, (struct sockaddr *)&addr, sizeof addr)) {
        closesocket(s);
        WSACleanup();
        return 2;
    }
    collect(s, 100);
    if (used < 15 || (unsigned char)output[0] != 255)
        return 3;
    used = 0;
    for (i = 0; i < sizeof negotiation; ++i)
        if (!send_all(s, (const char *)negotiation + i, 1))
            return 4;
#ifdef TEST_PU850
    (void)commands;
    (void)keys;
    (void)tail;
    {
        const char probe[] = "EcHo  raw 'quoted tail\r\nHEX -1\r\nSaVe\r\n";
        used = 0;
        send_all(s, probe, sizeof probe - 1);
        collect(s, 180);
        if (!has(" raw 'quoted tail") || !has("FFFFFFFF") || !has("Device backend not attached.") ||
            !has("\033[1;93m"))
            return 8;
        send_all(s, "EXIT\r\n", 6);
        collect(s, 80);
        closesocket(s);
        WSACleanup();
        return 0;
    }
#endif
#ifdef TEST_CONTROLS
    {
        const char open[] = "controls\r\n";
        const char adjust[] = " \033[B\033[B\033[B\033[C\033OQ\0253000\t\r";
        const char readback[] = "controls get 3\rcontrols get 0\rexit\r";
        send_all(s, open, sizeof open - 1);
        collect(s, 120);
        if (!has("Local bench") || !has("Relay 1"))
            return 9;
        used = 0;
        send_all(s, adjust, sizeof adjust - 1);
        collect(s, 120);
        if (!has("Applied") || !has("3000"))
            return 10;
        send_all(s, "\033", 1);
        collect(s, 120);
        used = 0;
        send_all(s, readback, sizeof readback - 1);
        collect(s, 120);
        closesocket(s);
        WSACleanup();
        return has("3000\r\n") && has("On\r\n") ? 0 : 11;
    }
#endif
    send_all(s, commands, sizeof commands - 1);
    collect(s, 180);
    if (!has("wire test") || !has("Tasks"))
        return 5;
    used = 0;
    send_all(s, keys, sizeof keys - 1);
    collect(s, 180);
    if (!has("Session bytes") || !has("Telnet / byte callbacks") || !has("xterm"))
        return 6;
    used = 0;
    send_all(s, "\033", 1);
    collect(s, 120);
    send_all(s, tail, sizeof tail - 1);
    collect(s, 180);
    closesocket(s);
    WSACleanup();
    if (!has("tasks=1"))
        return 7;
    return 0;
}
void network_entry(void) {
    DWORD n;
    int rc = check();
    const char *message = rc ? "Network check FAILED\r\n" : "Network check passed\r\n";
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, rc ? 22 : 22, &n, 0);
    ExitProcess((UINT)rc);
}
