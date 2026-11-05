#ifndef PORTABLE_SHELL_TERMINAL_RECORDING_H
#define PORTABLE_SHELL_TERMINAL_RECORDING_H
/* Optional host-side recorder. Supply a monotonic timestamp in milliseconds.
   Call F after a complete repaint and E later to hold the final frame. */
#include <stdio.h>
static int ps_record_begin(FILE *out,unsigned columns,unsigned rows) {
    return fprintf(out,"PSREPLAY 1 %u %u\n",columns,rows)>0;
}
static int ps_record_event(FILE *out,unsigned milliseconds,char type,const void *bytes,unsigned count) {
    return fprintf(out,"%u %c %u\n",milliseconds,type,count)>0 &&
        (!count||fwrite(bytes,1,count,out)==count) && fputc('\n',out)!=EOF;
}
#endif
