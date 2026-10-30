#include "portable_shell.h"
#include <assert.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../host/windows_console.h"
#endif
static union { void *align; unsigned char bytes[65536]; } memory;
static char output[65536];static unsigned used;
static void sink(void *ctx,const ps_byte *p,unsigned n){(void)ctx;assert(used+n<sizeof output);memcpy(output+used,p,n);used+=n;output[used]=0;}
int main(void){
    ps_config cfg={sink,0,96,28,PS_INTERACTIVE};
    ps_session *s=ps_init(memory.bytes,sizeof memory.bytes,&cfg);assert(s);
    assert(!ps_terminal_state(s));ps_start(s);
    assert(!ps_terminal_state(s));assert(strstr(output,"\033[?1002l"));
    assert(!strstr(output,"\033[?1049h"));assert(!strstr(output,"\0338"));
    assert(!ps_execute(s,"board"));
    assert(ps_terminal_state(s)&PS_TERMINAL_ALT_SCREEN);
    assert(!!(ps_terminal_state(s)&PS_TERMINAL_MOUSE)==!!PS_MOUSE);
    ps_key(s,27);assert(!ps_terminal_state(s));
    assert(!ps_execute(s,"board --prompt"));
    assert(ps_terminal_state(s)&PS_TERMINAL_ALT_SCREEN);
    ps_key(s,PS_KEY_F4);ps_key(s,27);assert(!ps_terminal_state(s));
    assert(!ps_execute(s,"edit"));
    assert(ps_terminal_state(s)==PS_TERMINAL_ALT_SCREEN);
    ps_close(s);assert(!ps_terminal_state(s));
#ifdef _WIN32
    { DWORD original=ENABLE_MOUSE_INPUT|ENABLE_QUICK_EDIT_MODE|ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT;
      DWORD prompt=ps_windows_input_mode(original,0),table=ps_windows_input_mode(original,1);
      assert(!(prompt&ENABLE_MOUSE_INPUT));assert(table&ENABLE_MOUSE_INPUT);
      assert((prompt^table)==ENABLE_MOUSE_INPUT);
      assert(!(prompt&(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT|ENABLE_QUICK_EDIT_MODE)));
      assert(prompt&ENABLE_WINDOW_INPUT);
      assert(ps_windows_input_mode(table,0)==prompt);
    }
#endif
    return 0;
}
