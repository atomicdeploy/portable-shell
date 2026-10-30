/* Optional firmware host: no operating system, libc, sockets or disk writes. */
#include "portable_shell.h"
#include "console.h"
static union {void *alignment;ps_byte bytes[PS_DEFAULT_SESSION_STORAGE];} storage;
static efi_console console;
static unsigned firmware_key(efi_key key){
    switch(key.scan){
    case 1:return PS_KEY_UP;case 2:return PS_KEY_DOWN;
    case 3:return PS_KEY_RIGHT;case 4:return PS_KEY_LEFT;
    case 5:return PS_KEY_HOME;case 6:return PS_KEY_END;
    case 8:return PS_KEY_DELETE;case 9:return PS_KEY_PAGEUP;case 10:return PS_KEY_PAGEDOWN;
    case 11:return PS_KEY_F1;case 12:return PS_KEY_F2;case 13:return PS_KEY_F3;
    case 14:return PS_KEY_F4;case 17:return PS_KEY_F7;case 23:return 27;
    default:return key.scan?0:key.unicode;
    }
}
efi_status EFIAPI efi_main(void *image,efi_system *system){
    ps_config config;ps_session *session;efi_status status;efi_event events[2];
    uintptr_t columns,rows,index;unsigned now=0,old_attribute;uint8_t old_visible;
    (void)image;
    if(!system||system->header.signature!=EFI_SYSTEM_SIGNATURE||!system->input||!system->output||!system->boot)return EFI_UNSUPPORTED;
    console.out=system->output;
    if(console.out->query(console.out,(uintptr_t)console.out->mode->mode,&columns,&rows)||columns<24||rows<10)return EFI_UNSUPPORTED;
    console.columns=(unsigned)columns;console.rows=(unsigned)rows;console.fg=7;
    old_attribute=(unsigned)console.out->mode->attribute;old_visible=console.out->mode->visible;
    config.write=efi_console_write;config.context=&console;config.columns=console.columns;config.rows=console.rows;
    config.flags=PS_INTERACTIVE|PS_COLOR|PS_UNICODE|PS_NO_SYNC_OUTPUT;
    session=ps_init(storage.bytes,sizeof storage.bytes,&config);if(!session)return EFI_UNSUPPORTED;
    /* A periodic firmware event keeps cooperative jobs running while input is idle.
       Scheduling timestamps are nominal 10 ms ticks, not a precision wall clock. */
    status=system->boot->create_event(EFI_EVENT_TIMER,0,0,0,&events[0]);if(status)return status;
    status=system->boot->set_timer(events[0],1,100000); /* TimerPeriodic, 100 ns units. */
    if(status){system->boot->close_event(events[0]);return status;}
    events[1]=system->input->wait;
    ps_set_application_title(session,"PortableShell / UEFI");ps_start(session);
    while(!ps_exited(session)){
        status=system->boot->wait(2,events,&index);if(status)break;
        if(!index){now+=10;ps_tick(session,now);}
        else {
            efi_key key;status=system->input->read(system->input,&key);
            if(!status){unsigned value=firmware_key(key);
                if(key.scan){if(value)ps_key(session,value);}
                else if(value<128)ps_key(session,value);
                else if(value<0xd800||value>0xdfff){ps_byte bytes[3];unsigned n;
                    if(value<0x800){bytes[0]=(ps_byte)(0xc0|(value>>6));bytes[1]=(ps_byte)(0x80|(value&63));n=2;}
                    else {bytes[0]=(ps_byte)(0xe0|(value>>12));bytes[1]=(ps_byte)(0x80|((value>>6)&63));bytes[2]=(ps_byte)(0x80|(value&63));n=3;}
                    ps_feed(session,bytes,n);
                }
            }else if(status!=EFI_NOT_READY)break;
        }
        if(!console.out->query(console.out,(uintptr_t)console.out->mode->mode,&columns,&rows)&&columns>=24&&rows>=10&&
            (columns!=console.columns||rows!=console.rows)){
            console.columns=(unsigned)columns;console.rows=(unsigned)rows;ps_resize(session,console.columns,console.rows);
        }
    }
    ps_close(session);system->boot->close_event(events[0]);
    console.out->attribute(console.out,old_attribute);console.out->enable_cursor(console.out,old_visible);
    return status==EFI_NOT_READY?0:status;
}
