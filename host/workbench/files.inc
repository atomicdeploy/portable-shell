static const ps_file_node session_files[]={ {"Session",PS_FILE_ROOT,PS_FILE_DIRECTORY},{"notes.txt",0,0} };
static void session_file_pick(ps_session *s,void *ctx,unsigned id){(void)ctx;if(id==1)ps_execute(s,"edit --framed");}
