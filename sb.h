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
  struct {                                                                     \
    u64 size;                                                                  \
    u64 cap;                                                                   \
    type *data;                                                                \
  }

#define dynPush(array, val)                                                    \
  ((array.size + 1 > array.cap)                                                \
       ? (array.data = realloc(array.data, (array.cap ? array.cap * 2 : 4) *   \
                                               sizeof(array.data[0])),         \
          array.cap = array.cap ? array.cap * 2 : 4)                           \
       : 0,                                                                    \
   array.data[array.size++] = val)

#define dynReserve(array, size)                                                \
  ((array.cap < size)                                                          \
       ? (array.data = realloc(array.data, size * sizeof(array.data[0])),      \
          array.cap = size)                                                    \
       : 0)

// INFO(ELI): This macro can't be included in expressions and must be
// on its own line. This is due to the loops which I could fix if
// I added more global functions but I decided I preferred to not
// do that.
#define dynResize(array, newsize)                                              \
  do {                                                                         \
    if (array.cap < newsize) {                                                 \
      u64 oldsize = array.cap;                                                 \
      while (array.cap < newsize) {                                            \
        array.cap = array.cap ? array.cap * 2 : 4;                             \
      }                                                                        \
      array.data = realloc(array.data, array.cap * sizeof(array.data[0]));     \
    }                                                                          \
                                                                               \
    if (array.size < newsize) {                                                \
      char *d = (char *)&array.data[array.size];                               \
      u32 len = (newsize - array.size) * sizeof(array.data[0]);                \
      for (u32 i = 0; i < len; i++)                                            \
        d[i] = 0;                                                              \
    }                                                                          \
    array.size = newsize;                                                      \
  } while (0);

#define dynBack(array) (array.data[array.size - 1])

#define dynFree(array)                                                         \
  (free(array.data))

// INFO(ELI): These Macros cannot be part of expressions since they require
// loops.

#define dynIns(array, idx, val)                                                \
  do {                                                                         \
    dynResize(array, array.size + 1);                                          \
    for (i64 i = array.size - 1; i >= idx; i--) {                              \
      array.data[i + 1] = array.data[i];                                       \
    }                                                                          \
    array.data[idx] = val;                                                     \
  } while (0)

#define dynDel(array, idx)                                                     \
  do {                                                                         \
    for (u32 i = idx; i < array.size - 1; i++) {                               \
      array.data[i] = array.data[i + 1];                                       \
    }                                                                          \
    dynResize(array, array.size - 1);                                          \
  } while (0)

#define dynExt(array, vals, num)                                               \
  do {                                                                         \
    dynResize(array, array.size + (num));                                      \
    for (u64 i = array.size - (num); i < array.size; i++) {                    \
      array.data[i] = (vals)[i - array.size + (num)];                          \
    }                                                                          \
  } while (0)


// String Builder
typedef struct StringBuilder {
  char *data;
  u32 size;
  u32 cap;
} StringBuilder;

void SBPushChar(StringBuilder *b, char c);
void SBPushStr(StringBuilder *b, char *str);
void SBResetString(StringBuilder *b);
void SBFreeString(StringBuilder b);

// Cmd List
typedef struct Cmd {
  // pointers into some underlying memory
  char **strs;
  u8 fence; // flag to wait for all previous commands to finish
} Cmd;

// Cmd Helpers
void PushCmdArg(Cmd *c, char *str);

typedef struct CmdList {
  struct {
    char *buffer;
    u32 size;
    u32 cap;
  } str;

  struct {
    Cmd *buffer;
    u32 size;
    u32 cap;
  } cmd;
} CmdList;

// helpers
char *AddStr(CmdList *list, char *str);
void AddCmd(CmdList *list, Cmd cmd);
void ExecuteCmd(CmdList *list, Cmd cmd);
void ExecuteCmdList(CmdList *list, u32 max_processes);
void FreeCmdList(CmdList list);

// Builders

// Executable
typedef struct Exec {
  CmdList *list; // external CMD list

  u32 exe_name;

  struct {
    u32 start;
    u32 num;
  } sources;

  struct {
    u32 start;
    u32 num;
  } flags;
} Exec;

void SetName(Exec *e, char *name);

#ifdef SB_IMPL
#include <stdlib.h>
#include <string.h>

// String builder Functions
void SBPushChar(StringBuilder *b, char c) {
  // The +2 ensures that the last byte is always 0,
  // this allows the string to be used as a null terminated
  // string rather than just as a sized string
  if (b->size + 2 > b->cap) {
    // realloc
    u32 newcap = b->cap ? b->cap * 2 : 8;
    char *newbuf = (char *)realloc(b->data, newcap);
    if (!newbuf)
      return;

    b->data = newbuf;

    memset(&b->data[b->size], 0, newcap - b->cap);
    b->cap = newcap;
  }
  b->data[b->size++] = c;
}

void SBPushStr(StringBuilder *b, char *str) {
  u32 len = strlen(str);
  if (b->size + len + 1 > b->cap) {
    // realloc
    u32 newcap = b->cap;

    while (b->size + len + 1 > newcap)
      newcap = newcap ? newcap * 2 : 8;

    char *newbuf = (char *)realloc(b->data, newcap);
    if (!newbuf)
      return;

    b->data = newbuf;

    memset(&b->data[b->size], 0, newcap - b->cap);
    b->cap = newcap;
  }

  strcpy(&b->data[b->size], str);
  b->size += len;
}

void SBResetString(StringBuilder *b) { b->size = 0; }

void SBFreeString(StringBuilder b) {
  if (b.data)
    free(b.data);
}

// Cmd List Functions
char *AddStr(CmdList *list, char *str) {
  u32 len = strlen(str);
  if (list->str.size + len + 1 > list->str.cap) {
    u32 newcap = list->str.cap;
    while (list->str.size + len + 1 > newcap)
      newcap = newcap ? newcap * 2 : 8;

    char *newbuf = (char *)realloc(list->str.buffer, newcap);
    if (!newbuf)
      return NULL;

    list->str.buffer = newbuf;
    list->str.cap = newcap;
  }
  u32 start = list->str.size;

  strcpy(&list->str.buffer[list->str.size], str);
  list->str.size += len + 1;
  return &list->str.buffer[start];
}

void AddCmd(CmdList *list, Cmd cmd) {
  if (list->cmd.size + 1 > list->cmd.cap) {
    u32 newcap = list->cmd.cap ? list->cmd.cap * 2 : 8;
    Cmd *newbuf = (Cmd *)realloc(list->cmd.buffer, newcap * sizeof(Cmd));
    if (!newbuf)
      return;

    list->cmd.buffer = newbuf;
    list->cmd.cap = newcap;
  }

  list->cmd.buffer[list->cmd.size++] = cmd;
}

void ExecuteCmdList(CmdList *list, u32 max_processes);
void FreeCmdList(CmdList list);

#endif
#endif
