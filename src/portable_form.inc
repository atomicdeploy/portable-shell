#if PS_FORM
static void form_load(ps_session *s){
    const ps_form_field *f=s->form->fields+s->form_index;unsigned i;
    str(s->edit,f->value,PS_VALUE);s->editpos=s->editlen=length(s->edit);
    s->form_error=0;s->form_top=s->form_choice=0;
    if(f->kind==PS_FORM_CHOICE)for(i=0;i<f->choice_count;++i)
        if(equal(f->value,f->choices[i])){s->form_choice=i;break;}
}
static void form_border(ps_session *s,unsigned y,unsigned w,int bottom,unsigned role){
    view_rule_color(s,y,2,w,bottom,ps_style_ink(s->theme,(s->io.flags&PS_TRUECOLOR)!=0,0,role));
}
static void form_value(ps_session *s,unsigned y,unsigned w,const char *value,unsigned role,unsigned border){
    at(s,y,2);ink(s,border);out(s,(s->io.flags&PS_UNICODE)?"│":"|");
    out(s," ");ink(s,role);field(s,value,w-4);out(s," ");ink(s,border);
    out(s,(s->io.flags&PS_UNICODE)?"│":"|");reset(s);
}
static void form_title(ps_session *s,unsigned y,unsigned w,const char *label,unsigned role){
    form_border(s,y,w,0,role);at(s,y,4);ink(s,role);out(s," ");
    field(s,label,min(w-8,cells(label,length(label),s->io.flags)));out(s," ");reset(s);
}
static void form_render(ps_session *s){
    const ps_form_field *f=s->form->fields+s->form_index;
    unsigned w=s->io.columns-3,y=2,i,slots,start=0,role=s->form_error?PS_INK_WARNING:PS_INK_HEADING;
    if(s->io.rows<8||s->io.columns<20)return;
    canvas_begin(s,9,0);
    for(i=1;i<=s->io.rows;++i){at(s,i,1);ansi(s,"\033[2K");}
    if(s->form_index&&s->io.rows>=15){
        const ps_form_field *previous=s->form->fields+s->form_index-1;
        form_title(s,y,w,previous->label,PS_INK_MUTED);
        form_value(s,y+1,w,previous->value,PS_INK_TEXT,PS_INK_BORDER);
        form_border(s,y+2,w,1,PS_INK_BORDER);y+=5;
    }
    form_title(s,y,w,f->label,role);
    if(f->kind==PS_FORM_TEXT){
        while(start<s->editpos&&cells(s->edit+start,s->editpos-start,s->io.flags)>=w-4)start=next(s->edit,start);
        form_value(s,y+1,w,s->edit+start,PS_INK_TEXT,role);slots=1;
    }else{
        slots=min(f->choice_count,s->io.rows-y-5);
        if(s->form_choice<s->form_top)s->form_top=s->form_choice;
        if(s->form_choice>=s->form_top+slots)s->form_top=s->form_choice-slots+1;
        for(i=0;i<slots;++i){
            unsigned selected=s->form_top+i==s->form_choice;
            at(s,y+1+i,2);ink(s,role);out(s,(s->io.flags&PS_UNICODE)?"│":"|");out(s," ");
            ink(s,selected?PS_INK_ACTION:PS_INK_MUTED);
            out(s,(s->io.flags&PS_UNICODE)?(selected?"● ":"○ "):(selected?"(*) ":"( ) "));
            field(s,f->choices[s->form_top+i],w-((s->io.flags&PS_UNICODE)?6:8));
            out(s," ");ink(s,role);out(s,(s->io.flags&PS_UNICODE)?"│":"|");reset(s);
        }
    }
    form_border(s,y+slots+1,w,1,role);
    at(s,y+slots+2,3);ink(s,s->form_error?PS_INK_WARNING:PS_INK_MUTED);
    if(s->form_error)field(s,s->form_error,w-2);
    else if(f->kind==PS_FORM_CHOICE){ps_number(s,s->form_choice+1);out(s," / ");ps_number(s,f->choice_count);}
    reset(s);at(s,s->io.rows,2);
    footer_field(s,f->kind==PS_FORM_CHOICE?((s->io.flags&PS_UNICODE)?"↑/↓ Choose  Enter Continue  Esc Cancel":"Up/Down Choose  Enter Continue  Esc Cancel"):
        ((s->io.flags&PS_UNICODE)?"←/→ Edit  Enter Continue  Esc Cancel":"Left/Right Edit  Enter Continue  Esc Cancel"),s->io.columns-2,0);
    if(f->kind==PS_FORM_TEXT){at(s,y+1,4+cells(s->edit+start,s->editpos-start,s->io.flags));ansi(s,"\033[?25h");}
    else ansi(s,"\033[?25l");
    frame_end(s);
}
#if PS_MOUSE
static void form_mouse(ps_session *s,unsigned x,unsigned y){
    const ps_form_field *f=s->form->fields+s->form_index;
    unsigned top=(s->form_index&&s->io.rows>=15)?8:3,w=s->io.columns-3;
    if(x<4||x>=s->io.columns-2||s->io.rows<8||s->io.columns<20)return;
    if(f->kind==PS_FORM_CHOICE){
        unsigned slots=min(f->choice_count,s->io.rows-(top-1)-5);
        if(y>=top&&y<top+slots){s->form_choice=s->form_top+y-top;s->form_error=0;form_render(s);}
    }else if(y==top){
        unsigned start=0,pos,cell=0;
        while(start<s->editpos&&cells(s->edit+start,s->editpos-start,s->io.flags)>=w-4)start=next(s->edit,start);
        pos=start;
        while(pos<s->editlen){unsigned n,c=rune(s->edit+pos,&n),cw=(s->io.flags&PS_UNICODE)?width(c):1;
            if(cell+cw>x-4)break;
            cell+=cw;pos+=n;
        }
        s->editpos=pos;form_render(s);
    }
}
#endif
static void form_finish(ps_session *s,int accepted){
    const ps_form *f=s->form;if(!f)return;
    s->form=0;s->form_error=0;s->canvas_kind=0;
    ps_secure_clear(s->edit,sizeof s->edit);s->editpos=s->editlen=0;
    if(PS_MOUSE)ansi(s,"\033[?1002l\033[?1006l");
    ansi(s,"\033[0m\033[?25h\033[?1049l");
    if(f->done)f->done(s,f->context,accepted);
}
static void form_key(ps_session *s,unsigned key){
    const ps_form_field *f=s->form->fields+s->form_index;
    if(key==27||key==3){form_finish(s,0);prompt(s);return;}
    if(key==13){
        const char *value=f->kind==PS_FORM_CHOICE?f->choices[s->form_choice]:s->edit;
        unsigned count=0,i=0,n;
        while(value[i]){rune(value+i,&n);i+=n;++count;}
        s->form_error=0;
        if(count<f->minimum)s->form_error="An answer is required or is too short.";
        else if(f->maximum&&count>f->maximum)s->form_error="The answer exceeds the allowed length.";
        else if(length(value)>=f->capacity)s->form_error="The answer exceeds its storage capacity.";
        else if(s->form->validate)s->form_error=s->form->validate(s->form->context,s->form_index,value);
        if(!s->form_error){
            str(f->value,value,f->capacity);
            if(++s->form_index==s->form->count){form_finish(s,1);prompt(s);return;}
            form_load(s);
        }
        form_render(s);return;
    }
    s->form_error=0;
    if(f->kind==PS_FORM_CHOICE){
        if(key==PS_KEY_UP&&s->form_choice)--s->form_choice;
        if(key==PS_KEY_DOWN&&s->form_choice+1<f->choice_count)++s->form_choice;
        if(key==PS_KEY_HOME)s->form_choice=0;
        if(key==PS_KEY_END)s->form_choice=f->choice_count-1;
    }else{
        if(key==PS_KEY_LEFT)s->editpos=prev(s->edit,s->editpos);
        else if(key==PS_KEY_RIGHT)s->editpos=next(s->edit,s->editpos);
        else if(key==PS_KEY_HOME||key==1)s->editpos=0;
        else if(key==PS_KEY_END||key==5)s->editpos=s->editlen;
        else if(key==21){s->edit[0]=0;s->editpos=s->editlen=0;}
        else if(key==8||key==127)edit_delete(s->edit,&s->editlen,&s->editpos,1);
        else if(key==PS_KEY_DELETE)edit_delete(s->edit,&s->editlen,&s->editpos,0);
        else if(key>=32&&key<256){
            if(s->editlen+1>=PS_VALUE){s->form_error="Input storage is full.";ps_ring_bell(s);}
            else edit_insert(s->edit,&s->editlen,&s->editpos,PS_VALUE,key);
        }
    }
    form_render(s);
}
#endif
PS_API unsigned ps_form_active(const ps_session *s){
#if PS_FORM
    return s&&s->form?s->form_index+1:0;
#else
    (void)s;return 0;
#endif
}
PS_API void ps_form_close(ps_session *s){
#if PS_FORM
    if(s)form_finish(s,0);
#else
    (void)s;
#endif
}
PS_API int ps_form_open(ps_session *s,const ps_form *form){
#if PS_FORM
    unsigned i,j;
    if(!s||s->exit||s->form||s->board||s->menu_depth||s->document_open||s->file_count||
       !(s->io.flags&PS_INTERACTIVE)||s->io.columns<20||s->io.rows<8||
       !form||!form->fields||!form->count||form->count>64)return 0;
    for(i=0;i<form->count;++i){
        const ps_form_field *f=form->fields+i;
        if(!f->label||!f->value||!f->capacity||f->kind>PS_FORM_CHOICE||
           (f->maximum&&f->minimum>f->maximum))return 0;
        for(j=0;j<f->capacity&&f->value[j];++j){}
        if(j==f->capacity||(f->kind==PS_FORM_TEXT&&j>=PS_VALUE))return 0;
        if(f->kind==PS_FORM_CHOICE){
            if(!f->choices||!f->choice_count||f->choice_count>256)return 0;
            for(j=0;j<f->choice_count;++j)if(!f->choices[j]||length(f->choices[j])>=f->capacity)return 0;
        }
    }
    s->form=form;s->form_index=0;s->search=0;form_load(s);
    ansi(s,"\033[?1049h");if(PS_MOUSE)ansi(s,"\033[?1002h\033[?1006h");s->canvas_kind=0;form_render(s);return 1;
#else
    (void)s;(void)form;return 0;
#endif
}
