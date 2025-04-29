#ifndef SB_H
#define SB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


#define SB_MIN_TEXT_SIZE 32
#define SB_MIN_CMD_NUM 1

//Process handle
#if defined(_WIN32)
typedef void* SB_PHANDLE;
#else
typedef int SB_PHANDLE;
#endif

//file handle
#if defined(_WIN32)
typedef void* SB_FHANDLE;
#else
typedef FILE* SB_FHANDLE;
#endif

typedef struct {
    char* textbuffer;
    uint32_t tsize;
    uint32_t tcap;
    uint32_t asize;
} sb_cmd;

void sb_cmd_push_args(sb_cmd* c, uint32_t num, ...);
void sb_cmd_clear_args(sb_cmd* c);
void sb_cmd_free(sb_cmd* c);

int sb_cmd_sync(sb_cmd* c);
SB_PHANDLE sb_cmd_async(sb_cmd* c);
int sb_cmd_sync_and_reset(sb_cmd* c);
SB_PHANDLE sb_cmd_async_and_reset(sb_cmd* c);

int sb_cmd_wait(SB_PHANDLE h);

int sb_should_rebuild(const char* srcpath, const char* binpath);
void sb_rebuild_self(int argc, char* argv[], const char* srcpath);

void sb_cmd_pushf(sb_cmd* c, const char* __restrict format, ...);

//utility
void sb_pick_compiler(void);
void sb_set_compiler(char* str);
char* sb_compiler(void);

#ifdef _WIN32
    #define sb_cmd_flag(f) "/" f
#else
    #define sb_cmd_flag(f) "-" f
#endif

//platform
void sb_fprintf(SB_FHANDLE h, char* format, ...);
void sb_mkdir(char* path);
void sb_rename(char* filepath, char* name);

SB_FHANDLE sb_fopen(char* file, char* mode);
void sb_fclose(SB_FHANDLE h);

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))
#define sb_cmd_push(c, ...) \
    sb_cmd_push_args(c, \
            ARRAY_SIZE(((const char*[]){ __VA_ARGS__ })), \
            __VA_ARGS__ )
#define AUTO_REBUILD(c, v) \
    sb_rebuild_self(c, v, __FILE__)

#ifdef SB_IMPL
#include "sb.c"
#endif


#endif
