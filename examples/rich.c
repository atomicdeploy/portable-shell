/* Optional hosted demonstration; reusable renderer stays freestanding. */
#include "portable_rich.h"
#include <stdio.h>
#include <string.h>
static void output(void *ctx,const unsigned char *p,size_t n){
    (void)ctx;
#ifdef _WIN32
    while(n--){if(*p!='\r'||!n||p[1]!='\n')fputc(*p,stdout);++p;}
#else
    fwrite(p,1,n,stdout);
#endif
}
int main(int argc,char **argv){
    ps_rich r={output,0,PS_COLOR_NONE,0,0};int i;
    for(i=1;i<argc;++i){
        if(!strcmp(argv[i],"--truecolor"))r.color=PS_COLOR_RGB;
        else if(!strcmp(argv[i],"--256"))r.color=PS_COLOR_256;
        else if(!strcmp(argv[i],"--16"))r.color=PS_COLOR_16;
        else if(!strcmp(argv[i],"--plain"))r.color=PS_COLOR_NONE;
        else {puts("PortableShellText [--plain|--16|--256|--truecolor]");return 2;}
    }
    ps_rich_color(&r,0x56b6c2,0);ps_rich_attributes(&r,PS_TEXT_BOLD);
    ps_rich_wrap(&r,"PortableShell rich text",64,PS_ALIGN_CENTER);ps_rich_reset(&r);
    ps_rich_wrap_indented(&r,"Width-aware text can be left aligned, centered, right aligned or justified. The producer owns terminal capability selection.",60,PS_ALIGN_FULL,2);
    ps_rich_text(&r,"中文  日本語  한국어  café  🚀",ps_text_length("中文  日本語  한국어  café  🚀"),64);
    ps_rich_literal(&r,"\r\n");
#if PS_RICH_MARKUP
    ps_rich_markup(&r,"[#98c379]Ready[/]  [#e5c07b]Pending[/]  [#e06c75]Failed[/]\n");
#endif
#if PS_RICH_SYNTAX
    ps_rich_code(&r,"{\"active\": true, \"progress\": 73}\n");
#endif
#if PS_RICH_DOCUMENT
    ps_rich_document(&r,"# Integration\n- Caller-owned byte transport\n- Optional rendering helpers\n> No OS calls in the renderer",64);
#endif
    return ferror(stdout)?1:0;
}
