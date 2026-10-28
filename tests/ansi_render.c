/* Host-only evidence tool: render captured terminal bytes, never synthesize UI. */
#include <stdio.h>
#include "render.c"
int main(int argc, char **argv) {
    static char svg[400000];
    unsigned char input[1024];size_t count;unsigned size;
    FILE *source,*target;
    if(argc!=3)return 2;
    source=fopen(argv[1],"rb");if(!source)return 3;
    ps_replay_reset();
    while((count=fread(input,1,sizeof(input),source))!=0)ps_replay_feed(input,(unsigned)count);
    if(ferror(source)){fclose(source);return 4;}fclose(source);
    size=ps_replay_svg(svg);target=fopen(argv[2],"wb");if(!target)return 5;
    if(fwrite(svg,1,size,target)!=size){fclose(target);return 6;}
    return fclose(target)!=0;
}
