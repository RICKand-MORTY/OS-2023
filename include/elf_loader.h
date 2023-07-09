#ifndef _ELF_LOADER_H
#define _ELF_LOADER_H


#include "elf.h"


int load_elf(char *path, loader_env_t user_data, ELFExec_t **exec_ptr,char* argv, char* envp, int fd);
static int initElf(ELFExec_t *e);
#endif