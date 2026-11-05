/* Host-only timed ANSI -> SVG replay. No shell commands or input are executed.
   The shared test renderer owns terminal parsing and all cell drawing. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

enum { MAX_COLUMNS=240, MAX_ROWS=100, MAX_FRAMES=1024,
       MAX_RECORD_BYTES=1024*1024, MAX_CAPTURE_BYTES=64*1024*1024,
       MAX_DURATION_MS=86400000, MAX_FONT_BYTES=4*1024*1024,
       CELL_WIDTH=12, CELL_HEIGHT=26, MARGIN=32, BLINK_PERIOD_MS=1000 };
static unsigned replay_columns=96, replay_rows=24;
#define PS_REPLAY_ONLY
#define PS_REPLAY_ATTRIBUTES
#define PS_REPLAY_MAX_COLUMNS MAX_COLUMNS
#define PS_REPLAY_MAX_ROWS MAX_ROWS
#define PS_REPLAY_SVG_CAPACITY (8*1024*1024)
#define W replay_columns
#define H replay_rows
#include "../tests/render.c"

typedef struct {
    char *drawing;
    unsigned drawing_index;
    unsigned cursor_row, cursor_column, visible, shape;
} ReplayFrame;
typedef struct { unsigned time, frame; } ReplayCheckpoint;
static ReplayFrame frames[MAX_FRAMES];
static ReplayCheckpoint checkpoints[MAX_FRAMES];
static unsigned frame_count, checkpoint_count, duration, input_count;
static size_t retained_bytes;
static char rendered[PS_REPLAY_SVG_CAPACITY];

static void release_frames(void) {
    unsigned i;
    for(i=0;i<frame_count;++i)if(frames[i].drawing_index==i)free(frames[i].drawing);
    frame_count=checkpoint_count=duration=input_count=0;retained_bytes=0;
}
static int checkpoint(unsigned time) {
    unsigned i, drawing_index=frame_count, n=ps_replay_svg(rendered);
    if(svg_overflow)return 0;
    for(i=0;i<frame_count;++i){
        if(strcmp(frames[i].drawing,rendered))continue;
        drawing_index=frames[i].drawing_index;
        if(frames[i].cursor_row==row && frames[i].cursor_column==col &&
           frames[i].visible==cursor_visible && frames[i].shape==cursor_shape)break;
    }
    if(i==frame_count){
        ReplayFrame *f;
        if(frame_count==MAX_FRAMES)return 0;
        f=&frames[frame_count];f->drawing_index=drawing_index;
        if(drawing_index==frame_count){
            if(retained_bytes+n+1>MAX_CAPTURE_BYTES)return 0;
            f->drawing=(char *)malloc(n+1);if(!f->drawing)return 0;
            memcpy(f->drawing,rendered,n+1);retained_bytes+=n+1;
        }else f->drawing=frames[drawing_index].drawing;
        f->cursor_row=row;f->cursor_column=col;f->visible=cursor_visible;f->shape=cursor_shape;
        ++frame_count;
    }
    if(checkpoint_count && checkpoints[checkpoint_count-1].time==time){
        checkpoints[checkpoint_count-1].frame=i;return 1;
    }
    if(checkpoint_count && checkpoints[checkpoint_count-1].frame==i)return 1;
    if(checkpoint_count==MAX_FRAMES)return 0;
    checkpoints[checkpoint_count].time=time;checkpoints[checkpoint_count++].frame=i;
    return 1;
}
static int unsigned_field(const char **p,unsigned *value) {
    char *end;unsigned long v;
    if(**p<'0'||**p>'9')return 0;
    errno=0;v=strtoul(*p,&end,10);
    if(errno || v>MAX_DURATION_MS)return 0;
    *value=(unsigned)v;*p=end;return 1;
}
static int line_end(const char *p){return !strcmp(p,"\n") || !strcmp(p,"\r\n");}
static int separator(const char **p){if(**p!=' ')return 0;++*p;return 1;}
static int read_recording(FILE *in) {
    char header[128];unsigned version, columns, rows, last_time=0, total=0;
    const char *p;int ended=0, dirty=0;
    release_frames();
    if(!fgets(header,sizeof header,in) || strncmp(header,"PSREPLAY ",9))return 0;
    p=header+9;
    if(!unsigned_field(&p,&version)||!separator(&p)||!unsigned_field(&p,&columns)||
       !separator(&p)||!unsigned_field(&p,&rows)||!line_end(p)||version!=1||
       columns<1||columns>MAX_COLUMNS||rows<1||rows>MAX_ROWS)return 0;
    replay_columns=columns;replay_rows=rows;ps_replay_reset();
    if(!checkpoint(0))return 0;
    while(fgets(header,sizeof header,in)){
        unsigned time,bytes;char type;unsigned char buffer[4096];
        p=header;
        if(ended || !unsigned_field(&p,&time)||!separator(&p))return 0;
        if(*p!='O'&&*p!='I'&&*p!='F'&&*p!='E')return 0;
        type=*p++;
        if(!separator(&p)||!unsigned_field(&p,&bytes)||!line_end(p)||time<last_time||
           bytes>MAX_RECORD_BYTES||total>MAX_CAPTURE_BYTES-bytes)return 0;
        last_time=time;total+=bytes;
        if((type=='F'||type=='E')&&bytes)return 0;
        if(type=='I')++input_count;
        if(type=='O')dirty=1;
        while(bytes){
            unsigned take=bytes>sizeof buffer?(unsigned)sizeof buffer:bytes;
            if(fread(buffer,1,take,in)!=take)return 0;
            if(type=='O')ps_replay_feed(buffer,take);
            bytes-=take;
        }
        if(fgetc(in)!='\n')return 0;
        if(type=='F'){if(!checkpoint(time))return 0;dirty=0;}
        if(type=='E'){
            if(dirty||!time||time<=checkpoints[checkpoint_count-1].time)return 0;
            duration=time;ended=1;
        }
    }
    return ended&&!ferror(in);
}
static void cursor_svg(FILE *out,const ReplayFrame *f,int blink) {
    unsigned x=MARGIN+f->cursor_column*CELL_WIDTH, y=MARGIN+f->cursor_row*CELL_HEIGHT;
    unsigned width=CELL_WIDTH,height=CELL_HEIGHT;
    if(!f->visible||f->cursor_row>=replay_rows||f->cursor_column>=replay_columns)return;
    if(f->shape==3||f->shape==4){height=2;y+=CELL_HEIGHT-height;}
    if(f->shape==5||f->shape==6)width=2;
    fprintf(out,"<rect class=\"cursor\" x=\"%u\" y=\"%u\" width=\"%u\" height=\"%u\" fill=\"#abb2bf\" fill-opacity=\"0.7\">",x,y,width,height);
    if(blink && (f->shape==0 || (f->shape&1)))
        fprintf(out,"<animate attributeName=\"opacity\" values=\"1;0;1\" keyTimes=\"0;0.5;1\" calcMode=\"discrete\" dur=\"%ums\" repeatCount=\"indefinite\"/>",BLINK_PERIOD_MS);
    fputs("</rect>",out);
}
static void drawing_svg(FILE *out,const ReplayFrame *f) {
    const char *start=strchr(strstr(f->drawing,"<svg"),'>')+1;
    size_t n=(size_t)(strstr(start,"</svg>")-start);
    fwrite(start,1,n,out);
}
/* Conversion is a host build step. Assemble converted drawings only after the
   static SVG converter has finished, so it never strips timing or cursor SMIL. */
static int drawing_path(char *path,size_t capacity,const char *directory,unsigned index) {
    int n=snprintf(path,capacity,"%s/frame-%u.svg",directory,index);
    return n>0&&(size_t)n<capacity;
}
static int export_drawings(const char *directory) {
    unsigned i;char path[4096];FILE *manifest;
    int n=snprintf(path,sizeof path,"%s/frames.txt",directory);
    if(n<0||(size_t)n>=sizeof path)return 0;
    manifest=fopen(path,"wb");if(!manifest)return 0;
    for(i=0;i<frame_count;++i)if(frames[i].drawing_index==i){
        FILE *out;int bad;
        if(!drawing_path(path,sizeof path,directory,i)){fclose(manifest);return 0;}
        out=fopen(path,"wb");if(!out){fclose(manifest);return 0;}
        bad=fputs(frames[i].drawing,out)==EOF;
        if(fclose(out))bad=1;
        if(bad||fprintf(manifest,"frame-%u.svg\n",i)<0){fclose(manifest);return 0;}
    }
    return fclose(manifest)==0;
}
static int import_drawings(const char *directory) {
    unsigned i,j;char path[4096];
    for(i=0;i<frame_count;++i)if(frames[i].drawing_index==i){
        FILE *in;long size;char *drawing,*root,*end;size_t old=strlen(frames[i].drawing)+1;
        if(!drawing_path(path,sizeof path,directory,i))return 0;
        in=fopen(path,"rb");if(!in)return 0;
        if(fseek(in,0,SEEK_END)||(size=ftell(in))<=0||size>=PS_REPLAY_SVG_CAPACITY||
           retained_bytes-old+(size_t)size+1>MAX_CAPTURE_BYTES||fseek(in,0,SEEK_SET)){
            fclose(in);return 0;
        }
        drawing=(char *)malloc((size_t)size+1);if(!drawing){fclose(in);return 0;}
        if(fread(drawing,1,(size_t)size,in)!=(size_t)size||ferror(in)){
            free(drawing);fclose(in);return 0;
        }
        fclose(in);drawing[size]=0;
        root=strstr(drawing,"<svg");end=root?strchr(root,'>'):0;
        /* This option accepts trusted static converter output, never a font or
           arbitrary webpage. Missing outlines fail rather than silently falling back. */
        if(!end||!strstr(end,"</svg>")||strstr(drawing,"<text")||strstr(drawing,"@font-face")||
           strstr(drawing,"<script")||strstr(drawing,"<image")||strstr(drawing,"<foreignObject")){
            free(drawing);return 0;
        }
        free(frames[i].drawing);retained_bytes=retained_bytes-old+(size_t)size+1;
        for(j=i;j<frame_count;++j)if(frames[j].drawing_index==i)frames[j].drawing=drawing;
    }
    return 1;
}
static int embedded_font(FILE *out,const char *path) {
    static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned char b[3];size_t n,total=0;FILE *in;
    if(!path)return 1;
    in=fopen(path,"rb");if(!in)return 0;
    fputs("<style>@font-face{font-family:Agave;src:url(data:font/ttf;base64,",out);
    while((n=fread(b,1,sizeof b,in))!=0){
        total+=n;if(total>MAX_FONT_BYTES){fclose(in);return 0;}
        fputc(alphabet[b[0]>>2],out);
        fputc(alphabet[((b[0]&3)<<4)|(n>1?b[1]>>4:0)],out);
        fputc(n>1?alphabet[((b[1]&15)<<2)|(n>2?b[2]>>6:0)]:'=',out);
        fputc(n>2?alphabet[b[2]&63]:'=',out);
    }
    {int bad=ferror(in)||!total;fclose(in);if(bad)return 0;}
    fputs(") format('truetype')}</style>",out);return 1;
}
static int write_svg(FILE *out,double speed,int loop,int blink,int poster,const char *font) {
    unsigned i,j,last=checkpoints[checkpoint_count-1].frame;
    fprintf(out,"<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"%u\" height=\"%u\" viewBox=\"0 0 %u %u\" role=\"img\" aria-labelledby=\"title description\">\n",
            replay_columns*CELL_WIDTH+2*MARGIN,replay_rows*CELL_HEIGHT+2*MARGIN,
            replay_columns*CELL_WIDTH+2*MARGIN,replay_rows*CELL_HEIGHT+2*MARGIN);
    fputs("<title id=\"title\">PortableShell terminal replay</title><desc id=\"description\">Actual emitted terminal output. Recorded interactions are replayed, not executed. A static final frame is available when animation is unsupported or reduced motion is requested.</desc>\n",out);
    if(!embedded_font(out,font))return 0;
    if(poster){drawing_svg(out,&frames[last]);cursor_svg(out,&frames[last],0);}
    else {
        fprintf(out,"<!-- %u unique frames, %u visual checkpoints, %u input events, %u ms -->\n",frame_count,checkpoint_count,input_count,duration);
        fputs("<style>@media(prefers-reduced-motion:reduce){.replay{display:none}}</style><defs>\n",out);
        for(i=0;i<frame_count;++i){
            if(frames[i].drawing_index!=i)continue;
            fprintf(out,"<g id=\"frame-%u\">",i);drawing_svg(out,&frames[i]);fputs("</g>\n",out);
        }
        fputs("</defs>\n",out);
        fprintf(out,"<g class=\"fallback\"><use xlink:href=\"#frame-%u\"/>",frames[last].drawing_index);
        cursor_svg(out,&frames[last],0);fputs("</g>\n<g class=\"replay\">\n",out);
        for(i=0;i<frame_count;++i){
            fprintf(out,"<g visibility=\"hidden\" data-frame=\"%u\"><use xlink:href=\"#frame-%u\"/>",i,frames[i].drawing_index);
            cursor_svg(out,&frames[i],blink);
            fprintf(out,"<animate attributeName=\"visibility\" calcMode=\"discrete\" dur=\"%.6fms\" repeatCount=\"%s\" fill=\"freeze\" keyTimes=\"",duration/speed,loop?"indefinite":"1");
            for(j=0;j<checkpoint_count;++j)fprintf(out,"%s%.9f",j?";":"",(double)checkpoints[j].time/duration);
            fputs(";1\" values=\"",out);
            for(j=0;j<checkpoint_count;++j)fprintf(out,"%s%s",j?";":"",checkpoints[j].frame==i?"visible":"hidden");
            fprintf(out,";%s\"/></g>\n",last==i?"visible":"hidden");
        }
        fputs("</g>\n",out);
    }
    fputs("</svg>\n",out);return !ferror(out);
}
#ifndef PS_ANIMATION_TEST
static int export_file(const char *path,double speed,int loop,int blink,int poster,const char *font){
    FILE *out=fopen(path,"wb");int good;
    if(!out){perror(path);return 0;}
    good=write_svg(out,speed,loop,blink,poster,font);
    if(fclose(out))good=0;
    if(!good)fprintf(stderr,"Could not export %s (I/O or font error).\n",path);
    return good;
}
int main(int argc,char **argv) {
    int i,loop=1,blink=1,good;double speed=1;const char *font=0,*poster=0,*export_dir=0,*import_dir=0;FILE *in;
    if(argc<3){
        fputs("Usage: terminal_animation recording.psrec output.svg [--once] [--speed N] [--no-blink] [--font Agave.ttf] [--poster static.svg] [--export-drawings DIR] [--drawings DIR]\n",stderr);return 2;
    }
    for(i=3;i<argc;++i){
        if(!strcmp(argv[i],"--once"))loop=0;
        else if(!strcmp(argv[i],"--no-blink"))blink=0;
        else if(!strcmp(argv[i],"--speed")&&i+1<argc){
            char *end;speed=strtod(argv[++i],&end);
            if(*end||!isfinite(speed)||speed<0.01||speed>100)return 2;
        }else if(!strcmp(argv[i],"--font")&&i+1<argc)font=argv[++i];
        else if(!strcmp(argv[i],"--poster")&&i+1<argc)poster=argv[++i];
        else if(!strcmp(argv[i],"--export-drawings")&&i+1<argc)export_dir=argv[++i];
        else if(!strcmp(argv[i],"--drawings")&&i+1<argc)import_dir=argv[++i];
        else {fprintf(stderr,"Unknown or incomplete option: %s\n",argv[i]);return 2;}
    }
    if((import_dir&&(export_dir||font))||!strcmp(argv[1],argv[2]) || (poster&&(!strcmp(poster,argv[1])||!strcmp(poster,argv[2]))))return 2;
    in=fopen(argv[1],"rb");if(!in){perror(argv[1]);return 3;}
    good=read_recording(in);fclose(in);
    if(!good){fputs("Invalid, incomplete, or oversized PSREPLAY recording.\n",stderr);release_frames();return 4;}
    if((export_dir&&!export_drawings(export_dir))||(import_dir&&!import_drawings(import_dir))){
        fputs("Could not export/import static outlined drawings.\n",stderr);release_frames();return 5;
    }
    good=export_file(argv[2],speed,loop,blink,0,font);
    if(good&&poster)good=export_file(poster,speed,loop,blink,1,font);
    if(good)fprintf(stderr,"%u unique frames, %u visual checkpoints, %u input events; %.3f seconds\n",frame_count,checkpoint_count,input_count,duration/speed/1000);
    release_frames();return good?0:5;
}
#endif
