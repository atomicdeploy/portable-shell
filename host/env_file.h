#ifndef PS_HOST_ENV_FILE_H
#define PS_HOST_ENV_FILE_H
#include "env_document.h"
#define PS_ENV_PATH 1024u
/* Existing regular files only. No symlink following, no environment evaluation. */
int env_file_read(const char *path,char *bytes,unsigned *size,const char **error);
int env_file_save(const char *path,env_document *,const char **error);
int env_file_create(const char *path,const char **error);
#endif
