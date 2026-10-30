/* Caller-owned file tree. Provider performs all I/O; indices are opaque to UI. */
PS_API unsigned ps_file_picker_active(const ps_session *s){return s?s->file_count:0;}
PS_API void ps_file_picker_close(ps_session *s){
    if(s&&s->file_count){s->file_count=0;if(PS_MOUSE)ansi(s,"\033[?1002l\033[?1006l");ansi(s,"\033[0m\033[?25h\033[?1049l");}
}
static void file_visit(ps_session *s,unsigned parent,unsigned depth,unsigned *ids,unsigned *depths,unsigned *count){
    unsigned i;for(i=0;i<s->file_count;++i)if(s->file_nodes[i].parent==parent){
        ids[*count]=i;depths[(*count)++]=depth;
        if(s->file_expanded[i])file_visit(s,i,depth+1,ids,depths,count);
    }
}
static unsigned file_visible(ps_session *s,unsigned *ids,unsigned *depths){unsigned count=0;file_visit(s,PS_FILE_ROOT,0,ids,depths,&count);return count;}
static unsigned file_last(ps_session *s,unsigned id){unsigned j;for(j=id+1;j<s->file_count;++j)if(s->file_nodes[j].parent==s->file_nodes[id].parent)return 0;return 1;}
static void file_render(ps_session *s){
    unsigned ids[128],depths[128],count=file_visible(s,ids,depths),i,slots=s->io.rows>=7?s->io.rows-6:1,w=s->io.columns-1;
    if(s->file_selected>=count)s->file_selected=count?count-1:0;
    if(s->file_selected<s->file_top)s->file_top=s->file_selected;
    if(s->file_selected>=s->file_top+slots)s->file_top=s->file_selected-slots+1;
    ansi(s,"\033[?25l\033[H\033[2J");view_color(s,"\033[1;33m");field(s,s->file_title,w);reset(s);
    if(s->io.rows<7){at(s,2,1);field(s,"Resize to browse",w);return;}
    view_rule(s,2,1,w,0);
    for(i=0;i<slots;++i){
        unsigned p=s->file_top+i;at(s,i+3,1);view_side(s);
        if(p<count){
            unsigned id=ids[p],indent=min(depths[p]*2,w-6),j;const ps_file_node *node=s->file_nodes+id;
            if(p==s->file_selected)view_color(s,"\033[30;43m");else view_color(s,(node->flags&PS_FILE_DISABLED)?"\033[90m":"\033[37m");
                        for(j=0;j<indent/2;++j){
                unsigned ancestor=id,k=depths[p]-j-1;while(k--)ancestor=s->file_nodes[ancestor].parent;
                if(j+1==depths[p])out(s,(s->io.flags&PS_UNICODE)?(file_last(s,ancestor)?"└─":"├─"):(file_last(s,ancestor)?"`-":"+-"));
                else out(s,file_last(s,ancestor)?"  ":(s->io.flags&PS_UNICODE)?"│ ":"| ");
            }
            if(indent&1)out(s," ");
            out(s,node->flags&PS_FILE_DIRECTORY?((s->io.flags&PS_UNICODE)?(s->file_expanded[id]?"▾ ":"▸ "):(s->file_expanded[id]?"- ":"+ ")):"  ");
            field(s,node->name,w-4-indent);reset(s);
        }else field(s,"",w-2);
        view_side(s);
    }
    view_rule(s,slots+3,1,w,1);
    at(s,s->io.rows-1,1);if(count){ps_number(s,s->file_selected+1);out(s," / ");ps_number(s,count);}
    at(s,s->io.rows,1);footer_field(s,(s->io.flags&PS_UNICODE)?"↑/↓ Select  ←/→ Fold  Enter Open  Esc Cancel":"Up/Down Select  Left/Right Fold  Enter Open  Esc Cancel",w,0);
}
PS_API int ps_file_picker_open(ps_session *s,const char *title,const ps_file_node *nodes,unsigned count,ps_file_pick_fn fn,void *ctx){
    unsigned i;if(!s||!title||!nodes||!count||count>128||s->document_open||s->exit||!(s->io.flags&PS_INTERACTIVE))return 0;
    for(i=0;i<count;++i){
        unsigned p=nodes[i].parent,depth=0;
        if(!nodes[i].name||!nodes[i].name[0])return 0;
        if(p!=PS_FILE_ROOT&&(p>=i||!(nodes[p].flags&PS_FILE_DIRECTORY)))return 0;
        while(p!=PS_FILE_ROOT){if(++depth>16)return 0;p=nodes[p].parent;}
    }
    if(s->board)table_close(s);
    ps_menu_close(s);ps_file_picker_close(s);
    s->file_nodes=nodes;s->file_count=count;s->file_title=title;s->file_pick=fn;s->file_context=ctx;
    s->file_selected=s->file_top=0;zero(s->file_expanded,sizeof s->file_expanded);
    ansi(s,"\033[?1049h");if(PS_MOUSE)ansi(s,"\033[?1002h\033[?1006h");file_render(s);return 1;
}
static void file_key(ps_session *s,unsigned key){
    unsigned ids[128],depths[128],count=file_visible(s,ids,depths),id,i;
    if(key==27||key==3){ps_file_pick_fn fn=s->file_pick;void *ctx=s->file_context;ps_file_picker_close(s);if(fn)fn(s,ctx,PS_FILE_ROOT);prompt(s);return;}
    if(!count)return;
    id=ids[s->file_selected];
    if(key==PS_KEY_UP&&s->file_selected)--s->file_selected;
    else if(key==PS_KEY_DOWN&&s->file_selected+1<count)++s->file_selected;
    else if(key==PS_KEY_HOME)s->file_selected=0;
    else if(key==PS_KEY_END)s->file_selected=count-1;
    else if(key==PS_KEY_PAGEUP)s->file_selected=s->file_selected>5?s->file_selected-5:0;
    else if(key==PS_KEY_PAGEDOWN)s->file_selected=min(count-1,s->file_selected+5);
    else if(key==PS_KEY_LEFT){
        if(s->file_expanded[id])s->file_expanded[id]=0;
        else for(i=0;i<count;++i)if(ids[i]==s->file_nodes[id].parent){s->file_selected=i;break;}
    }else if(key==PS_KEY_RIGHT||key==13||key==' '){
        if(s->file_nodes[id].flags&PS_FILE_DISABLED){ps_ring_bell(s);return;}
        if(s->file_nodes[id].flags&PS_FILE_DIRECTORY){s->file_expanded[id]=key==PS_KEY_RIGHT?1:!s->file_expanded[id];}
        else if(key==13){ps_file_pick_fn fn=s->file_pick;void *ctx=s->file_context;ps_file_picker_close(s);if(fn)fn(s,ctx,id);prompt(s);return;}
    }
    file_render(s);
}
static const ps_file_node session_files[]={ {"Session",PS_FILE_ROOT,PS_FILE_DIRECTORY},{"notes.txt",0,0} };
static void session_file_pick(ps_session *s,void *ctx,unsigned id){(void)ctx;if(id==1)ps_execute(s,"edit --framed");}
