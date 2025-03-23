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


char* sb_cmd_push_arg_sized(sb_cmd* c, char* str, uint32_t len);
void sb_cmd_clear_args(sb_cmd* c);
void sb_cmd_free(sb_cmd* c);

int sb_cmd_sync(sb_cmd* c);
pid_t sb_cmd_async(sb_cmd* c);

int sb_cmd_fence(pid_t id);

#ifdef SB_IMPL
#include "sb.c"
#endif

#define sb_cmd_push_arg(c, x) sb_cmd_push_arg_sized(c, x, sizeof(x))

#endif
