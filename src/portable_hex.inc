/* Binary editor: caller-owned draft storage; fixed-length overwrite semantics. */
#if PS_HEX_EDITOR
static void hex_close(ps_session *s){
    s->hex_open=0;s->hex_dialog=0;s->hex_data=0;
    ansi(s,"\033[0m\033[?25h\033[?1049l");prompt(s);
}
static unsigned hex_columns(ps_session *s){return s->io.columns>=78?16:s->io.columns>=46?8:1;}
static void hex_number(ps_session *s,unsigned n,unsigned digits){
    static const char alphabet[]="0123456789ABCDEF";char text[8];unsigned i;
    for(i=0;i<digits;++i){text[digits-i-1]=alphabet[n&15];n>>=4;}raw(s,text,digits);
}
static void hex_render(ps_session *s){
    unsigned cols=hex_columns(s),rows=s->io.rows>5?s->io.rows-5:0,r,c,index;
    if(!rows||s->io.columns<18){ansi(s,"\033[H\033[J");field(s,"Resize terminal",s->io.columns-1);return;}
    s->hex_top=s->hex_top/cols*cols;
    if(s->hex_pos<s->hex_top)s->hex_top=s->hex_pos/cols*cols;
    if(s->hex_pos>=s->hex_top+rows*cols)s->hex_top=(s->hex_pos/cols-rows+1)*cols;
    canvas_begin(s,1,0x484558);at(s,1,1);view_color(s,"\033[1;36m");field(s,s->hex_title,s->io.columns-1);reset(s);
    at(s,2,1);field(s,s->hex_dirty?"Modified":"Saved",s->io.columns-1);
    for(r=0;r<rows;++r){
        index=s->hex_top+r*cols;at(s,r+3,1);view_color(s,"\033[90m");
        if(index<s->hex_length)hex_number(s,index,8);else out(s,"        ");reset(s);out(s,"  ");
        for(c=0;c<cols;++c){
            if(index+c<s->hex_length){if(index+c==s->hex_pos&&!s->hex_ascii)view_selection(s);hex_number(s,s->hex_data[index+c],2);reset(s);}else out(s,"  ");out(s," ");
        }
        if(s->io.columns>=22){out(s,index<s->hex_length?" | ":"   ");for(c=0;c<cols;++c){
            unsigned b=index+c<s->hex_length?s->hex_data[index+c]:0;
            if(index+c==s->hex_pos&&s->hex_ascii)view_selection(s);
            if(index+c<s->hex_length){char ch=(char)(b>=32&&b<127?b:'.');raw(s,&ch,1);}else out(s," ");reset(s);
        }}
    }
    at(s,s->io.rows-2,1);field(s,s->hex_notice?s->hex_notice:(s->hex_ascii?"ASCII overwrite":"Hex overwrite: two digits per byte"),s->io.columns-1);
    at(s,s->io.rows-1,1);footer_field(s,"Tab Hex/ASCII  Ctrl-S Save  Esc Close",s->io.columns-1,0);
    at(s,s->io.rows,1);field(s,s->hex_dialog?"Unsaved: S Save / D Discard / Esc Cancel":"",s->io.columns-1);frame_end(s);
}
PS_API int ps_hex_editor_open(ps_session *s,const char *title,const ps_byte *data,unsigned length,
    ps_byte *draft,unsigned capacity,ps_hex_save_fn save,void *context){
    if(!s||!title||(!data&&length)||!draft||data==draft||capacity<length||s->hex_open||s->document_open||ps_form_active(s)||s->history_dialog||s->exit||!(s->io.flags&PS_INTERACTIVE)||s->io.columns<18||s->io.rows<6)return 0;
    if(s->board)table_close(s);
    ps_file_picker_close(s);ps_menu_close(s);
    copy(draft,data,length);s->hex_data=draft;s->hex_length=length;s->hex_title=title;s->hex_save=save;s->hex_context=context;
    s->hex_open=1;s->hex_pos=s->hex_top=s->hex_nibble=s->hex_ascii=s->hex_dirty=s->hex_dialog=0;s->hex_notice=0;
    s->canvas_kind=0;ansi(s,"\033[?1049h\033[?25l\033[?2004h");hex_render(s);return 1;
}
PS_API unsigned ps_hex_editor_active(const ps_session *s){return s?s->hex_open:0;}
static int hex_save(ps_session *s){
    if(!s->hex_save||!s->hex_save(s->hex_context,s->hex_data,s->hex_length)){s->hex_notice="Save failed; draft retained";return 0;}
    s->hex_dirty=0;s->hex_notice="Saved";return 1;
}
static void hex_key(ps_session *s,unsigned key){
    unsigned cols=hex_columns(s),step=cols*(s->io.rows>5?s->io.rows-5:1),value;
    if(s->hex_dialog){
        if(key==27||key==3)s->hex_dialog=0;
        else if(key=='d'||key=='D'){hex_close(s);return;}
        else if((key=='s'||key=='S')&&hex_save(s)){hex_close(s);return;}
    }else if(key==27||key==3){if(s->hex_dirty)s->hex_dialog=1;else {hex_close(s);return;}}
    else if(key==19)hex_save(s);
    else if(key==9){s->hex_ascii=!s->hex_ascii;s->hex_nibble=0;}
    else if(key==PS_KEY_HOME){s->hex_pos=0;s->hex_nibble=0;}
    else if(key==PS_KEY_END){s->hex_pos=s->hex_length?s->hex_length-1:0;s->hex_nibble=0;}
    else if(key==PS_KEY_LEFT||key==PS_KEY_UP||key==PS_KEY_PAGEUP){unsigned n=key==PS_KEY_LEFT?1:key==PS_KEY_UP?cols:step;s->hex_pos=s->hex_pos>n?s->hex_pos-n:0;s->hex_nibble=0;}
    else if(key==PS_KEY_RIGHT||key==PS_KEY_DOWN||key==PS_KEY_PAGEDOWN){unsigned n=key==PS_KEY_RIGHT?1:key==PS_KEY_DOWN?cols:step;if(s->hex_length)s->hex_pos+=min(n,s->hex_length-1-s->hex_pos);s->hex_nibble=0;}
    else if(s->hex_length){
        if(s->hex_ascii){if(key<32||key>126)return;value=key;}
        else {if(key>='0'&&key<='9')value=key-'0';else if(key>='a'&&key<='f')value=key-'a'+10;else if(key>='A'&&key<='F')value=key-'A'+10;else return;
            value=s->hex_nibble?(s->hex_data[s->hex_pos]&240)|value:(s->hex_data[s->hex_pos]&15)|(value<<4);}
        if(s->hex_data[s->hex_pos]!=value)s->hex_dirty=1;
        s->hex_data[s->hex_pos]=(ps_byte)value;s->hex_notice=0;
        if(s->hex_ascii||s->hex_nibble){if(s->hex_pos+1<s->hex_length)++s->hex_pos;s->hex_nibble=0;}else s->hex_nibble=1;
    }
    hex_render(s);
}
#else
static void hex_close(ps_session *s){(void)s;}
static void hex_render(ps_session *s){(void)s;}
static void hex_key(ps_session *s,unsigned key){(void)s;(void)key;}
PS_API unsigned ps_hex_editor_active(const ps_session *s){(void)s;return 0;}
PS_API int ps_hex_editor_open(ps_session *s,const char *t,const ps_byte *d,unsigned n,ps_byte *b,unsigned c,ps_hex_save_fn f,void *x){(void)s;(void)t;(void)d;(void)n;(void)b;(void)c;(void)f;(void)x;return 0;}
#endif
