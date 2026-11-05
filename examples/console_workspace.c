#include "console_workspace.h"
#include "portable_style.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned cw_find(const cw_workspace *w,const char *id){
    unsigned i;for(i=0;id&&i<w->count;++i)if(!strcmp(w->screens[i].id,id))return i;
    return w->count;
}
static unsigned cw_id(const cw_workspace *w,unsigned screen,unsigned entry){return w->offsets[screen]+entry;}
static char *cw_value(cw_workspace *w,unsigned screen,unsigned entry){return w->values[w->slots[cw_id(w,screen,entry)]];}
static const cw_entry *cw_definition(cw_workspace *w,unsigned id){
    unsigned i;for(i=0;i<w->count;++i)if(id>=w->offsets[i]&&id<w->offsets[i]+w->screens[i].count)return &w->screens[i].entries[id-w->offsets[i]];
    return 0;
}
static const char *cw_notice(void *context,const ps_menu *menu,unsigned *role){
    cw_workspace *w=context;(void)menu;*role=PS_INK_WARNING;
    return w->notice[0]?w->notice:w->screens[w->current].notice;
}
static const char *cw_description(void *context,const ps_menu *menu,unsigned index){
    cw_workspace *w=context;(void)menu;
    return index<w->screens[w->current].count?w->screens[w->current].entries[index].initial:"";
}
static int cw_action_run(ps_session *s,void *context){
    cw_action *a=context;cw_workspace *w=a->owner;(void)s;
    w->pending=w->screens[w->current].entries[a->entry].target;
    if(!w->pending||!w->pending[0]){
        ++w->requests;snprintf(w->notice,sizeof w->notice,"Simulation: %s acknowledged locally",w->screens[w->current].entries[a->entry].label);
        w->pending=w->screens[w->current].id;
    }
    return 0;
}
static int cw_read(void *context,unsigned id,ps_control_value *v){
    cw_workspace *w=context;const cw_entry *e=cw_definition(w,id);unsigned i;const char *text;
    if(!e)return 0;
    text=w->values[w->slots[id]];memset(v,0,sizeof *v);v->flags=PS_CONTROL_VALID;v->revision=w->revision;
    v->number=(unsigned)strtoul(text,0,10);
    if(e->kind==CW_CHOICE)for(i=0;i<e->choice_count;++i)if(!strcmp(text,e->choices[i]))v->number=i;
    if(e->kind==CW_NUMBER)v->flags|=PS_CONTROL_NUMBER_ONLY;
    snprintf(v->text,sizeof v->text,"%s",e->kind==CW_SECRET?"••••••••":text);
    return 1;
}
static int cw_write(void *context,unsigned id,const ps_control_value *v,ps_u32 revision){
    cw_workspace *w=context;const cw_entry *e=cw_definition(w,id);char *text;
    if(!e||revision!=w->revision||e->kind==CW_LABEL||e->kind==CW_SECRET)return 0;
    text=w->values[w->slots[id]];
    if(e->kind==CW_ACTION){
        ++w->requests;w->pending=e->target&&e->target[0]?e->target:w->screens[w->current].id;
        snprintf(w->notice,sizeof w->notice,"Simulation: %s requested locally",e->label);
    }else if(e->kind==CW_CHOICE){if(v->number>=e->choice_count)return 0;snprintf(text,CW_VALUE_SIZE,"%s",e->choices[v->number]);}
    else if(e->kind==CW_NUMBER||e->kind==CW_TOGGLE){if(v->number<e->minimum||v->number>e->maximum)return 0;snprintf(text,CW_VALUE_SIZE,"%u",v->number);}
    else snprintf(text,CW_VALUE_SIZE,"%s",v->text);
    ++w->revision;return 1;
}
static const char *cw_validate(void *context,unsigned field,const char *value){
    cw_workspace *w=context;const cw_entry *e=&w->screens[w->current].entries[field];
    if(e->kind==CW_NUMBER){char *end;unsigned long number=strtoul(value,&end,10);
        if(!*value||*end||number<e->minimum||number>e->maximum)return "Enter a number within the configured range.";}
    return 0;
}
static void cw_done(ps_session *s,void *context,int accepted){
    cw_workspace *w=context;const cw_screen *screen=&w->screens[w->current];unsigned i;(void)s;
    if(accepted){
        for(i=0;i<screen->count;++i)snprintf(cw_value(w,w->current,i),CW_VALUE_SIZE,"%s",w->drafts[i]);
        ++w->accepted;++w->revision;
    }
    snprintf(w->notice,sizeof w->notice,"Simulation: %s",accepted?"values accepted locally; no remote request":"cancelled; values unchanged");
    w->pending=screen->parent?screen->parent:screen->id;
}
int cw_init(cw_workspace *w,ps_session *s,const cw_screen *screens,unsigned count){
    unsigned i,j,total=0;
    if(!w||!s||!screens||!count||count>CW_MAX_SCREENS)return 0;
    memset(w,0,sizeof *w);w->session=s;w->screens=screens;w->count=count;w->revision=1;
    for(i=0;i<count;++i){
        if(screens[i].count>CW_MAX_ENTRIES||(screens[i].kind==CW_FORM&&screens[i].count>CW_MAX_FIELDS))return 0;
        w->offsets[i]=total;
        for(j=0;j<screens[i].count;++j){const cw_entry *e=&screens[i].entries[j];unsigned k,slot=w->value_count;
            if(total>=CW_MAX_VALUES)return 0;
            if(e->binding&&*e->binding)for(k=0;k<total;++k){const cw_entry *old=cw_definition(w,k);
                if(old&&old->binding&&!strcmp(old->binding,e->binding)){slot=w->slots[k];break;}}
            w->slots[total++]=slot;
            if(slot==w->value_count){snprintf(w->values[slot],CW_VALUE_SIZE,"%s",e->initial?e->initial:"");++w->value_count;}
        }
    }
    return 1;
}
int cw_open(cw_workspace *w,const char *id){
    unsigned index=cw_find(w,id),i,j,page_count=0,selected=0;
    const cw_screen *screen;ps_session *s=w->session;
    if(index==w->count)return 0;
    /* Closing is owned by the outer application, never a read/write callback. */
    if(ps_editor_active(s)){ps_key(s,27);if(ps_editor_active(s))return 0;}
    ps_form_close(s);ps_menu_close(s);ps_key(s,27);
    if(!ps_panel_attach(s,0))return 0;
    w->pending=0;w->current=index;screen=&w->screens[index];
    if(screen->kind==CW_DOCUMENT){
        ps_editor_set_style(s,PS_EDITOR_HEADER|PS_EDITOR_BORDER|PS_EDITOR_READ_ONLY);
        return ps_editor_open(s,screen->title,screen->document?screen->document:"",0,0);
    }
    if(screen->kind==CW_FORM){
        for(i=0;i<screen->count;++i){const cw_entry *e=&screen->entries[i];ps_form_field *f=&w->fields[i];
            snprintf(w->drafts[i],sizeof w->drafts[i],"%s",cw_value(w,index,i));
            f->label=e->label;f->kind=e->kind==CW_SECRET?PS_FORM_PASSWORD:e->kind==CW_CHOICE?PS_FORM_CHOICE:PS_FORM_TEXT;
            f->value=w->drafts[i];f->capacity=sizeof w->drafts[i];f->minimum=e->kind==CW_SECRET?4:1;f->maximum=CW_VALUE_SIZE-1;
            f->choices=e->choices;f->choice_count=e->choice_count;
        }
        w->form.fields=w->fields;w->form.count=screen->count;w->form.validate=cw_validate;w->form.done=cw_done;w->form.context=w;
        ps_form_set_style(s,PS_FORM_OVERVIEW);
        ps_form_set_title(s,screen->title);
        return ps_form_open(s,&w->form);
    }
    if(screen->kind==CW_MENU){
        for(i=0;i<screen->count;++i){ps_menu_item *item=&w->menu_items[i];
            w->actions[i].owner=w;w->actions[i].entry=i;
            item->label=screen->entries[i].label;item->submenu=0;item->run=cw_action_run;item->context=&w->actions[i];item->flags=0;
        }
        w->menu.title=screen->title;w->menu.items=w->menu_items;w->menu.count=screen->count;
        ps_menu_set_style(s,PS_MENU_TABLE|PS_MENU_NUMBERS|PS_MENU_DETAILS|PS_MENU_MARKER);
        ps_menu_set_description(s,cw_description,w);ps_menu_set_notice(s,cw_notice,w);
        return ps_menu_open(s,&w->menu);
    }
    for(i=0;i<w->count;++i){const cw_screen *candidate=&w->screens[i];
        if(i!=index&&(!screen->family||!*screen->family||!candidate->family||strcmp(screen->family,candidate->family)))continue;
        if(candidate->kind!=CW_TABLE||page_count>=CW_MAX_TABS)return 0;
        if(i==index)selected=page_count;
        for(j=0;j<candidate->count;++j){const cw_entry *e=&candidate->entries[j];ps_control *c=&w->controls[page_count][j];
            static const unsigned kinds[]={PS_CONTROL_TEXT,PS_CONTROL_LABEL,PS_CONTROL_CHOICE,PS_CONTROL_TOGGLE,PS_CONTROL_LEVEL,PS_CONTROL_LABEL,PS_CONTROL_ACTION};
            c->id=cw_id(w,i,j);c->group=e->group?e->group:"";c->label=e->label;c->kind=kinds[e->kind];
            c->minimum=e->minimum;c->maximum=e->maximum;c->step=1;c->choices=e->choices;c->choice_count=e->choice_count;
        }
        w->pages[page_count].title=candidate->tab&&*candidate->tab?candidate->tab:candidate->title;
        w->pages[page_count].controls=w->controls[page_count];w->pages[page_count].count=candidate->count;++page_count;
    }
    w->panel.title=screen->family&&*screen->family?"Endpoint workspace":screen->title;w->panel.status=w->notice[0]?w->notice:screen->notice;
    w->panel.pages=w->pages;w->panel.count=page_count;w->panel.read=cw_read;w->panel.write=cw_write;w->panel.context=w;
    ps_set_table_style(s,PS_TABLE_DEFAULT|PS_TABLE_RED_BORDER|PS_TABLE_BACK_TO_MENU);
    return ps_panel_attach(s,&w->panel)&&ps_panel_open(s,selected);
}
void cw_dispatch(cw_workspace *w){
    const char *target=w->pending;const cw_screen *screen=&w->screens[w->current];
    if(target){w->pending=0;cw_open(w,target);return;}
    /* Mode closure is observed after decoding, so raw bytes and decoded keys
       return to the same parent (including Escape resolved by ps_tick). */
    if(screen->parent&&*screen->parent&&
       ((screen->kind==CW_MENU&&!ps_menu_active(w->session))||
        (screen->kind==CW_TABLE&&!ps_panel_active(w->session))||
        (screen->kind==CW_DOCUMENT&&!ps_editor_active(w->session))))cw_open(w,screen->parent);
}
void cw_key(cw_workspace *w,unsigned key){
    ps_key(w->session,key);cw_dispatch(w);
}
void cw_feed(cw_workspace *w,const char *bytes,unsigned count){ps_feed(w->session,bytes,count);cw_dispatch(w);}
void cw_tick(cw_workspace *w,ps_u32 milliseconds){ps_tick(w->session,milliseconds);cw_dispatch(w);}
