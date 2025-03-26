#ifndef SB_H
#define SB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/wait.h>
#include "unistd.h"

#define SB_MIN_TEXT_SIZE 1
#define SB_MIN_CMD_NUM 1

typedef struct {
    char* textbuffer;
    uint32_t tsize;
    uint32_t tcap;
    uint32_t asize;
} sb_cmd;

typedef pid_t fproc(void);
extern fproc* fork_;

typedef int execproc(const char*, char* const *);
extern execproc* execvp_;

typedef void exitproc(int);
extern exitproc* exit_;


void sb_cmd_push_args(sb_cmd* c, uint32_t num, ...);
void sb_cmd_clear_args(sb_cmd* c);
void sb_cmd_free(sb_cmd* c);

int sb_cmd_sync(sb_cmd* c);
pid_t sb_cmd_async(sb_cmd* c);

int sb_cmd_fence(uint32_t);

int sb_should_rebuild(const char* srcpath, const char* binpath);
void sb_rebuild_self(int argc, char* argv[], const char* srcpath);



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
