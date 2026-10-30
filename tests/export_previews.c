/* Host-only export of the current C runtime's actual emitted-byte fixtures. */
#include <stdio.h>
extern int render_entry(void);
extern char preview_tasks[],preview_controls[],preview_control_inline[],preview_control_dialog[];
extern char preview_red[],preview_lumen[],preview_onedark[],preview_ascii[];
extern unsigned preview_tasks_n,preview_controls_n,preview_control_inline_n,preview_control_dialog_n;
extern unsigned preview_red_n,preview_lumen_n,preview_onedark_n,preview_ascii_n;
int main(int argc,char **argv){
    unsigned i;char *frames[]={preview_tasks,preview_controls,preview_control_inline,preview_control_dialog,
        preview_red,preview_lumen,preview_onedark,preview_ascii};
    unsigned *sizes[]={&preview_tasks_n,&preview_controls_n,&preview_control_inline_n,&preview_control_dialog_n,
        &preview_red_n,&preview_lumen_n,&preview_onedark_n,&preview_ascii_n};
    if(argc!=9||render_entry())return 2;
    for(i=0;i<8;++i){
        FILE *f=fopen(argv[i+1],"wb");int bad;
        if(!f)return 3;
        bad=fwrite(frames[i],1,*sizes[i],f)!=*sizes[i];
        if(fclose(f)||bad)return 4;
    }
    return 0;
}
