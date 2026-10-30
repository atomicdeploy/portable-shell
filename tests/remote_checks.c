#include "../examples/remote_device.h"
#include <assert.h>
#include <string.h>
static unsigned char output[1024];static unsigned used;
static void sink(void *ctx,const unsigned char *bytes,unsigned n){(void)ctx;assert(n+used<sizeof output);memcpy(output+used,bytes,n);used+=n;output[used]=0;}
int main(void){
    DemoDevice device={{0},0,0};ps_remote_provider provider=demo_provider(&device);
    assert(ps_remote_dispatch(&provider,"GET","/list",0,0,sink,0)==200);
    assert(strstr((char*)output,"notes.txt"));used=0;
    assert(ps_remote_dispatch(&provider,"PUT","/files/notes.txt",(const unsigned char*)"hello",5,sink,0)==200);
    assert(ps_remote_dispatch(&provider,"GET","/files/notes.txt",0,0,sink,0)==200);
    assert(!strcmp((char*)output,"hello"));used=0;
    assert(ps_remote_dispatch(&provider,"POST","/command",(const unsigned char*)"enable",6,sink,0)==200);
    assert(!strcmp((char*)output,"enabled\n"));
    assert(ps_remote_dispatch(&provider,"GET","/files/../secret",0,0,sink,0)==400);
    assert(ps_remote_dispatch(&provider,"GET","/files/%2e%2e/secret",0,0,sink,0)==400);
    assert(ps_remote_dispatch(&provider,"GET","/files/a\\b",0,0,sink,0)==400);
    assert(ps_remote_dispatch(&provider,"DELETE","/files/notes.txt",0,0,sink,0)==405);
    assert(ps_remote_dispatch(&provider,"POST","/command",(const unsigned char*)"system",6,sink,0)==404);
    provider.write=0;assert(ps_remote_dispatch(&provider,"PUT","/files/notes.txt",0,0,sink,0)==501);
    return 0;
}
