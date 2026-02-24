#ifndef SB_BUILD_H
#define SB_BUILD_H

#include <stdint.h>

// Utils
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

/*
    Inline Dynamic Arrays
    Borrowed from SimpleDS (no allocator interface)
*/

#define dynArray(type)                                                         \
    struct {                                                                   \
        u64 size;                                                              \
        u64 cap;                                                               \
        type *data;                                                            \
    }

#define dynPush(array, val)                                                    \
    (((array).size + 1 > (array).cap)                                          \
         ? (((array)).data =                                                   \
                realloc((array).data, ((array).cap ? (array).cap * 2 : 4) *    \
                                          sizeof((array).data[0])),            \
            (array).cap = (array).cap ? (array).cap * 2 : 4)                   \
         : 0,                                                                  \
     (array).data[(array).size++] = (val))

#define dynReserve(array, size)                                                \
    ((array.cap < size)                                                        \
         ? (array.data = realloc(array.data, size * sizeof(array.data[0])),    \
            array.cap = size)                                                  \
         : 0)

// INFO(ELI): This macro can't be included in expressions and must be
// on its own line. This is due to the loops which I could fix if
// I added more global functions but I decided I preferred to not
// do that.
#define dynResize(array, newsize)                                              \
    do {                                                                       \
        if ((array).cap < (newsize)) {                                         \
            while ((array).cap < (newsize)) {                                  \
                (array).cap = (array).cap ? (array).cap * 2 : 4;               \
            }                                                                  \
            (array).data =                                                     \
                realloc((array).data, (array).cap * sizeof((array).data[0]));  \
        }                                                                      \
                                                                               \
        if ((array).size < (newsize)) {                                        \
            char *d = (char *)&(array).data[(array).size];                     \
            for (u32 i = 0;                                                    \
                 i < ((newsize) - (array).size) * sizeof((array).data[0]);     \
                 i++)                                                          \
                d[i] = 0;                                                      \
        }                                                                      \
        (array).size = (newsize);                                              \
    } while (0);

#define dynBack(array) (array.data[array.size - 1])

#define dynFree(array) (free(array.data))

// INFO(ELI): These Macros cannot be part of expressions since they require
// loops.

#define dynIns(array, idx, val)                                                \
    do {                                                                       \
        dynResize(array, array.size + 1);                                      \
        for (i64 i = array.size - 1; i >= idx; i--) {                          \
            array.data[i + 1] = array.data[i];                                 \
        }                                                                      \
        array.data[idx] = val;                                                 \
    } while (0)

#define dynDel(array, idx)                                                     \
    do {                                                                       \
        for (u32 i = idx; i < array.size - 1; i++) {                           \
            array.data[i] = array.data[i + 1];                                 \
        }                                                                      \
        dynResize(array, array.size - 1);                                      \
    } while (0)

#define dynExt(array, vals, num)                                               \
    do {                                                                       \
        dynResize((array), (array).size + (num));                              \
        for (u64 i = (array).size - (num); i < (array).size; i++) {            \
            (array).data[i] = (vals)[i - (array).size + (num)];                \
        }                                                                      \
    } while (0)

// String Builder
typedef struct StringBuilder {
    dynArray(char) str;
} StringBuilder;

void SBPushChar(StringBuilder *b, char c);
void SBPushStr(StringBuilder *b, char *str);
void SBResetString(StringBuilder *b);
void SBFreeString(StringBuilder b);

// Cmd List
typedef struct Cmd {
    dynArray(u32) args;
    u8 fence; // flag to wait for all previous commands to finish
} Cmd;

// Cmd Helpers

typedef struct CmdList {
    dynArray(char) strs;
    dynArray(Cmd) cmds;
    dynArray(char *) curr_cmd;
} CmdList;

// helpers
u32 AddStr(CmdList *list, char *str);
void AddCmd(CmdList *list, Cmd cmd);
void ExecuteCmd(CmdList *list, Cmd cmd, u32 num_jobs);
void ExecuteCmdList(CmdList *list, u32 max_processes);
void FreeCmdList(CmdList list);

void PushCmdArg(CmdList *list, Cmd *c, char *str);

// Builders

// Executable
typedef struct Exec {
    u32 exe_name;

    dynArray(u32) sources;
    dynArray(u32) flags;
} Exec;

void SetName(CmdList *list, Exec *e, char *name);
void AddSource(CmdList *list, Exec *e, char *filename);
void AddFlag(CmdList *list, Exec *e, char *flag);
void PushExec(CmdList *list, Exec e); // frees Exec

#ifdef SB_IMPL
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// String builder Functions
void SBPushChar(StringBuilder *b, char c) {
    if (b->str.size)
        b->str.size--;
    dynPush(b->str, c);
    dynPush(b->str, 0);
}
void SBPushStr(StringBuilder *b, char *str) {
    b->str.size -= 1;
    dynExt(b->str, str, strlen(str));
    dynPush(b->str, 0);
}
void SBResetString(StringBuilder *b) { b->str.size = 0; }
void SBFreeString(StringBuilder b) { dynFree(b.str); }

// Cmd Functions

// Cmd List Functions
u32 AddStr(CmdList *list, char *str) {
    u32 len = strlen(str);
    u32 curr = list->strs.size;

    dynExt(list->strs, str, len);
    dynPush(list->strs, 0);

    return curr;
}

void AddCmd(CmdList *list, Cmd cmd) { dynPush(list->cmds, cmd); }

void PushCmdArg(CmdList *list, Cmd *c, char *str) {
    u32 id = AddStr(list, str);
    dynPush(c->args, id);
}

void ExecuteCmd(CmdList *list, Cmd cmd, u32 num_jobs) {

    static int num_jobs_outstanding = 0;

    // fence
    if (cmd.fence) {
        int stat;
        while (waitpid(-1, &stat, 0) != -1);
        num_jobs_outstanding = 0;
        return;
    }

    // limit concurrent jobs
    if (num_jobs_outstanding >= num_jobs) {
        int stat;
        while (waitpid(-1, &stat, 0) > 0);
        num_jobs_outstanding--;
    }

    list->curr_cmd.size = 0;
    for (u32 i = 0; i < cmd.args.size; i++) {
        u32 id = cmd.args.data[i];
        dynPush(list->curr_cmd, &list->strs.data[id]);
    }
    dynPush(list->curr_cmd, NULL);
    num_jobs_outstanding++;

    if (fork() != 0)
        return;
    execvp(list->curr_cmd.data[0], list->curr_cmd.data);
}

void ExecuteCmdList(CmdList *list, u32 max_processes) {
    for (u32 i = 0; i < list->cmds.size; i++) {
        ExecuteCmd(list, list->cmds.data[i], max_processes);
    }
    ExecuteCmd(list, (Cmd){.fence = 1}, max_processes);
}

void FreeCmdList(CmdList list) {
    dynFree(list.cmds);
    dynFree(list.strs);
    dynFree(list.curr_cmd);
}

// exec
void SetName(CmdList *list, Exec *e, char *name) {
    e->exe_name = AddStr(list, name);
}

void AddSource(CmdList *list, Exec *e, char *filename) {
    u32 i = AddStr(list, filename);
    dynPush(e->sources, i);
}

void AddFlag(CmdList *list, Exec *e, char *flag) {
    u32 i = AddStr(list, flag);
    dynPush(e->flags, i);
}

void PushExec(CmdList *list, Exec e) {
    //generate cmd

    StringBuilder b = {};
    Cmd comp = {};
    PushCmdArg(list, &comp, "cc");

    //push sources
    for (u32 i = 0; i < e.sources.size; i++) {
        dynPush(comp.args, e.sources.data[i]);
    }

    //flags
    for (u32 i = 0; i < e.flags.size; i++) {
        SBResetString(&b);
        SBPushChar(&b, '-'); //flag char
        SBPushStr(&b, &list->strs.data[e.flags.data[i]]);
        PushCmdArg(list, &comp, b.str.data);
    }

    //output
    PushCmdArg(list, &comp, "-o");
    PushCmdArg(list, &comp, &list->strs.data[e.exe_name]);
    AddCmd(list, comp);
}

#endif
#endif
