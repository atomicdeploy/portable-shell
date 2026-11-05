#define PS_ANIMATION_TEST
#include "../tools/terminal_animation.c"
#include "../tools/terminal_recording.h"
#include <assert.h>
#define ARRAY_COUNT(array) (sizeof(array)/sizeof((array)[0]))
static void event(FILE *f,unsigned time,char type,const char *text){
    assert(ps_record_event(f,time,type,text,(unsigned)strlen(text)));
}
static char xml[65536];
static void exported(double speed,int loop,int blink,int poster){
    FILE *f=tmpfile();size_t n;assert(f);assert(write_svg(f,speed,loop,blink,poster,0));
    rewind(f);n=fread(xml,1,sizeof xml-1,f);assert(!ferror(f)&&feof(f));xml[n]=0;fclose(f);
}
int main(void){
    FILE *f=tmpfile();unsigned a,b;assert(f);assert(ps_record_begin(f,20,6));
    event(f,0,'O',"\033[2J\033[H\033[5 q\033]0;invisible title\007A<&");event(f,0,'F',"");
    event(f,500,'I',"SECRET INPUT MUST NOT APPEAR");event(f,500,'F',"");
    event(f,1000,'O',"\033[?25l\rB");event(f,1000,'F',"");
    event(f,1500,'O',"\033[?25h\rA\033[1;4H");event(f,1500,'F',"");
    event(f,2000,'E',"");rewind(f);assert(read_recording(f));fclose(f);
    assert(duration==2000&&checkpoint_count==3&&input_count==1);
    a=checkpoints[0].frame;b=checkpoints[1].frame;
    assert(a==checkpoints[2].frame&&a!=b&&frame_count==3); /* includes initial blank */
    assert(frames[a].shape==5&&frames[a].visible&&!frames[b].visible);
    assert(!export_drawings("missing-animation-output-directory"));
    assert(!import_drawings("missing-animation-output-directory"));
    assert(strstr(frames[a].drawing,"&lt;")&&strstr(frames[a].drawing,"&amp;"));
    assert(!strstr(frames[a].drawing,"invisible title"));
    exported(2,0,1,0);assert(strstr(xml,"dur=\"1000.000000ms\""));
    assert(strstr(xml,"keyTimes=\"0.000000000;0.500000000;0.750000000;1\""));
    assert(strstr(xml,"repeatCount=\"1\"")&&strstr(xml,"prefers-reduced-motion"));
    assert(!strstr(xml,"SECRET")&&!strstr(xml,"<script"));
    exported(1,1,0,0);assert(strstr(xml,"repeatCount=\"indefinite\""));
    assert(!strstr(xml,"attributeName=\"opacity\""));
    exported(1,1,1,1);assert(!strstr(xml,"<animate")&&strstr(xml,"class=\"cursor\""));
    f=tmpfile();assert(f);assert(ps_record_begin(f,20,6));
    event(f,0,'O',"A");event(f,0,'F',"");
    event(f,100,'O',"\033[5G");event(f,100,'F',"");event(f,200,'E',"");
    rewind(f);assert(read_recording(f));fclose(f);
    assert(checkpoint_count==2&&frame_count==3);
    assert(frames[1].drawing==frames[2].drawing); /* Cursor motion shares cells. */
    exported(1,1,1,0);assert(!strstr(xml,"id=\"frame-2\""));
    f=tmpfile();assert(f);assert(ps_record_begin(f,20,6));
    event(f,0,'O',"\033[");event(f,0,'O',"3 q\033[1m\xe2");
    event(f,0,'O',"\x9d\xaf");event(f,0,'F',"");
    event(f,0,'O',"\033[6 q");event(f,0,'F',"");event(f,100,'E',"");
    rewind(f);assert(read_recording(f));fclose(f);
    assert(checkpoint_count==1&&frames[checkpoints[0].frame].shape==6);
    exported(1,1,1,0);assert(strstr(xml,"h 4 l 6 8 -6 8"));
    /* A steady bar has no blink animation; an overwritten same-time state
       can still exist in definitions, but never becomes a visible checkpoint. */
    assert(!strstr(xml,"values=\"visible;hidden"));
    {const char *bad[]={"", "PSREPLAY 1 999 24\n", "PSREPLAY 1 20 6\n0 O 3\na",
        "PSREPLAY 1 20 6\n10 F 0\n\n9 E 0\n\n", "PSREPLAY 1 20 6\n0 Z 0\n\n",
        "PSREPLAY 1 20 6\n0 O 1\nA\n100 E 0\n\n", "PSREPLAY 1 20 6\n0 E 0\n\n"};
        size_t i;for(i=0;i<ARRAY_COUNT(bad);++i){f=tmpfile();assert(f);fputs(bad[i],f);rewind(f);assert(!read_recording(f));fclose(f);}}
    release_frames();puts("Timed replay: timing, deduplication, cursor, OSC, fallback and malformed input checks passed");return 0;
}
