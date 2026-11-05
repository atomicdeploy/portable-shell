/* Hosted firmware-protocol mock: real adapter and workbench, no firmware boot claim. */
#include <assert.h>
#include <string.h>
#include "../host/uefi/main.c"
enum { COLUMNS=80, ROWS=25 };
static efi_char screen[ROWS][COLUMNS];
static efi_mode mode={1,0,7,0,0,1};
static unsigned writes,closed,key_index,waits;
static const char input[]="help\rcontrols\r\033hex\r\033exit\r";
static efi_status EFIAPI mock_write(efi_output *out,efi_char *text){
    (void)out;
    while(*text){unsigned ch=*text++;++writes;assert(ch!=27);
        if(ch==13)mode.column=0;
        else if(ch==10)++mode.row;
        else if(ch==8){if(mode.column)--mode.column;}
        else {screen[mode.row][mode.column]=(efi_char)ch;if(++mode.column==COLUMNS){mode.column=0;++mode.row;}}
        if(mode.row==ROWS){memmove(screen,screen[1],sizeof screen-sizeof screen[0]);memset(screen[ROWS-1],0,sizeof screen[0]);--mode.row;}
    }return 0;
}
static efi_status EFIAPI mock_test(efi_output *out,efi_char *text){(void)out;return text[0]==0x276f?EFI_UNSUPPORTED:0;}
static efi_status EFIAPI mock_query(efi_output *out,uintptr_t index,uintptr_t *w,uintptr_t *h){(void)out;assert(!index);*w=COLUMNS;*h=ROWS;return 0;}
static efi_status EFIAPI mock_attribute(efi_output *out,uintptr_t attribute){(void)out;assert(attribute<128);mode.attribute=(int)attribute;return 0;}
static efi_status EFIAPI mock_clear(efi_output *out){(void)out;memset(screen,0,sizeof screen);mode.column=mode.row=0;return 0;}
static efi_status EFIAPI mock_cursor(efi_output *out,uintptr_t x,uintptr_t y){(void)out;assert(x<COLUMNS&&y<ROWS);mode.column=(int)x;mode.row=(int)y;return 0;}
static efi_status EFIAPI mock_enable(efi_output *out,uint8_t visible){(void)out;mode.visible=visible;return 0;}
static efi_output output={0,mock_write,mock_test,mock_query,0,mock_attribute,mock_clear,mock_cursor,mock_enable,&mode};
static efi_status EFIAPI mock_read(efi_input *in,efi_key *key){(void)in;assert(key_index<sizeof input-1);key->scan=0;key->unicode=(efi_char)input[key_index++];return 0;}
static efi_status EFIAPI mock_create(uint32_t type,uintptr_t tpl,void *fn,void *ctx,efi_event *event){assert(type==EFI_EVENT_TIMER&&!tpl&&!fn&&!ctx);*event=(void *)1;return 0;}
static efi_status EFIAPI mock_timer(efi_event event,unsigned type,uint64_t period){assert(event==(void *)1&&type==1&&period==100000);return 0;}
static efi_status EFIAPI mock_wait(uintptr_t count,efi_event *events,uintptr_t *index){assert(count==2&&events[0]==(void *)1);assert(++waits<100);*index=waits%2;return 0;}
static efi_status EFIAPI mock_close(efi_event event){assert(event==(void *)1);++closed;return 0;}
static void send(efi_console *c,const char *text){efi_console_write(c,(const unsigned char *)text,(unsigned)strlen(text));}
int main(void){
    efi_console c={0};efi_key key={1,0};efi_input in={0,mock_read,(void *)2};efi_boot boot={0};efi_system system={0};
    c.out=&output;c.columns=COLUMNS;c.rows=ROWS;c.fg=7;
    send(&c,"\033[3;5H");assert(mode.row==2&&mode.column==4);
    send(&c,"\033[31mX");assert(mode.attribute==4&&screen[2][4]=='X');
    send(&c,"\033[38;5;81;48;5;236m");assert(mode.attribute==11);
    send(&c,"\033[s\033[999C\033[u");assert(mode.column==5);
    send(&c,"\033]0;ignored title\007Y");assert(screen[2][5]=='Y');
    send(&c,"\033[2K");assert(screen[2][4]==' ');
    send(&c,"\033[?25l");assert(!mode.visible);
    send(&c,"\033[0m\033[1;1H\xe2");send(&c,"\x95\xad");assert(screen[0][0]==0x256d);
    send(&c,"\xe2\x9d\xaf");assert(screen[0][1]=='?');
    assert(firmware_key(key)==PS_KEY_UP);key.scan=17;assert(firmware_key(key)==PS_KEY_F7);
    mock_clear(&output);mode.attribute=7;mode.visible=1;
    boot.create_event=mock_create;boot.set_timer=mock_timer;boot.wait=mock_wait;boot.close_event=mock_close;
    system.header.signature=EFI_SYSTEM_SIGNATURE;system.input=&in;system.output=&output;system.boot=&boot;
    assert(!efi_main(0,&system));assert(closed==1&&key_index==sizeof input-1&&writes>100);
    assert(mode.visible&&mode.attribute==7);
    return 0;
}
