#include "env_document.h"
void env_zero(void *dst,unsigned n){unsigned char *p=dst;while(n--)*p++=0;}
void env_copy(void *dst,const void *src,unsigned n){unsigned char *d=dst;const unsigned char *s=src;while(n--)*d++=*s++;}
unsigned env_length(const char *s){unsigned n=0;while(s[n])++n;return n;}
int env_equal(const char *a,const char *b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
void env_string(char *dst,unsigned cap,const char *src){unsigned i=0;if(!cap)return;while(src[i]&&i+1<cap){dst[i]=src[i];++i;}dst[i]=0;}
static int same(const char *a,const char *b,unsigned n){while(n--)if(*a++!=*b++)return 0;return 1;}
static int space(char c){return c==' '||c=='\t';}
static int first(char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_';}
static int keychar(char c){return first(c)||(c>='0'&&c<='9');}
int env_key_valid(const char *s){if(!first(*s++))return 0;while(*s)if(!keychar(*s++))return 0;return 1;}
int env_value_valid(const char *s){
    unsigned i=0,n=env_length(s);char quote;
    if(!n)return 1;
    if(space(s[0])||space(s[n-1])||s[n-1]=='\r')return 0;
    quote=s[0]=='\''||s[0]=='"'?s[0]:0;
    if(quote){
        for(i=1;i<n;++i){if(s[i]==quote)return i+1==n;if(s[i]=='\\'&&quote=='"'&&i+1<n)++i;}
        return 0;
    }
    for(i=0;i<n;++i){if(s[i]=='\n'||s[i]=='\r'||(s[i]=='#'&&(!i||space(s[i-1]))))return 0;
        if(s[i]=='\\'&&i+1<n)++i;}
    return 1;
}
static unsigned line_end(const env_document *d,unsigned at){while(at<d->size&&d->text[at]!='\n')++at;return at;}
static unsigned trim_end(const env_document *d,unsigned at,unsigned begin){while(at>begin&&(space(d->text[at-1])||d->text[at-1]=='\r'))--at;return at;}
static int valid_text(const char *text,unsigned n){
    unsigned i=0;
    while(i<n){unsigned c=(unsigned char)text[i++],left,cp,min;
        if(c<128){if((c<32&&c!='\n'&&c!='\r'&&c!='\t')||c==127)return 0;continue;}
        if(c>=194&&c<=223){left=1;cp=c&31;min=128;}
        else if(c>=224&&c<=239){left=2;cp=c&15;min=2048;}
        else if(c>=240&&c<=244){left=3;cp=c&7;min=65536;}else return 0;
        while(left--){if(i>=n||((unsigned char)text[i]&192)!=128)return 0;cp=(cp<<6)|((unsigned char)text[i++]&63);}
        if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return 0;
    }
    return 1;
}
int env_parse(env_document *d){
    unsigned at,line=1,i,j;d->count=0;d->crlf=0;d->error=0;
    d->bom=d->size>=3&&(unsigned char)d->text[0]==239&&(unsigned char)d->text[1]==187&&(unsigned char)d->text[2]==191?3:0;
    if(!valid_text(d->text,d->size)){d->error="Expected UTF-8 text without terminal control bytes";return 0;}
    at=d->bom;
    while(at<d->size){env_record *r;unsigned p,e,tail,k;char quote=0;
        if(d->count==PS_ENV_RECORDS){d->error="Document has too many records";return 0;}
        r=&d->records[d->count++];env_zero(r,sizeof *r);r->start=at;r->line=line;
        e=line_end(d,at);r->body_end=e;if(e>at&&d->text[e-1]=='\r'){--r->body_end;d->crlf=1;}
        r->end=e+(e<d->size);p=at;while(p<r->body_end&&space(d->text[p]))++p;
        if(p==r->body_end){r->flags=ENV_BLANK;goto complete;}
        if(d->text[p]=='#'){r->flags=ENV_DISABLED;++p;while(p<r->body_end&&space(d->text[p]))++p;}
        if(r->body_end-p>=7&&same(d->text+p,"export",6)&&space(d->text[p+6])){p+=7;while(p<r->body_end&&space(d->text[p]))++p;}
        r->key=p;
        if(p>=r->body_end||!first(d->text[p]))goto unknown;
        while(p<r->body_end&&keychar(d->text[p]))++p;
        r->key_size=p-r->key;while(p<r->body_end&&space(d->text[p]))++p;
        if(p==r->body_end||d->text[p++]!='=')goto unknown;
        while(p<r->body_end&&space(d->text[p]))++p;
        r->value=p;tail=p;
        if(p<r->body_end&&(d->text[p]=='\''||d->text[p]=='"')){
            quote=d->text[p++];
            while(p<d->size){
                if(d->text[p]=='\n'&&(r->flags&ENV_DISABLED)){
                    unsigned next=p+1;while(next<d->size&&space(d->text[next]))++next;
                    if(next>=d->size||d->text[next]!='#')break;
                    p=next+1;if(d->text[p]==' ')++p;continue;
                }
                if(d->text[p]==quote){++p;quote=0;break;}
                if(d->text[p]=='\\'&&quote=='"'&&p+1<d->size)p+=2;else ++p;
            }
            if(quote){if(!(r->flags&ENV_DISABLED)){r->body_end=d->size;r->end=d->size;}goto unknown;}
            tail=p;e=line_end(d,p);r->body_end=e;if(e>p&&d->text[e-1]=='\r')--r->body_end;
            r->end=e+(e<d->size);
            while(p<r->body_end&&space(d->text[p]))++p;
            if(p<r->body_end&&d->text[p]!='#')goto unknown;
        }else{
            while(p<r->body_end){
                if(d->text[p]=='#'&&(p==r->value||space(d->text[p-1])))break;
                if(d->text[p]=='\\'&&p+1<r->body_end)p+=2;else ++p;
            }
            tail=trim_end(d,p,r->value);
        }
        r->value_size=tail-r->value;if(p<r->body_end&&d->text[p]=='#')r->comment=p;
        r->flags|=ENV_ASSIGNMENT;
        for(k=r->start;k<r->body_end;++k)if(d->text[k]=='\n')r->flags|=ENV_MULTILINE;
        goto complete;
unknown:
        /* Unsupported syntax is preserved, not interpreted or silently repaired. */
        r->flags=(r->flags&ENV_DISABLED)?ENV_COMMENT:ENV_UNKNOWN;r->key_size=0;
complete:
        for(k=r->start;k<r->end;++k)if(d->text[k]=='\n')++line;
        at=r->end;
    }
    /* Presentation hint only: a comment block separated from preceding values
       by whitespace can introduce the next assignments. Use its last nonempty
       comment as the heading; do not mistake disabled assignments for comments. */
    for(i=0;i<d->count;++i)if((d->records[i].flags&ENV_COMMENT)&&
        (!i||(d->records[i-1].flags&ENV_BLANK))){
        unsigned last=i,next_record=i,p;
        while(next_record<d->count&&(d->records[next_record].flags&ENV_COMMENT)){
            p=d->records[next_record].start;
            while(p<d->records[next_record].body_end&&(space(d->text[p])||d->text[p]=='#'))++p;
            if(p<d->records[next_record].body_end)last=next_record;
            ++next_record;
        }
        while(next_record<d->count&&(d->records[next_record].flags&ENV_BLANK))++next_record;
        if(next_record<d->count&&(d->records[next_record].flags&ENV_ASSIGNMENT)){
            p=d->records[last].start;
            while(p<d->records[last].body_end&&(space(d->text[p])||d->text[p]=='#'))++p;
            if(p<d->records[last].body_end)d->records[last].flags|=ENV_HEADING;
        }
    }
    for(i=0;i<d->count;++i)if(d->records[i].flags&ENV_ASSIGNMENT)
        for(j=0;j<i;++j)if((d->records[j].flags&ENV_ASSIGNMENT)&&d->records[i].key_size==d->records[j].key_size&&
            same(d->text+d->records[i].key,d->text+d->records[j].key,d->records[i].key_size)){
            d->records[i].flags|=ENV_DUPLICATE;d->records[j].flags|=ENV_DUPLICATE;
        }
    return 1;
}
int env_load(env_document *d,const char *text,unsigned size){
    if(size>=PS_ENV_BYTES){d->error="Document exceeds configured capacity";return 0;}
    env_copy(d->text,text,size);d->text[size]=0;d->size=size;d->undo_valid=0;
    if(!env_parse(d))return 0;
    env_mark_saved(d);++d->revision;return 1;
}
void env_mark_saved(env_document *d){env_copy(d->saved,d->text,d->size+1);d->saved_size=d->size;}
int env_dirty(const env_document *d){return d->size!=d->saved_size||!same(d->text,d->saved,d->size);}
int env_replace(env_document *d,unsigned begin,unsigned end,const char *text,unsigned size){
    unsigned total,old_size=d->size;
    if(end<begin||end>d->size||size>=PS_ENV_BYTES||d->size-(end-begin)>=PS_ENV_BYTES-size){d->error="Edit exceeds document capacity";return 0;}
    total=d->size-(end-begin)+size;
    /* Stage first: replacement may point inside the current document. */
    env_copy(d->scratch,d->text,begin);env_copy(d->scratch+begin,text,size);
    env_copy(d->scratch+begin+size,d->text+end,d->size-end);d->scratch[total]=0;
    /* Preserve the previous undo on rejected edits. */
    env_copy(d->backup,d->text,d->size+1);
    env_copy(d->text,d->scratch,total+1);d->size=total;
    if(!env_parse(d)){
        const char *error=d->error;
        env_copy(d->text,d->backup,old_size+1);d->size=old_size;env_parse(d);
        d->error=error;return 0;
    }
    env_copy(d->undo,d->backup,old_size+1);d->undo_size=old_size;d->undo_valid=1;
    ++d->revision;return 1;
}
/* All public mutations take a snapshot before committing a splice. */
static int change(env_document *d,unsigned begin,unsigned end,const char *text,unsigned n){
    return env_replace(d,begin,end,text,n);
}
int env_set_value(env_document *d,unsigned index,const char *value){
    env_record r;unsigned count,flags;int result;
    if(index>=d->count||!(d->records[index].flags&ENV_ASSIGNMENT))return 0;
    if(!env_value_valid(value)){d->error="Invalid value: quote leading spaces, # comments and multiline text";return 0;}
    r=d->records[index];count=d->count;flags=r.flags;
    if(r.comment==r.value&&*value){unsigned n=env_length(value);if(n+1>=PS_ENV_BYTES)return 0;
        env_copy(d->backup,value,n);d->backup[n++]=' ';result=change(d,r.value,r.value,d->backup,n);
    }else result=change(d,r.value,r.value+r.value_size,value,env_length(value));
    if(result&&(d->count!=count||d->records[index].value_size!=env_length(value)||!(d->records[index].flags&ENV_ASSIGNMENT)||
       ((d->records[index].flags^flags)&ENV_DISABLED))){env_undo(d);d->error="Enter one valid dotenv value; quote spaces, comments and multiline text";return 0;}
    return result;
}
int env_toggle(env_document *d,unsigned index){
    env_record r;unsigned p;
    if(index>=d->count||!(d->records[index].flags&ENV_ASSIGNMENT))return 0;
    r=d->records[index];
    if(r.flags&ENV_MULTILINE){
        unsigned at=r.start,out=0;char *buffer=d->backup;
        while(at<r.end){unsigned end=line_end(d,at),prefix=at;
            while(prefix<end&&space(d->text[prefix]))++prefix;
            if(r.flags&ENV_DISABLED){
                if(d->text[prefix]!='#'){d->error="Every continuation line must be commented";return 0;}
                env_copy(buffer+out,d->text+at,prefix-at);out+=prefix-at;++prefix;if(d->text[prefix]==' ')++prefix;
                env_copy(buffer+out,d->text+prefix,end-prefix);out+=end-prefix;
            }else{
                if(out+2+end-at+1>=PS_ENV_BYTES){d->error="Commented record exceeds capacity";return 0;}
                buffer[out++]='#';buffer[out++]=' ';env_copy(buffer+out,d->text+at,end-at);out+=end-at;
            }
            if(end<r.end)buffer[out++]='\n';
            at=end+(end<r.end);
        }
        return change(d,r.start,r.end,buffer,out);
    }
    p=r.start;while(p<r.body_end&&space(d->text[p]))++p;
    if(r.flags&ENV_DISABLED){unsigned end=p+1;if(d->text[end]==' ')++end;return change(d,p,end,"",0);}
    return change(d,p,p,"# ",2);
}
int env_move(env_document *d,unsigned index,int direction){
    env_record a,b;unsigned n,k;
    if(index>=d->count||(!index&&direction<0)||(index+1==d->count&&direction>0))return 0;
    if(direction<0)--index;
    a=d->records[index];b=d->records[index+1];
    if(b.end==d->size&&d->text[b.end-1]!='\n'){d->error="Add a final newline before moving the last record";return 0;}
    n=b.end-a.start;env_copy(d->undo,d->text,d->size+1);d->undo_size=d->size;d->undo_valid=1;
    k=b.end-b.start;env_copy(d->scratch,d->text+b.start,k);env_copy(d->scratch+k,d->text+a.start,a.end-a.start);
    env_copy(d->text+a.start,d->scratch,n);++d->revision;return env_parse(d);
}
int env_undo(env_document *d){unsigned n;
    if(!d->undo_valid)return 0;
    n=d->size;env_copy(d->scratch,d->text,n+1);env_copy(d->text,d->undo,d->undo_size+1);
    d->size=d->undo_size;env_copy(d->undo,d->scratch,n+1);d->undo_size=n;++d->revision;return env_parse(d);
}
int env_find(const env_document *d,unsigned index,const char *query){
    unsigned p,n=env_length(query);const env_record *r=&d->records[index];
    if(!n)return 1;
    for(p=r->key;p+n<=r->key+r->key_size;++p)if(same(d->text+p,query,n))return 1;
    return 0;
}
