/* Numbered menus: caller-owned trees, bounded navigation, no table/OS calls. */
static int menu_valid(const ps_menu *m){
    unsigned i;
    if(!m||!m->title||m->count>999||(m->count&&!m->items))return 0;
    for(i=0;i<m->count;++i)
        if(!m->items[i].label||(m->items[i].submenu&&m->items[i].run))return 0;
    return 1;
}
PS_API unsigned ps_menu_active(const ps_session *s){return s?s->menu_depth:0;}
PS_API void ps_menu_refresh(ps_session *s){
    unsigned i,w;const ps_menu *m;
    if(!s||!s->menu_depth)return;
    m=s->menu_stack[s->menu_depth-1];w=s->io.columns>24?s->io.columns-24:8;
    s->menu_length=s->menu_bad=0;
    ps_newline(s);ink(s,PS_INK_HEADING);field(s,m->title,min(cells(m->title,length(m->title),s->io.flags),min(w,120)));reset(s);ps_newline(s);
    for(i=0;i<m->count;++i){
        const ps_menu_item *item=m->items+i;
        out(s,"  ");ink(s,PS_INK_NUMBER);ps_number(s,i+1);out(s,". ");
        ink(s,(item->flags&PS_MENU_DISABLED)||(!item->run&&!item->submenu)?PS_INK_INACTIVE:PS_INK_TEXT);
        field(s,item->label,min(cells(item->label,length(item->label),s->io.flags),min(w,120)));
        if(item->submenu)out(s," >");
        if((item->flags&PS_MENU_DISABLED)||(!item->run&&!item->submenu))out(s," (unavailable)");
        reset(s);ps_newline(s);
    }
    out(s,s->menu_depth>1?"  0. Back":"  0. Close");ps_newline(s);
    if(s->io.flags&PS_INTERACTIVE){ink(s,PS_INK_HEADING);out(s,"> ");reset(s);}
}
PS_API void ps_menu_close(ps_session *s){
    if(!s)return;
    s->menu_depth=s->menu_length=s->menu_bad=0;
}
PS_API int ps_menu_open(ps_session *s,const ps_menu *menu){
    if(!s||s->exit||s->document_open||!menu_valid(menu))return 0;
    if(s->board)table_close(s);
    s->search=0;s->menu_stack[0]=menu;s->menu_depth=1;
    ps_menu_refresh(s);return 1;
}
PS_API int ps_menu_choose(ps_session *s,unsigned number){
    const ps_menu *m;const ps_menu_item *item;
    if(!s||!s->menu_depth)return 2;
    m=s->menu_stack[s->menu_depth-1];
    if(!number){if(--s->menu_depth)ps_menu_refresh(s);else ps_menu_close(s);return 0;}
    if(number>m->count)return 2;
    item=m->items+number-1;
    if((item->flags&PS_MENU_DISABLED)||(!item->submenu&&!item->run))return 2;
    if(item->submenu){
        if(s->menu_depth==PS_MENU_DEPTH||!menu_valid(item->submenu))return 2;
        s->menu_stack[s->menu_depth++]=item->submenu;ps_menu_refresh(s);return 0;
    }
    ps_menu_close(s);s->status=item->run(s,item->context);return s->status;
}
static void menu_key(ps_session *s,unsigned key){
    if(key==3){ps_newline(s);ps_menu_close(s);prompt(s);return;}
    if(key==27){ps_newline(s);ps_menu_choose(s,0);if(!s->menu_depth)prompt(s);return;}
    if(key==8||key==127){
        if(s->menu_bad)s->menu_bad=0;
        else if(s->menu_length){--s->menu_length;if(s->io.flags&PS_INTERACTIVE)out(s,"\b \b");}
        return;
    }
    if(key==21){while(s->menu_length)menu_key(s,8);s->menu_bad=0;return;}
    if(key==13){
        unsigned i,n=0,bad=s->menu_bad;
        ps_newline(s);
        if(!s->menu_length&&!bad){ps_menu_refresh(s);return;}
        for(i=0;i<s->menu_length;++i)n=n*10+(unsigned)(s->menu_input[i]-'0');
        s->menu_length=s->menu_bad=0;
        if(bad||ps_menu_choose(s,n)==2){
            if(s->menu_depth){ink(s,PS_INK_WARNING);out(s,"Unavailable selection");reset(s);ps_menu_refresh(s);}
        }
        if(!s->menu_depth)prompt(s);
        return;
    }
    if(key>='0'&&key<='9'){
        if(s->menu_length==3){s->menu_bad=1;return;}
        s->menu_input[s->menu_length++]=(char)key;
        if(s->io.flags&PS_INTERACTIVE){char c=(char)key;raw(s,&c,1);}
    }else if(key>=32&&key<256)s->menu_bad=1;
}
static int menu_command(ps_session *s,void *context){return ps_execute(s,(const char*)context);}
static const ps_menu_item menu_workspaces[]={
    {"Tasks",0,menu_command,(void*)"board tasks",0},
    {"Controls",0,menu_command,(void*)"controls",0},
    {"Variables",0,menu_command,(void*)"board variables",0}};
static const ps_menu workspace_menu={"Workspaces",menu_workspaces,3};
static const ps_menu_item menu_colors[]={
    {"Semantic colors",0,menu_command,(void*)"theme semantic",0},
    {"Lumen",0,menu_command,(void*)"theme lumen",0},
    {"Monochrome",0,menu_command,(void*)"theme mono",0}};
static const ps_menu color_menu={"Color policy",menu_colors,3};
static const ps_menu_item menu_tools[]={
    {"Terminal status",0,menu_command,(void*)"status",0},
    {"Command help",0,menu_command,(void*)"help",0},
    {"Color policy",&color_menu,0,0,0}};
static const ps_menu tools_menu={"Tools",menu_tools,3};
static const ps_menu_item menu_root_items[]={
    {"Workspaces",&workspace_menu,0,0,0},
    {"Tools",&tools_menu,0,0,0}};
static const ps_menu default_menu={"PortableShell",menu_root_items,2};
