#include "env_manager.h"
#include "portable_util.h"
#if !PS_PANEL_KEYS
#error The host env manager requires PS_PANEL_KEYS (or disable PORTABLE_SHELL_HOST_ENV)
#endif
enum { ENV_IDLE,ENV_REFRESH,ENV_SEARCH,ENV_TOGGLE,ENV_UP,ENV_DOWN,ENV_SAVE,
       ENV_ADD,ENV_RECORD,ENV_UNDO,ENV_CLOSE,ENV_DELETE,ENV_PREVIOUS,ENV_NEXT,ENV_REVEAL,ENV_RELOAD,ENV_HELP };
static env_manager local_manager;
static void append(char *out,unsigned cap,const char *text){unsigned n=env_length(out);if(n<cap)env_string(out+n,cap-n,text);}
static void decimal(char *out,unsigned cap,unsigned value){char b[11];unsigned n=0;do{b[n++]=(char)('0'+value%10);value/=10;}while(value);if(n>=cap){if(cap)out[0]=0;return;}out[n]=0;while(n--)*out++=b[n];}
static void slice(char *out,unsigned cap,const char *text,unsigned n){unsigned i;if(!cap)return;if(n>=cap)n=cap-1;for(i=0;i<n;++i)out[i]=text[i]=='\n'||text[i]=='\r'?' ':text[i];out[n]=0;}
static void message(env_manager *m,const char *text){env_string(m->notice,sizeof m->notice,text?text:"Operation unavailable");}
static void lines(ps_session *s,const char *text){char part[256];unsigned n=0;
    while(*text){if(*text=='\r'){++text;continue;}if(*text=='\n'||n+1==sizeof part){part[n]=0;ps_write(s,part);n=0;if(*text=='\n'){ps_newline(s);++text;}continue;}part[n++]=*text++;}
    if(n){part[n]=0;ps_write(s,part);}
}
static int secret(const env_document *d,const env_record *r){
    static const char *const words[]={"PASSWORD","SECRET","TOKEN","PRIVATE","CREDENTIAL"};unsigned i,j,k;
    for(i=0;i<PS_COUNT_OF(words);++i)for(j=0;j<r->key_size;++j){
        for(k=0;words[i][k]&&j+k<r->key_size;++k){char c=d->text[r->key+j+k];if(c>='a'&&c<='z')c=(char)(c-'a'+'A');if(c!=words[i][k])break;}
        if(!words[i][k])return 1;
    }
    return 0;
}
static int read_control(void *context,unsigned id,ps_control_value *v){
    env_manager *m=context;env_document *d=&m->document;unsigned source=id>=PS_ENV_RECORDS,index=id%PS_ENV_RECORDS;const env_record *r;
    if(index>=d->count)return 0;
    r=&d->records[index];env_zero(v,sizeof *v);v->flags=PS_CONTROL_VALID;v->revision=d->revision;
    if(r->flags&(ENV_DISABLED|ENV_COMMENT|ENV_BLANK))v->flags|=PS_CONTROL_INACTIVE;
    if(r->flags&(ENV_DUPLICATE|ENV_UNKNOWN))v->flags|=PS_CONTROL_ATTENTION;
    if(!m->reveal&&secret(d,r))env_string(v->text,sizeof v->text,"[masked - Ctrl+E to edit record]");
    else if(source)slice(v->text,sizeof v->text,d->text+r->start,r->body_end-r->start);
    else slice(v->text,sizeof v->text,d->text+r->value,r->value_size);
    return 1;
}
static int write_control(void *context,unsigned id,const ps_control_value *v,ps_u32 revision){
    env_manager *m=context;env_document *d=&m->document;int ok;unsigned index=id%PS_ENV_RECORDS;
    if(revision!=d->revision||index>=d->count||id>=PS_ENV_RECORDS)return 0;
    ok=env_set_value(d,index,v->text);message(m,ok?"Value updated in draft":d->error);
    m->selected=index;m->pending=ENV_REFRESH;return ok;
}
static int shortcut(ps_session *s,unsigned key,void *context){
    env_manager *m=context;unsigned id=0,action=ENV_IDLE;
    if(!m->open)return 0;
    m->source=ps_panel_active(s)==2;
    if(ps_panel_selection(s,&id))m->selected=id%PS_ENV_RECORDS;
    switch(key){
        case PS_KEY_F1:action=ENV_HELP;break;
        case PS_KEY_F3:case '/':action=ENV_SEARCH;break;
        case 20:action=ENV_TOGGLE;break; /* Ctrl+T */
        case 21:action=ENV_UP;break;case 4:action=ENV_DOWN;break;
        case 19:action=ENV_SAVE;break;case 14:action=ENV_ADD;break;
        case 5:action=ENV_RECORD;break;case 26:action=ENV_UNDO;break;
        case 27:case 17:case 3:case 'q':action=ENV_CLOSE;break;
        case PS_KEY_DELETE:action=ENV_DELETE;break;
        case '[':action=ENV_PREVIOUS;break;case ']':action=ENV_NEXT;break;
        case 18:action=ENV_REVEAL;break;case 12:action=ENV_RELOAD;break;
        default:break;
    }
    if(action){
        if((action==ENV_DELETE||action==ENV_RECORD||action==ENV_TOGGLE||action==ENV_UP||action==ENV_DOWN)&&!ps_panel_selection(s,&id))return 1;
        m->pending=action;return 1;
    }return 0;
}
static void actions(env_manager *m){
    const env_record *r=m->selected<m->document.count?&m->document.records[m->selected]:0;
    int editable=r&&!m->source&&(r->flags&ENV_ASSIGNMENT)&&!(r->flags&ENV_MULTILINE)&&
        r->value_size<sizeof ((ps_control_value *)0)->text&&(m->reveal||!secret(&m->document,r));
    m->chrome.actions=editable?"Enter edit  F2 dialog  F3 find  ^S save  ^E record":
        "^E record  F3 find  ^S save  ^N add  Del remove";
}
static int show(env_manager *m){
    static const ps_column columns[]={{"Line",4,5,0},{"Key",14,32,2},{"Value / Source",14,0,3}};
    env_document *d=&m->document;unsigned i,p,n[2]={0,0},matched=0;char number[16];
    ps_session *s=m->session;
    if(!ps_panel_attach(s,0))return 0;
    for(i=0;i<d->count;++i){const env_record *r=&d->records[i];
        if(!env_find(d,i,m->filter))continue;
        ++matched;
        if(i<m->offset||i>=m->offset+PS_ENV_VIEW_ROWS)continue;
        for(p=0;p<PS_COUNT_OF(m->pages);++p){ps_control *c;char *label;unsigned row=n[p];
            if(n[p]==PS_ENV_VIEW_ROWS||(!p&&!(r->flags&ENV_ASSIGNMENT)))continue;
            c=&m->controls[p][n[p]];label=m->labels[p][n[p]++];env_zero(c,sizeof *c);
            decimal(m->line_numbers[p][row],sizeof m->line_numbers[p][row],r->line);label[0]=0;
            if(r->flags&ENV_DISABLED)append(label,PS_ENV_LABEL_BYTES,"# ");
            if(r->flags&ENV_DUPLICATE)append(label,PS_ENV_LABEL_BYTES,"! ");
            if(r->flags&ENV_ASSIGNMENT){unsigned len=env_length(label);slice(label+len,PS_ENV_LABEL_BYTES-len,d->text+r->key,r->key_size);
            }else append(label,PS_ENV_LABEL_BYTES,r->flags&ENV_COMMENT?"Comment":r->flags&ENV_BLANK?"Blank line":"Unparsed text");
            c->id=i+p*PS_ENV_RECORDS;c->label=label;c->group=m->line_numbers[p][row];
            c->kind=!p&&r->value_size<sizeof ((ps_control_value *)0)->text&&!(r->flags&ENV_MULTILINE)&&
                (m->reveal||!secret(d,r))?PS_CONTROL_TEXT:PS_CONTROL_LABEL;
        }
    }
    m->pages[0].title="Values";m->pages[1].title="Source order";
    for(p=0;p<PS_COUNT_OF(m->pages);++p){m->pages[p].controls=m->controls[p];m->pages[p].count=n[p];}
    env_string(m->status,sizeof m->status,env_dirty(d)?"Modified | ":"Saved | ");
    decimal(number,sizeof number,matched);append(m->status,sizeof m->status,number);append(m->status,sizeof m->status," records");
    if(m->filter[0]){append(m->status,sizeof m->status," | key: ");append(m->status,sizeof m->status,m->filter);}
    if(m->selected<d->count){const env_record *r=&d->records[m->selected];
        append(m->status,sizeof m->status," | line ");decimal(number,sizeof number,r->line);append(m->status,sizeof m->status,number);
        if(r->flags&ENV_DUPLICATE)append(m->status,sizeof m->status," | Duplicate key");
        if(r->flags&ENV_DISABLED)append(m->status,sizeof m->status," | Commented out");
    }
    m->panel.title=m->path;m->panel.status=m->status;m->panel.pages=m->pages;m->panel.count=PS_COUNT_OF(m->pages);
    m->panel.read=read_control;m->panel.write=write_control;m->panel.context=m;
    ps_set_table_style(s,PS_TABLE_DEFAULT|PS_TABLE_RED_BORDER|PS_TABLE_CATEGORY_COLUMN);
    if(!ps_panel_attach(s,&m->panel))return 0;
    m->chrome.columns=columns;m->chrome.count=PS_COUNT_OF(columns);m->chrome.summary=m->status;m->chrome.notice=m->notice;
    actions(m);
    m->chrome.navigation="Tab views  ^T toggle  ^U/^D move  F1 keys  Esc close";
    ps_panel_set_chrome(s,&m->chrome);ps_panel_set_key_handler(s,shortcut,m);
    if(!ps_panel_open(s,m->source))return 0;
    ps_panel_select(s,m->selected+m->source*PS_ENV_RECORDS);
    {unsigned id;if(ps_panel_selection(s,&id))m->selected=id%PS_ENV_RECORDS;}
    ps_panel_refresh(s);return 1;
}
static int disk_save(void *context,env_document *d,const char **error){return env_file_save((const char *)context,d,error);}
int env_manager_open(env_manager *m,ps_session *s,const char *path,const char *text,unsigned n){
    if(!m||!s||!path||env_length(path)>=sizeof m->path)return 0;
    env_zero(m,sizeof *m);m->session=s;
    if(!env_load(&m->document,text,n))return 0;
    env_string(m->path,sizeof m->path,path);m->save=disk_save;m->save_context=m->path;m->open=1;
    while(m->selected<m->document.count&&!(m->document.records[m->selected].flags&ENV_ASSIGNMENT))++m->selected;
    message(m,"Literal values, original file order. Ctrl+N adds an entry; Delete removes one.");
    if(!show(m)){m->open=0;return 0;}return 1;
}
static void close_manager(env_manager *m){ps_panel_set_key_handler(m->session,0,0);ps_panel_attach(m->session,0);m->open=0;m->returning=0;}
static int save(env_manager *m){const char *error=0;int ok;
    if(!env_dirty(&m->document)){message(m,"No changes to save");return 1;}
    ok=m->save&&m->save(m->save_context,&m->document,&error);message(m,ok?"Saved to disk":error);return ok;
}
static const char *validate(void *context,unsigned field,const char *value){
    env_manager *m=context;
    if(m->form_action==ENV_ADD&&field&&!env_value_valid(value))return "Use literal dotenv syntax; quote spaces, # and multiline values.";
    if(m->form_action==ENV_ADD&&!field){unsigned i;
        if(!env_key_valid(value))return "Use a letter or underscore, then letters, digits or underscores.";
        for(i=0;i<m->document.count;++i){const env_record *r=&m->document.records[i];
            if(r->key_size==env_length(value)&&env_find(&m->document,i,value))return "Key already exists (including commented entries).";
        }
    }
    return 0;
}
static void done(ps_session *s,void *context,int accepted){
    env_manager *m=context;env_document *d=&m->document;unsigned action=m->form_action;(void)s;
    m->form_action=ENV_IDLE;
    if(accepted){
        if(action==ENV_SEARCH){unsigned i;env_string(m->filter,sizeof m->filter,m->drafts[0]);m->offset=0;
            for(i=0;i<d->count;++i)if(env_find(d,i,m->filter)){m->selected=i;m->offset=i/PS_ENV_VIEW_ROWS*PS_ENV_VIEW_ROWS;break;}}
        else if(action==ENV_CLOSE){
            if(env_equal(m->drafts[0],"Save"))m->pending=ENV_SAVE;
            else if(env_equal(m->drafts[0],"Discard"))m->pending=ENV_CLOSE;
            /* Closing is deferred until after the form callback returns. */
            m->form_action=env_equal(m->drafts[0],"Keep editing")?ENV_IDLE:ENV_CLOSE;
        }else if(action==ENV_DELETE&&env_equal(m->drafts[0],"Delete")){
            const env_record *r=&d->records[m->selected];env_replace(d,r->start,r->end,"",0);message(m,"Record removed from draft; Ctrl+Z restores it");
        }else if(action==ENV_RELOAD&&env_equal(m->drafts[0],"Reload")){m->pending=ENV_RELOAD;m->form_action=ENV_RELOAD;}
        else if(action==ENV_ADD){
            char record[PS_ENV_DRAFT_BYTES];unsigned n=0;
            record[0]=0;if(d->size>d->bom&&d->text[d->size-1]!='\n')append(record,sizeof record,d->crlf?"\r\n":"\n");
            append(record,sizeof record,m->drafts[0]);append(record,sizeof record,"=");append(record,sizeof record,m->drafts[1]);
            append(record,sizeof record,d->crlf?"\r\n":"\n");n=env_length(record);
            if(env_replace(d,d->size,d->size,record,n)){m->selected=d->count-1;m->offset=m->selected/PS_ENV_VIEW_ROWS*PS_ENV_VIEW_ROWS;message(m,"Entry added at end; Ctrl+U moves it upward");}
            else message(m,d->error);
        }
    }else {m->form_action=ENV_IDLE;message(m,"Cancelled; draft retained");}
    m->returning=1;
}
static int form(env_manager *m,unsigned action){
    static const char *const closing[]={"Keep editing","Save","Discard"};
    static const char *const deleting[]={"Keep","Delete"};
    static const char *const reloading[]={"Keep draft","Reload"};
    unsigned i,count=action==ENV_ADD?2:1;
    env_zero(m->fields,sizeof m->fields);env_zero(m->drafts,sizeof m->drafts);
    for(i=0;i<count;++i){m->fields[i].kind=PS_FORM_TEXT;m->fields[i].value=m->drafts[i];m->fields[i].capacity=128;m->fields[i].maximum=127;}
    m->fields[0].label=action==ENV_SEARCH?"Find key (empty clears filter)":action==ENV_ADD?"New key":"Choose an action";
    m->fields[1].label="Value (dotenv syntax; quote # and multiline text)";
    if(action==ENV_SEARCH){m->fields[0].maximum=sizeof m->filter-1;env_string(m->drafts[0],sizeof m->drafts[0],m->filter);}
    if(action==ENV_ADD)m->fields[0].minimum=1;
    if(action==ENV_CLOSE||action==ENV_DELETE||action==ENV_RELOAD){
        m->fields[0].kind=PS_FORM_CHOICE;
        m->fields[0].choices=action==ENV_CLOSE?closing:action==ENV_DELETE?deleting:reloading;
        m->fields[0].choice_count=action==ENV_CLOSE?PS_COUNT_OF(closing):PS_COUNT_OF(deleting);
        env_string(m->drafts[0],sizeof m->drafts[0],m->fields[0].choices[0]);
    }
    m->form.fields=m->fields;m->form.count=count;m->form.validate=validate;m->form.done=done;m->form.context=m;
    m->form_action=action;
    ps_panel_attach(m->session,0);ps_form_set_style(m->session,PS_FORM_OVERVIEW);
    ps_form_set_title(m->session,action==ENV_CLOSE?"Unsaved .env changes":action==ENV_DELETE?"Delete selected record?":action==ENV_RELOAD?"Reload discards the current draft":action==ENV_SEARCH?"Find configuration key":"Add configuration entry");
    if(ps_form_open(m->session,&m->form))return 1;
    m->form_action=ENV_IDLE;
    message(m,"Forms are unavailable in this build; draft retained");show(m);return 0;
}
static int record_save(void *context,const char *text,unsigned n){
    env_manager *m=context;env_document *d=&m->document;env_record r=d->records[m->selected];
    int ok;unsigned i,k=0;int crlf=r.end>r.body_end+1;
    for(i=0;i<n;++i){if(k+2>=sizeof m->drafts[1])return 0;if(text[i]=='\n'&&crlf)m->drafts[1][k++]='\r';m->drafts[1][k++]=text[i];}
    ok=env_replace(d,r.start,r.body_end,m->drafts[1],k);message(m,ok?"Record updated in draft; Ctrl+S in table saves file":d->error);return ok;
}
void env_manager_dispatch(env_manager *m){
    unsigned action;env_document *d;
    if(!m||!m->open)return;
    d=&m->document;action=m->pending;m->pending=ENV_IDLE;
    if(!action){
        if(m->returning&&!ps_form_active(m->session)&&!ps_editor_active(m->session)){m->returning=0;show(m);}
        else if(ps_panel_active(m->session)){
            unsigned id;
            if(ps_panel_selection(m->session,&id)){
                unsigned index=id%PS_ENV_RECORDS;const env_record *r=&d->records[index];char num[16];
                if(index!=m->selected||m->source!=(ps_panel_active(m->session)==2)){
                    m->selected=index;m->source=ps_panel_active(m->session)==2;actions(m);
                    env_string(m->status,sizeof m->status,env_dirty(d)?"Modified | line ":"Saved | line ");decimal(num,sizeof num,r->line);append(m->status,sizeof m->status,num);
                    if(r->flags&ENV_DUPLICATE)append(m->status,sizeof m->status," | Duplicate key - each occurrence is independent");
                    if(r->flags&ENV_DISABLED)append(m->status,sizeof m->status," | Commented out");
                    if(r->flags&ENV_MULTILINE)append(m->status,sizeof m->status," | Multiline record");
                    if(r->comment)slice(m->notice,sizeof m->notice,d->text+r->comment,r->body_end-r->comment);
                    else message(m,r->flags&ENV_UNKNOWN?"Unparsed text is retained; Ctrl+E edits its exact source":"Ctrl+E edits the entire record. Ctrl+N adds; Delete removes; F1 shows all keys.");
                    ps_panel_refresh(m->session);
                }
            }
        }
        return;
    }
    if(action==ENV_CLOSE&&m->form_action==ENV_CLOSE){m->form_action=ENV_IDLE;close_manager(m);return;}
    if(action==ENV_SAVE){if(save(m)&&m->form_action==ENV_CLOSE){m->form_action=ENV_IDLE;close_manager(m);return;}m->form_action=ENV_IDLE;}
    else if(action==ENV_SEARCH||action==ENV_ADD||action==ENV_DELETE){form(m,action);return;}
    else if(action==ENV_CLOSE){if(env_dirty(d)){form(m,action);return;}close_manager(m);return;}
    else if(action==ENV_HELP){
        static const char help[]=".env document manager\n\n"
            "Enter       Inline value edit (short, visible values)\nF2          Modal value edit\n"
            "Ctrl+E      Edit the complete record, including key and comments\n"
            "Tab         Values / Source order\nF3 or /     Find keys; empty query clears the filter\n"
            "Ctrl+T      Comment / uncomment the selected assignment\nCtrl+U/D    Move one source record up / down\n"
            "Ctrl+N      Add key and value\nDelete      Remove a record with confirmation\n"
            "Ctrl+Z      One-step undo / redo\nCtrl+S      Save the complete draft to disk\n"
            "Ctrl+L      Reload from disk with confirmation\nCtrl+R      Reveal / mask secret-like keys\n"
            "[ / ]       Previous / next 128-record window\nEsc         Close; Save / Discard / Keep if modified\n\n"
            "Editing uses literal dotenv syntax, including quotes. Nothing is evaluated.\n"
            "Source order includes comments, blank lines and unsupported records.\n"
            "Moves affect one record, including its inline comment and multiline value.\n"
            "Standalone comments remain independent records. Duplicate keys are retained.\n"
            "Masked names are a convenience, not a guarantee of detecting every secret.\n"
            "Ctrl+E explicitly reveals the raw record. Changes stay in memory until saved.\n"
            "Save checks the original bytes and refuses external changes.\n";
        ps_panel_attach(m->session,0);ps_editor_set_style(m->session,PS_EDITOR_HEADER|PS_EDITOR_BORDER|PS_EDITOR_READ_ONLY);
        if(!ps_editor_open(m->session,".env keyboard reference",help,0,0)){
            message(m,"Text editor unavailable in this build");show(m);return;
        }
        m->returning=1;return;
    }else if(action==ENV_RECORD){
        const env_record *r;unsigned n,i,k=0;
        if(m->selected>=d->count)return;
        r=&d->records[m->selected];n=r->body_end-r->start;
        if(n>=sizeof m->drafts[0]){message(m,"Record exceeds editor capacity; untouched bytes are preserved");show(m);return;}
        for(i=0;i<n;++i){char ch=d->text[r->start+i];if(ch=='\r'&&i+1<n&&d->text[r->start+i+1]=='\n')continue;m->drafts[0][k++]=ch;}m->drafts[0][k]=0;
        ps_panel_attach(m->session,0);ps_editor_set_style(m->session,PS_EDITOR_HEADER|PS_EDITOR_BORDER);
        if(!ps_editor_open(m->session,"Edit record | literal dotenv syntax | saves to draft",m->drafts[0],record_save,m)){
            message(m,"Editor unavailable or record exceeds configured editor capacity");show(m);return;
        }
        m->returning=1;return;
    }else if(action==ENV_RELOAD){
        if(m->form_action!=ENV_RELOAD){form(m,action);return;}
        else {const char *error=0;unsigned n;m->form_action=ENV_IDLE;
            if(env_file_read(m->path,d->scratch,&n,&error)){
                /* Keep the current document if parsing the new file would fail. */
                if(env_replace(d,0,d->size,d->scratch,n)){env_mark_saved(d);m->offset=0;message(m,"Reloaded from disk");}
                else message(m,d->error);
            }else message(m,error);
        }
    }else if(action==ENV_UNDO){if(env_undo(d))message(m,"Undo / redo applied");else message(m,"No edit to undo");}
    else if(action==ENV_REVEAL){m->reveal=!m->reveal;message(m,m->reveal?"Values revealed; Ctrl+R masks secret-like keys":"Secret-like keys masked");}
    else if(action==ENV_PREVIOUS){m->offset=m->offset>PS_ENV_VIEW_ROWS?m->offset-PS_ENV_VIEW_ROWS:0;m->selected=m->offset;}
    else if(action==ENV_NEXT){if(m->offset+PS_ENV_VIEW_ROWS<d->count){m->offset+=PS_ENV_VIEW_ROWS;m->selected=m->offset;}}
    else if(m->selected<d->count){int ok=0;
        if(action==ENV_TOGGLE)ok=env_toggle(d,m->selected);
        if(action==ENV_UP||action==ENV_DOWN){int direction=action==ENV_UP?-1:1;ok=env_move(d,m->selected,direction);if(ok){m->selected=(unsigned)((int)m->selected+direction);m->offset=m->selected/PS_ENV_VIEW_ROWS*PS_ENV_VIEW_ROWS;}}
        if(action!=ENV_REFRESH)message(m,ok?"Draft updated; Ctrl+S saves, Ctrl+Z undoes":d->error?d->error:"No adjacent record / assignment available");
    }
    m->returning=0;show(m);
}
static int command(ps_session *s,int argc,const char *const *argv,void *context){
    env_manager *m=context;static char input[PS_ENV_BYTES];unsigned n;const char *error=0,*path=argc==1?".env":argv[argc-1];
    if(argc==2&&env_equal(argv[1],"--help")){
        lines(s,"env [FILE]        Edit a local UTF-8 dotenv file\nenv create FILE   Create an empty file without overwriting\nenv check FILE    Validate syntax; report counts without values\nF1 in the editor shows editing, ordering and save shortcuts.\n");return 0;
    }
    if(argc>3||(argc==3&&!env_equal(argv[1],"check")&&!env_equal(argv[1],"create"))){lines(s,"Usage: env [FILE] | env create FILE | env check FILE\n");return 2;}
    if(m->open){lines(s,"Close the current .env draft first\n");return 2;}
    if(argc==3&&env_equal(argv[1],"create")){
        if(!env_file_create(path,&error)){ps_write(s,error);ps_newline(s);return 1;}
        lines(s,"Created empty file; use env FILE to edit it\n");return 0;
    }
    if(argc==3){unsigned i,unknown=0,duplicates=0;char number[16];
        if(!env_file_read(path,input,&n,&error)||!env_load(&m->document,input,n)){ps_write(s,error?error:m->document.error);ps_newline(s);return 1;}
        for(i=0;i<m->document.count;++i){unknown+=!!(m->document.records[i].flags&ENV_UNKNOWN);duplicates+=!!(m->document.records[i].flags&ENV_DUPLICATE);}
        decimal(number,sizeof number,m->document.count);ps_write(s,number);ps_write(s," records; ");decimal(number,sizeof number,unknown);ps_write(s,number);ps_write(s," unparsed; ");
        decimal(number,sizeof number,duplicates);ps_write(s,number);lines(s," duplicate-key occurrences\n");return unknown?1:0;
    }
    if(!env_file_read(path,input,&n,&error)||!env_manager_open(m,s,path,input,n)){
        ps_write(s,error?error:m->document.error?m->document.error:"Interactive table support is required");ps_newline(s);return 1;
    }
    return 0;
}
int env_manager_register(ps_session *s){
    ps_command c={"env","[FILE] | create FILE | check FILE","Files",command,0,&local_manager};return ps_register(s,&c);
}
void env_manager_poll(ps_session *s){if(local_manager.session==s)env_manager_dispatch(&local_manager);}
void env_manager_feed(ps_session *s,const void *bytes,unsigned count){
    const ps_byte *p=bytes;
    /* Dispatch between decoded events, including shortcut + typing in one native
       read. A form must exist before the following bytes are sent to it. */
    while(count){
        unsigned n=1;
        /* Keep ordinary shell input batched; a submitted command can open the
           manager before the next bytes in the same native read arrive. */
        if(!local_manager.open||local_manager.session!=s){
            while(n<count&&p[n-1]!='\r'&&p[n-1]!='\n')++n;
        }
        ps_feed(s,p,n);env_manager_poll(s);p+=n;count-=n;
    }
}
