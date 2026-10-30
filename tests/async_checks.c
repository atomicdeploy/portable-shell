#include "portable_async.h"
#if PS_ASYNC && PS_ASYNC_QUEUE
#include "../examples/async_command.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef struct { unsigned steps,done,stopped,state; int result; ps_async *pool; } probe;
static unsigned step(void *p,uint32_t now,int *result) {
    probe *v=(probe *)p;(void)now;++v->steps;
    assert(!ps_async_poll(v->pool,now,1));
    *result=42;return v->steps==2?PS_ASYNC_DONE:PS_ASYNC_PENDING;
}
static void stop(void *p,unsigned reason) { probe *v=(probe *)p;++v->stopped;v->state=reason; }
static void done(void *p,uint32_t id,unsigned state,int result) {
    probe *v=(probe *)p;assert(id);++v->done;v->state=state;v->result=result;
    assert(!ps_async_submit(v->pool,0,0,0,0,0,0));
}
static char output[8192];static unsigned used;
static void sink(void *p,const ps_byte *bytes,unsigned n) {
    (void)p;assert(n<sizeof output-used);memcpy(output+used,bytes,n);used+=n;output[used]=0;
}
static union { void *alignment; unsigned char bytes[65536]; } storage;
int main(void) {
    ps_async pool;ps_async_job slots[2];uint32_t a,b;
    probe x={0,0,0,0,0,&pool},y={0,0,0,0,0,&pool};
    ps_byte_queue q;unsigned char bytes[5];const unsigned char *span;size_t n;
    ps_config config={sink,0,80,24,PS_NO_BUILTINS};ps_session *shell;
    async_command_example app;
    assert(ps_async_init(&pool,slots,2));
    a=ps_async_submit(&pool,0,0,step,stop,done,&x);
    b=ps_async_submit(&pool,0,0,step,stop,done,&y);
    assert(a && b && !ps_async_submit(&pool,0,0,0,0,0,0));
    assert(!ps_async_poll(&pool,1,0));
    assert(ps_async_poll(&pool,1,1)==1 && x.steps==1 && y.steps==0);
    assert(ps_async_poll(&pool,2,1)==1 && y.steps==1);
    ps_async_poll(&pool,3,2);assert(x.done==1 && y.done==1 && x.result==42);
    assert(!ps_async_complete(&pool,a,PS_ASYNC_DONE,0));
    a=ps_async_submit(&pool,UINT32_MAX-3,5,0,stop,done,&x);
    assert(a);ps_async_poll(&pool,0,2);assert(ps_async_pending(&pool)==1);
    ps_async_poll(&pool,1,2);assert(x.state==PS_ASYNC_TIMEOUT && x.stopped==1);
    a=ps_async_submit(&pool,2,0,0,stop,done,&x);
    assert(ps_async_complete(&pool,a,PS_ASYNC_FAILED,-9));
    assert(!ps_async_cancel(&pool,a));ps_async_poll(&pool,3,1);
    assert(x.state==PS_ASYNC_FAILED && x.result==-9 && x.stopped==1);
    a=ps_async_submit(&pool,4,0,0,stop,done,&x);
    assert(ps_async_cancel_all(&pool)==1);ps_async_poll(&pool,5,1);
    assert(x.state==PS_ASYNC_CANCELLED && x.stopped==2);
    b=ps_async_submit(&pool,5,0,0,0,done,&y);
    assert(b!=a && !ps_async_complete(&pool,a,PS_ASYNC_DONE,0));
    assert(ps_async_complete(&pool,b,PS_ASYNC_DONE,7));ps_async_poll(&pool,6,1);
    assert(y.result==7 && !ps_async_pending(&pool));
    pool.next_id=UINT32_MAX;
    assert(ps_async_submit(&pool,0,0,0,0,0,0)==UINT32_MAX);
    assert(!ps_async_submit(&pool,0,0,0,0,0,0));
    assert(ps_byte_queue_init(&q,bytes,sizeof bytes));
    assert(ps_byte_queue_write(&q,"abcde",5));
    assert(!ps_byte_queue_write(&q,"x",1) && q.count==5);
    assert(ps_byte_queue_consume(&q,3));
    assert(ps_byte_queue_write(&q,"XYZ",3));
    n=ps_byte_queue_peek(&q,&span);assert(n==2 && !memcmp(span,"de",2));
    assert(!ps_byte_queue_consume(&q,6));assert(ps_byte_queue_consume(&q,n));
    n=ps_byte_queue_peek(&q,&span);assert(n==3 && !memcmp(span,"XYZ",3));
    assert(ps_byte_queue_consume(&q,n) && q.count==0);
    assert(ps_sizeof_session()<=sizeof storage.bytes);
    shell=ps_init(storage.bytes,sizeof storage.bytes,&config);assert(shell);
    assert(example_async_init(&app,shell));
    assert(ps_execute(shell,"later \"deferred result\"")==0 && app.id);
    assert(ps_execute(shell,"later busy")==1);
    example_async_tick(&app,1);example_async_tick(&app,2);
    assert(!strstr(output,"deferred result"));
    example_async_tick(&app,3);
    assert(strstr(output,"deferred result") && !app.id);
    assert(ps_execute(shell,"later cancelled")==0);
    assert(ps_async_cancel(&app.pool,app.id));example_async_tick(&app,4);
    assert(!app.id && strstr(output,"cancelled or timed out"));
    ps_close(shell);
    puts("Async callbacks, deadlines, cancellation, fairness and queue checks passed");
    return 0;
}

#else
int main(void) { return 0; }
#endif
