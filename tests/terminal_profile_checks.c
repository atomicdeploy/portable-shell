#include "portable_terminal_profiles.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static ps_profile_token tokens[4096];
static char strings[65536];
static ps_terminal_profile profiles[16];
static ps_profile_workspace workspace;
static size_t count;
static int parse(const char *json,unsigned format,unsigned platform) {
    memset(&workspace,0,sizeof workspace);workspace.tokens=tokens;workspace.token_capacity=4096;
    workspace.strings=strings;workspace.string_capacity=sizeof strings;count=99;
    return ps_terminal_profiles_parse(json,strlen(json),format,platform,"fixture/settings.json",&workspace,profiles,16,&count);
}
static void rejected(const char *json) {
    assert(!parse(json,PS_PROFILE_LIST_DEFAULTS,PS_PROFILE_WINDOWS));assert(count==0);assert(workspace.error);
}
static void list_defaults(void) {
    const char *json="\xef\xbb\xbf{ // settings\n"
        "\"defaultProfile\":\"{stable}\",\"disabledProfileSources\":[\"Dynamic.Test\"],"
        "\"profiles\":{\"defaults\":{\"hidden\":true,\"startingDirectory\":\"%USERPROFILE%\","
        "\"environment\":{\"BASE\":\"keep\",\"Path\":\"old\"}},\"list\":["
        "{\"guid\":\"{stable}\",\"name\":\"Power shell\",\"hidden\":false,"
        "\"commandline\":\"\\\"C:\\\\Program Files\\\\PowerShell\\\\pwsh.exe\\\" -NoLogo \\\"two words\\\" \\\"\\\" \\\\server\\\\share\","
        "\"environment\":{\"PATH\":null},\"startingDirectory\":null},"
        "{\"name\":\"Generated\",\"source\":\"Dynamic.Test\"},"
        "{\"name\":\"Explicit disabled\",\"disabled\":true,\"commandline\":\"cmd.exe /k echo %PATH%\"},]}}";
    assert(parse(json,PS_PROFILE_LIST_DEFAULTS,PS_PROFILE_WINDOWS));assert(count==3);
    assert(!strcmp(profiles[0].executable,"C:\\Program Files\\PowerShell\\pwsh.exe"));
    assert(profiles[0].argument_count==4);
    assert(!strcmp(profiles[0].arguments[0],"-NoLogo"));assert(!strcmp(profiles[0].arguments[1],"two words"));
    assert(!strcmp(profiles[0].arguments[2],""));assert(!strcmp(profiles[0].arguments[3],"\\server\\share"));
    assert((profiles[0].flags&(PS_PROFILE_DEFAULT|PS_PROFILE_CWD_INHERIT))==(PS_PROFILE_DEFAULT|PS_PROFILE_CWD_INHERIT));
    assert(!(profiles[0].flags&PS_PROFILE_HIDDEN));assert(profiles[0].environment_count==2);
    assert(profiles[0].environment[1].unset);assert(!strcmp(profiles[0].environment[0].value,"keep"));
    assert(profiles[1].flags&PS_PROFILE_UNRESOLVED);assert(profiles[1].flags&PS_PROFILE_DISABLED);assert(profiles[1].flags&PS_PROFILE_HIDDEN);
    assert(profiles[2].flags&PS_PROFILE_DISABLED);assert(profiles[2].flags&PS_PROFILE_UNRESOLVED);
    assert(!strcmp(profiles[2].arguments[2],"%PATH%"));
    {char id[256];strcpy(id,profiles[0].stable_id);assert(parse(json,1,1));assert(!strcmp(id,profiles[0].stable_id));}
}
static void named_map(void) {
    assert(parse("{\"terminal.integrated.defaultProfile.linux\":\"Zsh\","
        "\"terminal.integrated.env.linux\":{\"SHARED\":\"yes\",\"DROP\":\"old\"},"
        "\"terminal.integrated.cwd\":\"/work\",\"terminal.integrated.inheritEnv\":false,"
        "\"terminal.integrated.profiles.linux\":{"
        "\"Zsh\":{\"path\":\"/bin/zsh\",\"args\":[\"-l\",\"${env:HOME}\",\"; $(never execute)\"],\"env\":{\"DROP\":null},\"icon\":\"terminal\"},"
        "\"No Bash\":null,\"Alternative\":{\"path\":[\"/bin/fish\",\"/usr/bin/fish\"]},"
        "\"Dynamic\":{\"source\":\"PowerShell\"}}}",2,2));
    assert(count==4);assert(!strcmp(profiles[0].name,"Zsh"));assert(profiles[0].flags&PS_PROFILE_DEFAULT);
    assert(profiles[0].flags&PS_PROFILE_ENV_CLEAR);assert(!strcmp(profiles[0].cwd,"/work"));
    assert(!strcmp(profiles[0].arguments[2],"; $(never execute)"));assert(profiles[0].environment[1].unset);
    assert(profiles[1].flags&PS_PROFILE_DISABLED);assert(profiles[2].path_count==2);assert(!profiles[2].executable);
    assert(profiles[3].flags&PS_PROFILE_UNRESOLVED);
    assert(parse("{\"terminal.integrated.profiles.osx\":{\"فارسی \\uD83D\\uDE00\":{\"path\":\"/bin/zsh\"}}}",2,3));
    assert(!strcmp(profiles[0].name,"فارسی \xf0\x9f\x98\x80"));
    assert(parse("{\"terminal.integrated.profiles.windows\":{\"Quoted\":{\"path\":\"pwsh\",\"args\":\"-x \\\"a\\\\\\\"b\\\" \\\"\\\\\\\\\\\"\"}}}",2,1));
    assert(profiles[0].argument_count==3);assert(!strcmp(profiles[0].arguments[1],"a\"b"));assert(!strcmp(profiles[0].arguments[2],"\\"));
}
static void malformed(void) {
    static const char *const bad[]={"", "{}x", "{\"profiles\":[],,}","{\"profiles\":[],\"profiles\":[]}",
        "{\"profiles\":[{\"name\":\"a\",\"hidden\":\"false\"}]}",
        "{\"profiles\":[{\"name\":\"a\"},{\"name\":\"a\"}]}",
        "{\"profiles\":[{\"name\":\"\\u0000\"}]}","{\"profiles\":[{\"name\":\"\\uD800\"}]}",
        "{\"profiles\":[{\"name\":\"\\uDE00\"}]}","{\"profiles\":[{\"name\":\"\xc0\xaf\"}]}",
        "{\"profiles\":[],\"number\":01}","{\"profiles\":[],\"number\":1.}","{\"profiles\":[],\"number\":1e}",
        "{\"profiles\":[]}/*unclosed", "{\"profiles\":[{\"name\":\"x\",\"commandline\":\"\\\"unclosed\"}]}",
        "{\"profiles\":[{\"name\":\"x\",\"environment\":{\"BAD=KEY\":\"x\"}}]}",
        "{\"profiles\":{\"defaults\":false,\"list\":[]}}"};
    unsigned i;for(i=0;i<sizeof bad/sizeof *bad;++i)rejected(bad[i]);
    assert(!parse("{\"terminal.integrated.profiles.linux\":{\"x\":{\"path\":[]}}}",2,2));
    assert(!parse("{\"terminal.integrated.profiles.linux\":{\"x\":{\"args\":[1]}}}",2,2));
    {char deep[256];unsigned n=0;strcpy(deep,"{\"unknown\":");n=(unsigned)strlen(deep);
     for(i=0;i<40;++i)deep[n++]='[';
     deep[n++]='0';
     for(i=0;i<40;++i)deep[n++]=']';
     deep[n++]='}';deep[n]=0;rejected(deep);}
    assert(parse("{\"profiles\":[],\"unknown\":-1.25e+5}",1,1));
    assert(parse("{\"defaultProfile\":\"{ABC}\",\"profiles\":[{\"guid\":\"{abc}\",\"name\":\"Case\"}]}",1,1));
    assert(profiles[0].flags&PS_PROFILE_DEFAULT);
    assert(parse("{\"profiles\":[{\"name\":\"Admin\",\"elevate\":true}]}",1,1));
    assert(profiles[0].flags&PS_PROFILE_REQUIRES_ELEVATION);assert(profiles[0].flags&PS_PROFILE_UNRESOLVED);
    {char id[256];assert(parse("{\"terminal.integrated.profiles.windows\":{\"Same\":null},\"terminal.integrated.profiles.linux\":{\"Same\":null}}",2,1));
     strcpy(id,profiles[0].stable_id);
     assert(parse("{\"terminal.integrated.profiles.windows\":{\"Same\":null},\"terminal.integrated.profiles.linux\":{\"Same\":null}}",2,2));
     assert(strcmp(id,profiles[0].stable_id));}
}
static void bounds(void) {
    const char *json="{\"profiles\":[{\"name\":\"Good\"}]}";size_t n;
    assert(parse(json,1,1));workspace.string_capacity=3;n=10;
    assert(!ps_terminal_profiles_parse(json,strlen(json),1,1,"test",&workspace,profiles,16,&n));assert(n==0);
    workspace.string_capacity=sizeof strings;workspace.token_capacity=2;n=10;
    assert(!ps_terminal_profiles_parse(json,strlen(json),1,1,"test",&workspace,profiles,16,&n));assert(n==0);
    assert(parse(json,1,1));n=10;
    assert(!ps_terminal_profiles_parse(json,PS_PROFILE_MAX_INPUT+1,1,1,"test",&workspace,profiles,16,&n));assert(n==0);
    assert(!ps_terminal_profiles_parse(json,strlen(json),1,1,"test",&workspace,profiles,0,&n));
}
int main(void){list_defaults();named_map();malformed();bounds();puts("terminal profile checks passed");return 0;}
