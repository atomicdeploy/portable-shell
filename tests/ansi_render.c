/* Host-only evidence tool: render captured terminal bytes, never synthesize UI. */
#include <stdio.h>
#include "render.c"
int main(int argc, char **argv) {
    static char svg[PS_REPLAY_SVG_CAPACITY];
    unsigned char input[1024];size_t count;unsigned size;
    FILE *source,*target;
    /* A tmux capture-pane snapshot has LF-delimited rows, unlike a raw VT log. */
    int snapshot=argc==4 && argv[3][0]=='-' && argv[3][1]=='s' && !argv[3][2];
    if(argc!=3&&!snapshot)return 2;
    source=fopen(argv[1],"rb");if(!source)return 3;
    ps_replay_reset();
    while((count=fread(input,1,sizeof(input),source))!=0){
        size_t i;
        if(!snapshot)ps_replay_feed(input,(unsigned)count);
        else for(i=0;i<count;++i){if(input[i]=='\n')ps_replay_feed((const unsigned char*)"\r",1);ps_replay_feed(input+i,1);}
    }
    if(ferror(source)){fclose(source);return 4;}fclose(source);
    size=ps_replay_svg(svg);target=fopen(argv[2],"wb");if(!target)return 5;
    if(fwrite(svg,1,size,target)!=size){fclose(target);return 6;}
    return fclose(target)!=0;
}
