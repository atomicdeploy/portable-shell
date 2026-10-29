/* Optional native loopback smoke probe. Only the process created here is stopped.
   Usage: feedback_probe PATH_TO_HOST [--no-bell] */
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
static int contains(const char *bytes, int count, const char *text) {
    int i, n = (int)strlen(text);
    for (i = 0; i + n <= count; ++i)
        if (!memcmp(bytes + i, text, (size_t)n)) return 1;
    return 0;
}
int main(int argc, char **argv) {
    WSADATA data;
    SOCKET reservation = INVALID_SOCKET, client = INVALID_SOCKET;
    struct sockaddr_in address;
    int size = sizeof address, n, total = 0, ok = 0, mute = argc > 2;
    unsigned attempt;
    DWORD timeout = 3000;
    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    char command[2048], received[65536];
    static const unsigned char negotiate[] = {255,253,1,255,253,3,255,251,31,
                                              255,250,31,0,96,0,36,255,240};
    static const char keys[] = "\bth\033[C lum\033[F\rhistory\rhelp\rhelp --compact\rbell\recho live-feedback-ok\rexit\r";
    if (argc < 2 || WSAStartup(MAKEWORD(2,2), &data)) return 2;
    reservation = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(reservation, (struct sockaddr *)&address, size) ||
        getsockname(reservation, (struct sockaddr *)&address, &size)) goto done;
    closesocket(reservation); reservation = INVALID_SOCKET;
    snprintf(command, sizeof command, "\"%s\" --listen %u --bind 127.0.0.1 %s",
             argv[1], (unsigned)ntohs(address.sin_port), mute ? "--no-bell" : "");
    startup.cb = sizeof startup;
    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                        NULL, NULL, &startup, &process)) goto done;
    for (attempt = 0; attempt < 100; ++attempt) {
        client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!connect(client, (struct sockaddr *)&address, size)) break;
        closesocket(client); client = INVALID_SOCKET;
        if (WaitForSingleObject(process.hProcess, 20) == WAIT_OBJECT_0) break;
    }
    if (client == INVALID_SOCKET) goto done;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof timeout);
    if (send(client, (const char *)negotiate, sizeof negotiate, 0) != sizeof negotiate ||
        send(client, keys, sizeof keys - 1, 0) != sizeof keys - 1) goto done;
    while (total < (int)sizeof received - 1 &&
           (n = recv(client, received + total, sizeof received - 1 - total, 0)) > 0)
        total += n;
    received[total] = 0;
    /* NVT cursor-return sequences contain CR NUL; inspect the entire byte span. */
    ok = contains(received, total, "theme lumen") && contains(received, total, "Workspace:") &&
         contains(received, total, "live-feedback-ok\r\n") &&
         (!!memchr(received, 7, (size_t)total) == !mute) &&
         memchr(received, 255, (size_t)total);
    printf("native Telnet: %s, %d received bytes, BEL %s\n", ok ? "PASS" : "FAIL",
           total, mute ? "muted" : "present");
    if (!ok) printf("history=%d headings=%d echo=%d bell=%d negotiation=%d\n",
        !!contains(received, total, "theme lumen"), !!contains(received, total, "Workspace:"),
        !!contains(received, total, "live-feedback-ok"), !!memchr(received,7,(size_t)total),
        !!memchr(received,255,(size_t)total));
done:
    if (client != INVALID_SOCKET) closesocket(client);
    if (reservation != INVALID_SOCKET) closesocket(reservation);
    if (process.hProcess) {
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 3000);
        CloseHandle(process.hProcess); CloseHandle(process.hThread);
    }
    WSACleanup();
    return ok ? 0 : 1;
}
