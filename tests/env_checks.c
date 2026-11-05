#include "../host/env_manager.h"
#include "portable_util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void ps_replay_reset(void);
void ps_replay_feed(const unsigned char *,unsigned);
unsigned ps_replay_svg(char *);
const char *ps_replay_cell(unsigned,unsigned);
static env_document doc;
static env_manager manager;
static char svg[1000000];
static ps_u32 now=100;
static const char fixture[]=
    "# Service configuration - demonstration values only\n"
    "# Runtime\nAPP_NAME=\"Workbench\"\nAPP_ENV=development\nDEBUG=false # Enable only while diagnosing\n"
    "PORT=8080\nHOST=127.0.0.1\n\n"
    "# Storage\nDB_HOST=localhost\nDB_PORT=5432\nDB_NAME=workspace\nDB_USER=operator\n"
    "DB_PASSWORD=\"example-only\"\nPOOL_SIZE=8\n\n"
    "# Network and delivery\n# MAIL_ENABLED=true\nMAIL_HOST=localhost\nMAIL_PORT=2525\n"
    "TIMEOUT=30\nRETRIES=3\nREGION=local\nLOG_LEVEL=info\n"
    "LOG_LEVEL=debug # Duplicate intentionally retained\n"
    "WELCOME=\"Hello,\nworld!\"\n"
    "COLOR=auto\nCACHE_ENABLED=true\nCACHE_TTL=300\nWORKERS=4\n";
static void sink(void *ctx,const ps_byte *b,unsigned n){(void)ctx;ps_replay_feed(b,n);}
static void key(unsigned k){ps_key(manager.session,k);env_manager_dispatch(&manager);now+=120;ps_tick(manager.session,now);}
static int visible(const char *word){unsigned y,x;for(y=0;y<32;++y){char row[401];unsigned n=0;for(x=0;x<100;++x){const char *cell=ps_replay_cell(y,x);while(*cell&&n+1<sizeof row)row[n++]=*cell++;}row[n]=0;if(strstr(row,word))return 1;}return 0;}
static void type(const char *s){while(*s)key((unsigned char)*s++);}
static void capture(const char *directory,const char *name){char path[1024];FILE *f;unsigned n;if(!directory)return;
    snprintf(path,sizeof path,"%s/%s.svg",directory,name);f=fopen(path,"wb");assert(f);n=ps_replay_svg(svg);assert(n);assert(fwrite(svg,1,n,f)==n);assert(!fclose(f));}
static unsigned find_key(const char *keyname){unsigned i;for(i=0;i<manager.document.count;++i)if(env_find(&manager.document,i,keyname))return i;assert(0);return 0;}
static void model_checks(void){
    const char *text="\xef\xbb\xbf# heading\r\n export KEY = \"a # b\"  # note\r\n# OFF=42\r\n\r\nKEY=second\r\nX='one\r\ntwo'\r\nweird syntax\r\nLAST=no-newline";
    assert(env_load(&doc,text,(unsigned)strlen(text)));assert(!strcmp(doc.text,text));assert(doc.bom==3&&doc.crlf&&doc.count==8);
    assert(doc.records[1].flags&ENV_DUPLICATE);assert(doc.records[4].flags&ENV_DUPLICATE);
    assert(doc.records[5].flags&ENV_MULTILINE);assert(doc.records[6].flags&ENV_UNKNOWN);
    assert(env_set_value(&doc,1,"\"changed # literal\""));assert(strstr(doc.text," export KEY = \"changed # literal\"  # note\r\n"));
    assert(env_dirty(&doc));assert(env_undo(&doc));assert(!strcmp(doc.text,text));assert(!env_dirty(&doc));
    assert(env_toggle(&doc,2));assert(strstr(doc.text,"\r\nOFF=42\r\n"));assert(env_undo(&doc));assert(!strcmp(doc.text,text));
    assert(env_toggle(&doc,5));assert(doc.records[5].flags&ENV_DISABLED);assert(doc.count==8);
    assert(env_toggle(&doc,5));assert(!strcmp(doc.text,text));
    assert(env_move(&doc,2,-1));assert(strstr(doc.text,"# heading\r\n# OFF=42\r\n export"));assert(env_undo(&doc));assert(!strcmp(doc.text,text));
    assert(!env_move(&doc,7,-1));assert(!strcmp(doc.text,text));
    assert(!env_set_value(&doc,1,"bad\nINJECTED=true"));assert(!strcmp(doc.text,text));
    assert(!env_set_value(&doc,1,"x # new comment"));assert(!strcmp(doc.text,text));
    assert(!env_replace(&doc,3,3,"\033[2J",4));assert(!strcmp(doc.text,text));
    assert(!env_replace(&doc,3,3,"\xc0\xaf",2));assert(!strcmp(doc.text,text));
    assert(env_load(&doc,"A=\"unfinished\nB=inside",PS_LITERAL_LENGTH("A=\"unfinished\nB=inside")));assert(doc.count==1&&doc.records[0].flags&ENV_UNKNOWN);
    assert(env_load(&doc,"A=\nB=hello#literal\nC=hi # note\n",PS_LITERAL_LENGTH("A=\nB=hello#literal\nC=hi # note\n")));assert(doc.records[0].value_size==0);
    assert(doc.records[1].value_size==13);assert(doc.records[2].value_size==2);
    assert(env_load(&doc,"EMPTY= # note\n",PS_LITERAL_LENGTH("EMPTY= # note\n")));
    assert(env_set_value(&doc,0,"filled"));assert(!strcmp(doc.text,"EMPTY= filled # note\n"));
    assert(!env_set_value(&doc,0,"\"bad quote"));assert(!strcmp(doc.text,"EMPTY= filled # note\n"));
    assert(env_undo(&doc));assert(!strcmp(doc.text,"EMPTY= # note\n"));
}
static void file_checks(void){
    const char *path="env-manager-test.tmp",*error=0;FILE *f;unsigned n;
    assert(env_file_create(path,&error));assert(!env_file_create(path,&error));
    f=fopen(path,"wb");assert(f);fputs("KEY=one\r\n# retained\r\n",f);assert(!fclose(f));
    assert(env_file_read(path,doc.scratch,&n,&error));assert(env_load(&doc,doc.scratch,n));
    assert(env_set_value(&doc,0,"two"));assert(env_file_save(path,&doc,&error));assert(!env_dirty(&doc));
    assert(env_file_read(path,doc.scratch,&n,&error));assert(!strcmp(doc.scratch,"KEY=two\r\n# retained\r\n"));
    assert(env_set_value(&doc,0,"three"));f=fopen(path,"wb");assert(f);fputs("KEY=external\n",f);fclose(f);
    assert(!env_file_save(path,&doc,&error));assert(env_dirty(&doc));
    assert(env_file_read(path,doc.scratch,&n,&error));assert(!strcmp(doc.scratch,"KEY=external\n"));assert(!remove(path));
}
int main(int argc,char **argv){
    ps_config config={sink,0,100,32,PS_INTERACTIVE|PS_COLOR|PS_TRUECOLOR|PS_UNICODE};
    void *storage;ps_session *s;const char *directory=argc>1?argv[1]:0;unsigned i;
    model_checks();file_checks();storage=calloc(1,ps_sizeof_session());assert(storage);s=ps_init(storage,ps_sizeof_session(),&config);assert(s);
    ps_replay_reset();assert(env_manager_open(&manager,s,"configuration.env",fixture,(unsigned)strlen(fixture)));ps_tick(s,100);
    assert(visible("Line")&&visible("Key")&&visible("Value / Source"));
    assert(!strcmp(manager.controls[0][0].group,"3"));
    assert(!strcmp(manager.controls[0][0].label,"APP_NAME"));
    capture(directory,"env-values");
    assert(ps_panel_select(s,find_key("PORT")));key(13);key(21);type("9090");key(13);
    assert(strstr(manager.document.text,"PORT=9090\n"));assert(env_dirty(&manager.document));
    key(26);assert(strstr(manager.document.text,"PORT=8080\n"));
    assert(ps_panel_select(s,find_key("APP_NAME")));key(PS_KEY_F2);ps_tick(s,now+=200);assert(ps_panel_active(s));assert(!ps_panel_selection(s,&i));assert(visible("Apply")&&visible("Cancel"));capture(directory,"env-modal");key(27);
    key(9);ps_tick(s,now+=120);capture(directory,"env-source");
    key(PS_KEY_F3);type("MAIL");key(13);assert(!strcmp(manager.filter,"MAIL"));capture(directory,"env-filter");
    assert(ps_panel_select(s,find_key("MAIL_ENABLED")+PS_ENV_RECORDS));key(20);assert(strstr(manager.document.text,"\nMAIL_ENABLED=true\n"));
    key(5);assert(ps_editor_active(s));capture(directory,"env-record");key(27);assert(ps_panel_active(s));
    key(27);assert(ps_form_active(s));capture(directory,"env-unsaved");key(27);assert(manager.open&&ps_panel_active(s));
    key(14);type("NEW_KEY");key(13);type("value");key(13);assert(strstr(manager.document.text,"NEW_KEY=value\n"));
    key(PS_KEY_F1);assert(ps_editor_active(s));key('x');key(27);assert(ps_panel_active(s));
    ps_resize(s,48,18);ps_tick(s,now+=120);
    assert(visible("Line")&&visible("Key")&&visible("MAIL"));
    ps_resize(s,100,32);ps_tick(s,now+=120);
    /* Byte decoder paths, including fragmented escape input. */
    ps_feed(s,(const ps_byte *)"\033[",2);ps_feed(s,(const ps_byte *)"B",1);env_manager_dispatch(&manager);
    key(PS_KEY_F3);key(21);key(13);assert(!manager.filter[0]);
    for(i=0;i<manager.document.count;++i)assert(manager.document.records[i].end<=manager.document.size);
    ps_close(s);free(storage);
    printf("env document, disk conflict, inline/modal and navigation checks passed; model=%zu UI=%zu bytes\n",sizeof(env_document),sizeof(env_manager));return 0;
}
