#include "portable_terminal_profile_host.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef PS_PROFILE_FIXTURES
#define PS_PROFILE_FIXTURES "tests/fixtures"
#endif
static unsigned received,errors,cancel;
static int emit(void *context,const ps_terminal_profile *p) {
    (void)context;assert(p->name&&p->stable_id&&p->provenance);
    assert(strstr(p->provenance,"terminal-profiles.jsonc"));
    if(cancel)return 0;
    if(!received){assert(!strcmp(p->executable,"never-launch.exe"));assert(p->flags&PS_PROFILE_DEFAULT);}
    ++received;return 1;
}
static void error(void *context,const char *path,const char *message) {
    (void)context;assert(path&&message&&*message);++errors;
}
int main(void) {
    ps_profile_scan_result result=ps_terminal_profiles_import_file(PS_PROFILE_FIXTURES "/terminal-profiles.jsonc",1,1,emit,0,error,0);
    assert(result.files_read==1&&result.profiles_emitted==2&&received==2&&!errors);
    received=0;
    result=ps_terminal_profiles_import_file(PS_PROFILE_FIXTURES "/terminal-profiles-invalid.jsonc",1,1,emit,0,error,0);
    assert(result.errors==1&&errors==1&&!received&&!result.profiles_emitted);
    result=ps_terminal_profiles_import_file(PS_PROFILE_FIXTURES "/missing-terminal-profiles.jsonc",1,1,emit,0,error,0);
    assert(!result.errors&&!result.files_read&&!received);
    cancel=1;result=ps_terminal_profiles_import_file(PS_PROFILE_FIXTURES "/terminal-profiles.jsonc",1,1,emit,0,error,0);
    assert(result.cancelled&&!result.profiles_emitted&&!received);
    puts("terminal profile host checks passed");return 0;
}
