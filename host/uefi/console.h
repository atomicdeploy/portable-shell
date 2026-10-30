/* VT subset -> UEFI Simple Text Output. Firmware calls stay in this adapter. */
#ifndef PS_UEFI_CONSOLE_H
#define PS_UEFI_CONSOLE_H
#include "abi.h"
typedef struct {
    efi_output *out;
    unsigned columns,rows,state,args[16],count,private_mode;
    unsigned utf,remaining,minimum,saved_x,saved_y,fg,bg,bold,reverse;
} efi_console;
static unsigned efi_bound(unsigned n,unsigned limit){return n<limit?n:limit-1;}
static void efi_position(efi_console *c,unsigned x,unsigned y){c->out->cursor(c->out,efi_bound(x,c->columns),efi_bound(y,c->rows));}
static void efi_colors(efi_console *c){
    unsigned fg=c->fg|(c->bold?8:0),bg=c->bg;
    if(c->reverse){unsigned tmp=fg;fg=bg;bg=tmp;}
    c->out->attribute(c->out,(fg&15)|((bg&7)<<4));
}
static unsigned efi_ansi_color(unsigned n){static const unsigned map[]={0,4,2,6,1,5,3,7};return map[n&7]|(n&8);}
static unsigned efi_rgb_color(unsigned r,unsigned g,unsigned b){
    return (r>=96?4:0)|(g>=96?2:0)|(b>=96?1:0)|((r>=192||g>=192||b>=192)?8:0);
}
static unsigned efi_index_color(unsigned n){
    static const unsigned cube[]={0,95,135,175,215,255};
    if(n<16)return efi_ansi_color(n);
    if(n>=256)return 7;
    if(n>=232){unsigned level=8+(n-232)*10;return efi_rgb_color(level,level,level);}
    n-=16;return efi_rgb_color(cube[n/36],cube[n/6%6],cube[n%6]);
}
static void efi_glyph(efi_console *c,unsigned n){
    efi_char text[2];
    if(n>0xffff||(n>=0xd800&&n<=0xdfff))n='?';
    if(n<32&&n!=8&&n!=9&&n!=10&&n!=13)return;
    text[0]=(efi_char)n;text[1]=0;
    if(n>=128&&(c->out->test(c->out,text)&EFI_ERROR_BIT)){
        text[0]=(n==0x2502)?'|':(n==0x2500)?'-':(n>=0x2500&&n<=0x257f)?'+':'?';
    }
    c->out->write(c->out,text);
}
static void efi_erase_line(efi_console *c,unsigned mode){
    unsigned x=(unsigned)c->out->mode->column,y=(unsigned)c->out->mode->row;
    unsigned start=mode==0?x:0,end=mode==1?x+1:c->columns,i;
    efi_position(c,start,y);
    /* Writing the bottom-right cell scrolls some firmware consoles. Leave it blank
       via ClearScreen on full clears; never wrap the application's viewport. */
    if(y+1==c->rows&&end==c->columns)--end;
    for(i=start;i<end;++i)efi_glyph(c,' ');
    efi_position(c,x,y);
}
static void efi_csi(efi_console *c,unsigned final){
    unsigned a=c->args[0],n=a?a:1,x=(unsigned)c->out->mode->column,y=(unsigned)c->out->mode->row,i;
    if(c->private_mode){
        if((final=='h'||final=='l')&&a==25)c->out->enable_cursor(c->out,final=='h');
        else if((final=='h'||final=='l')&&a==1049){c->out->clear(c->out);efi_position(c,0,0);}
        return; /* Mouse, paste and synchronized-output modes have no firmware equivalent. */
    }
    switch(final){
    case 'H':case 'f':efi_position(c,c->args[1]?c->args[1]-1:0,a?a-1:0);break;
    case 'A':efi_position(c,x,y>n?y-n:0);break;
    case 'B':efi_position(c,x,y+n);break;
    case 'C':efi_position(c,x+n,y);break;
    case 'D':efi_position(c,x>n?x-n:0,y);break;
    case 'G':efi_position(c,n-1,y);break;
    case 'K':efi_erase_line(c,a);break;
    case 'J':
        if(a==2||a==3){c->out->clear(c->out);efi_position(c,x,y);}
        else if(a==0){efi_erase_line(c,0);for(i=y+1;i<c->rows;++i){efi_position(c,0,i);efi_erase_line(c,2);}efi_position(c,x,y);}
        break;
    case 's':c->saved_x=x;c->saved_y=y;break;
    case 'u':efi_position(c,c->saved_x,c->saved_y);break;
    case 'm':
        for(i=0;i<=c->count;++i){unsigned v=c->args[i];
            if(!v){c->fg=7;c->bg=c->bold=c->reverse=0;}
            else if(v==1)c->bold=1;else if(v==22)c->bold=0;
            else if(v==7)c->reverse=1;else if(v==27)c->reverse=0;
            else if(v>=30&&v<=37)c->fg=efi_ansi_color(v-30);
            else if(v>=40&&v<=47)c->bg=efi_ansi_color(v-40);
            else if(v>=90&&v<=97)c->fg=efi_ansi_color(v-90)|8;
            else if(v>=100&&v<=107)c->bg=efi_ansi_color(v-100);
            else if(v==39)c->fg=7;else if(v==49)c->bg=0;
            else if((v==38||v==48)&&i<c->count){
                unsigned color=7,kind=c->args[++i];
                if(kind==5&&i<c->count)color=efi_index_color(c->args[++i]);
                else if(kind==2&&i+3<=c->count){color=efi_rgb_color(c->args[i+1],c->args[i+2],c->args[i+3]);i+=3;}
                if(v==38)c->fg=color;else c->bg=color&7;
            }
        }efi_colors(c);break;
    default:break;
    }
}
static void efi_console_write(void *context,const unsigned char *bytes,unsigned length){
    efi_console *c=context;unsigned i;
    for(i=0;i<length;++i){unsigned b=bytes[i];
        if(c->state==3){if(b==7)c->state=0;else if(b==27)c->state=4;continue;}
        if(c->state==4){c->state=b=='\\'?0:3;continue;}
        if(c->state==1){
            c->state=0;
            if(b=='['){unsigned k;c->state=2;c->count=c->private_mode=0;for(k=0;k<16;++k)c->args[k]=0;}
            else if(b==']')c->state=3;
            else if(b=='7'){c->saved_x=(unsigned)c->out->mode->column;c->saved_y=(unsigned)c->out->mode->row;}
            else if(b=='8')efi_position(c,c->saved_x,c->saved_y);
            continue;
        }
        if(c->state==2){
            if(b=='?')c->private_mode=1;
            else if(b>='0'&&b<='9'){if(c->args[c->count]<6553)c->args[c->count]=c->args[c->count]*10+b-'0';}
            else if(b==';'){if(c->count<15)++c->count;}
            else if(b>=0x40&&b<=0x7e){efi_csi(c,b);c->state=0;}
            continue;
        }
        if(c->remaining){
            if((b&0xc0)==0x80){c->utf=(c->utf<<6)|(b&63);if(!--c->remaining)efi_glyph(c,c->utf>=c->minimum?c->utf:'?');continue;}
            c->remaining=0;efi_glyph(c,'?');
        }
        if(b==27)c->state=1;
        else if(b<128)efi_glyph(c,b);
        else if(b>=0xc2&&b<0xe0){c->utf=b&31;c->remaining=1;c->minimum=0x80;}
        else if(b>=0xe0&&b<0xf0){c->utf=b&15;c->remaining=2;c->minimum=0x800;}
        else if(b>=0xf0&&b<=0xf4){c->utf=b&7;c->remaining=3;c->minimum=0x10000;}
        else efi_glyph(c,'?');
    }
}
#endif
