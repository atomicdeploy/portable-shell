#include "portable_shell_legacy.h"
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/* Demo parameters */
static int demo_counter = 0;
static float demo_temperature = 25.5;
static char demo_name[64] = "MyDevice";
static bool demo_enabled = true;

/* Custom command example */
int cmd_status(PortableShell* shell, int argc, char** argv) {
    char value[128];
    (void)argc; (void)argv;
    snprintf(value, sizeof(value), "counter=%d temperature=%.2f enabled=%s name=%s",
             demo_counter, (double)demo_temperature, demo_enabled ? "on" : "off", demo_name);
    ps_println(shell, value);
    return 0;
}

int cmd_reboot(PortableShell* shell, int argc, char** argv) {
    (void)argc; (void)argv;
    ps_stop(shell);
    return 0;
}

/* Setup raw terminal mode for proper keyboard input handling */
static struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &orig_termios)) return;
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    
    /* Enable raw mode for proper keyboard handling */
    enable_raw_mode();
    
    /* Create shell configuration */
    PSConfig config = {0};
    config.history_size = 20;
    config.vt100_enabled = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
    config.prompt = "$ ";  /* Simple prompt */
    
    /* Create shell instance */
    PortableShell* shell = ps_create(&config);
    if (!shell) {
        fprintf(stderr, "Failed to create shell\n");
        return 1;
    }
    
    /* Register custom commands */
    ps_register_command(shell, "status", "Show system status", cmd_status);
    ps_register_command(shell, "stop", "Close this session", cmd_reboot);
    
    /* Register parameters */
    ps_register_parameter(shell, "counter", PS_PARAM_INT, &demo_counter, 0);
    ps_register_parameter(shell, "temperature", PS_PARAM_FLOAT, &demo_temperature, 0);
    ps_register_parameter(shell, "name", PS_PARAM_STRING, demo_name, sizeof(demo_name));
    ps_register_parameter(shell, "enabled", PS_PARAM_BOOL, &demo_enabled, 0);
    
    /* Run shell */
    ps_run(shell);
    
    /* Cleanup */
    ps_destroy(shell);
    
    return 0;
}
