#ifndef PS_REMOTE_DEVICE_EXAMPLE_H
#define PS_REMOTE_DEVICE_EXAMPLE_H
#include "portable_remote.h"
/* A RAM-backed firmware provider. Replace these callbacks with flash, SD, sensor
   or registered-command operations. No filesystem or OS is assumed. */
typedef struct {unsigned char note[256];unsigned size;int enabled;} DemoDevice;
static void demo_text(ps_remote_write_fn out,void *ctx,const char *p){unsigned n=0;while(p[n])++n;out(ctx,(const unsigned char*)p,n);}
static int demo_list(void *opaque,const char *path,const unsigned char *body,unsigned n,ps_remote_write_fn out,void *ctx){
    (void)opaque;(void)path;(void)body;(void)n;demo_text(out,ctx,"file\tnotes.txt\nfile\tstatus.txt\n");return 200;
}
static int demo_read(void *opaque,const char *path,const unsigned char *body,unsigned n,ps_remote_write_fn out,void *ctx){
    DemoDevice *d=opaque;(void)body;(void)n;
    if(ps_remote_equal(path,"notes.txt")){out(ctx,d->note,d->size);return 200;}
    if(ps_remote_equal(path,"status.txt")){demo_text(out,ctx,d->enabled?"enabled\n":"disabled\n");return 200;}return 404;
}
static int demo_write(void *opaque,const char *path,const unsigned char *body,unsigned n,ps_remote_write_fn out,void *ctx){
    DemoDevice *d=opaque;unsigned i;(void)out;(void)ctx;
    if(!ps_remote_equal(path,"notes.txt"))return 404;if(n>sizeof d->note)return 413;
    for(i=0;i<n;++i)d->note[i]=body[i];d->size=n;return 200;
}
static int demo_execute(void *opaque,const char *path,const unsigned char *body,unsigned n,ps_remote_write_fn out,void *ctx){
    DemoDevice *d=opaque;char command[32];unsigned i;(void)path;
    if(n>=sizeof command)return 413;for(i=0;i<n;++i){if(!body[i])return 400;command[i]=(char)body[i];}command[n]=0;
    if(ps_remote_equal(command,"enable"))d->enabled=1;
    else if(ps_remote_equal(command,"disable"))d->enabled=0;
    else if(!ps_remote_equal(command,"status"))return 404;
    demo_text(out,ctx,d->enabled?"enabled\n":"disabled\n");return 200;
}
static ps_remote_provider demo_provider(DemoDevice *device){ps_remote_provider p={device,demo_list,demo_read,demo_write,demo_execute};return p;}
#endif
