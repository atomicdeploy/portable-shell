#ifndef PS_HOST_ENV_DOCUMENT_H
#define PS_HOST_ENV_DOCUMENT_H
/* Host-side lossless dotenv document. No evaluation or process environment writes.
   All offsets refer to UTF-8 source bytes, not display columns. */
#ifndef PS_ENV_BYTES
#define PS_ENV_BYTES 65536u
#endif
#ifndef PS_ENV_RECORDS
#define PS_ENV_RECORDS 1024u
#endif
enum { ENV_ASSIGNMENT=1, ENV_DISABLED=2, ENV_COMMENT=4, ENV_BLANK=8,
       ENV_UNKNOWN=16, ENV_MULTILINE=32, ENV_DUPLICATE=64 };
typedef struct {
    unsigned start,end,body_end,key,key_size,value,value_size,comment,flags,line;
} env_record;
typedef struct {
    char text[PS_ENV_BYTES],saved[PS_ENV_BYTES],undo[PS_ENV_BYTES],scratch[PS_ENV_BYTES],backup[PS_ENV_BYTES];
    env_record records[PS_ENV_RECORDS];
    unsigned size,saved_size,undo_size,count,bom,revision,undo_valid,crlf;
    const char *error;
} env_document;
void env_zero(void *,unsigned);
void env_copy(void *,const void *,unsigned);
unsigned env_length(const char *);
int env_equal(const char *,const char *);
void env_string(char *,unsigned,const char *);
int env_parse(env_document *);
int env_load(env_document *,const char *,unsigned);
int env_dirty(const env_document *);
int env_replace(env_document *,unsigned start,unsigned end,const char *,unsigned);
int env_set_value(env_document *,unsigned record,const char *);
int env_toggle(env_document *,unsigned record);
int env_move(env_document *,unsigned record,int direction);
int env_undo(env_document *);
int env_key_valid(const char *);
int env_value_valid(const char *);
int env_find(const env_document *,unsigned record,const char *query);
void env_mark_saved(env_document *);
#endif
