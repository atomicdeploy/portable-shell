/* Shared, fixed-storage help presentation. No host API or allocator. */
static unsigned help_width(ps_session *s) {
    return s->io.columns >= 20 ? min(s->io.columns,240)-1 : 79;
}
static unsigned help_cells(ps_session *s,const char *text) { return text?cells(text,length(text),s->io.flags):0; }
/* Preserve UTF-8 sequences, explicit newlines and hanging indentation.
   Metadata is trusted printable text, not embedded terminal control codes. */
static void help_text(ps_session *s,const char *text,unsigned column,unsigned indent) {
    unsigned available=help_width(s);
    if(!text)text="";
    while(*text){
        const char *end;unsigned cells=0;
        if(*text=='\n'){ps_newline(s);spaces(s,indent);column=indent;++text;continue;}
        if(*text==' '||*text=='\t'){
            ++text;
            if(column>indent&&column<available){out(s," ");++column;}
            continue;
        }
        end=text;while(*end&&*end!=' '&&*end!='\t'&&*end!='\n'){unsigned n,c=rune(end,&n);cells+=(s->io.flags&PS_UNICODE)?width(c):1;end+=n;}
        if(column>indent&&column+cells>available){ps_newline(s);spaces(s,indent);column=indent;}
        while(text<end){
            unsigned n,c=rune(text,&n),w=(s->io.flags&PS_UNICODE)?width(c):1; const char *next=text+n;
            if(column+w>available){ps_newline(s);spaces(s,indent);column=indent;}
            raw(s,text,(unsigned)(next-text));column+=w;text=next;
        }
    }
    ps_newline(s);
}
static void help_heading(ps_session *s,const char *label){
    ink(s,PS_INK_HEADING);ansi(s,"\033[1m");ps_write(s,label);out(s,":");reset(s);ps_newline(s);
}
static void help_row(ps_session *s,const char *label,const char *description,unsigned width,unsigned role){
    unsigned n=help_cells(s,label),indent=width+4;
    out(s,"  ");ink(s,role);
    if(n>width){help_text(s,label,2,2);reset(s);spaces(s,indent);}
    else {ps_write(s,label);reset(s);spaces(s,width-n+2);}
    help_text(s,description,indent,indent);
}
#if PS_HELP_DETAILS
static const char *const help_summaries[]={
    "Inspect and adjust connected controls.","Explore commands and their usage.",
    "Open the workspace dashboard.","Edit session notes with save and discard.",
    "Navigate a numbered menu.","Browse session files.",
    "Create, complete and organize tasks.","Store a session value.",
    "Read session values.","Remove a session value.",
    "Transform text.","Calculate with integers.","Hash text in a background job.",
    "Inspect or cancel background work.","Ring the terminal bell.",
    "Print text.","Inspect parsed arguments.","Review or clear command history.",
    "Bind a function key to a command.","Configure terminal colors and capabilities.",
    "Customize prompt text, colors and lines.","Render a seven-segment display.",
    "Clear the terminal.","Show session status.","Close this session."
};
static const ps_help_item segment_arguments[]={
    {"TEXT","Digits, a minus sign, decimal points and colons. Quote text containing spaces."}
};
static const ps_help_item segment_options[]={
    {"--large","Use larger segments for a display that is easier to read at a distance."},
    {"--red","Use red for active segments."},
    {"--mono","Disable segment colors."},
    {"--ascii","Draw with ASCII characters for terminals without Unicode support."}
};
static const ps_help_item segment_examples[]={
    {"segments 12:48.6","Display a time with fractional seconds."},
    {"segments -23.5 --large --red","Show a large signed reading."},
    {"segments 0123456789 --ascii","Preview all digits on a basic terminal."}
};
static const ps_help_document segment_help={
    "segments","Render a seven-segment display from text. Combine size, color and character options to suit your terminal.",
    segment_arguments,1,segment_options,4,segment_examples,3
};
static const ps_help_document *help_document(ps_session *s,const char *name){
    unsigned i;for(i=0;i<s->help_document_count;++i)
        if(equal(name,s->help_documents[i].name))return s->help_documents+i;
    return equal(name,"segments")?&segment_help:0;
}
static const char *help_description(ps_session *s,const ps_command *c){
    const ps_help_document *d=help_document(s,c->name);unsigned i;
    if(d&&d->description)return d->description;
    if(c->run==builtin)for(i=0;i<sizeof(builtins)/sizeof(builtins[0]);++i)
        if(equal(c->name,builtins[i].name))return help_summaries[i];
    return c->usage;
}
static void help_items(ps_session *s,const char *heading,const ps_help_item *items,unsigned count,unsigned role){
    unsigned i,width=0,limit=help_width(s)/3;
    if(!items||!count)return;
    for(i=0;i<count;++i)if(help_cells(s,items[i].label)>width)width=help_cells(s,items[i].label);
    if(width>limit)width=limit;
    ps_newline(s);help_heading(s,heading);
    for(i=0;i<count;++i)help_row(s,items[i].label,items[i].description,width,role);
}
#endif
PS_API void ps_set_help_documents(ps_session *s,const ps_help_document *docs,unsigned count){
#if PS_HELP_DETAILS
    if(s){s->help_documents=docs;s->help_document_count=docs?count:0;}
#else
    (void)s;(void)docs;(void)count;
#endif
}
PS_API int ps_print_help(ps_session *s,const char *name,unsigned layout){
    unsigned i,j,k,printed=0,width=0,group_width=0;
    if(!s)return 0;
    for(i=0;i<s->command_count;++i){
        const ps_command *c=s->commands+i;
        if(help_cells(s,c->name)>width)width=help_cells(s,c->name);
        if(help_cells(s,command_category(c))>group_width)group_width=help_cells(s,command_category(c));
    }
    if(width>help_width(s)/3)width=help_width(s)/3;
    for(i=0;i<s->command_count;++i){
        const ps_command *c=s->commands+i;
        if(name){
            if(!name_match(s,c->name,name,0))continue;
#if PS_HELP_DETAILS
            {const ps_help_document *d=help_document(s,c->name);
            help_heading(s,"Description");out(s,"  ");help_text(s,help_description(s,c),2,2);ps_newline(s);
#endif
            ink(s,PS_INK_HEADING);out(s,"Usage: ");reset(s);ps_newline(s);
            out(s,"  ");ink(s,PS_INK_COMMAND);ps_write(s,c->name);out(s," ");if(s->io.flags&PS_COLOR)reset(s);
            help_text(s,c->usage,3+help_cells(s,c->name),4);
#if PS_HELP_DETAILS
            if(d){help_items(s,"Arguments",d->arguments,d->argument_count,PS_INK_NUMBER);
                help_items(s,"Options",d->options,d->option_count,PS_INK_SUCCESS);
                help_items(s,"Examples",d->examples,d->example_count,PS_INK_COMMAND);}
            }
#endif
            return 1;
        }
        if(layout==PS_HELP_COMPACT){
            ink(s,0);field(s,command_category(c),group_width+2);
            ink(s,PS_INK_COMMAND);
            if(help_cells(s,c->name)>width){help_text(s,c->name,group_width+2,2);reset(s);spaces(s,2);help_text(s,c->usage,2,2);++printed;continue;}
            field(s,c->name,width+2);reset(s);
            help_text(s,c->usage,group_width+width+4,2);++printed;continue;
        }
        for(j=0;j<i;++j)if(equal(command_category(s->commands+j),command_category(c)))break;
        if(j!=i)continue;
        if(printed)ps_newline(s);
        help_heading(s,command_category(c));
        for(k=i;k<s->command_count;++k){
            const ps_command *entry=s->commands+k;
            if(!equal(command_category(entry),command_category(c)))continue;
#if PS_HELP_DETAILS
            help_row(s,entry->name,help_description(s,entry),width,PS_INK_COMMAND);
#else
            help_row(s,entry->name,entry->usage,width,PS_INK_COMMAND);
#endif
            ++printed;
        }
    }
    return (int)printed;
}
