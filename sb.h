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
    u8 gen_compile_commands;

    u32 exe_name;
    dynArray(u32) sources;
    dynArray(u32) flags;
} Exec;

void SetName(CmdList *list, Exec *e, char *name);
void AddSource(CmdList *list, Exec *e, char *filename);
void AddFlag(CmdList *list, Exec *e, char *flag);
void AddDynamicLib(CmdList *list, Exec* e, char* libname);
void IncludeDir(CmdList *list, Exec* e, char* dirname);
void GenCompileCommands(Exec* e);
void PushExec(CmdList *list, Exec e); // frees Exec
void SetCompiler(char* comp);


//File Helpers

void ExeRelative();
u32 GetPathDepth(char* filepath);
void GetExtension(char* filepath, StringBuilder* b);
void PopDirLevel(char* filepath, StringBuilder* b);
void ExtractBaseName(char* filepath, StringBuilder* b);
void MakeDirectory(char* filepath);
u64 GetFileTime(char* filepath);
void _RebuildSelf(char* sourcename, u32 argc, char** argv);
#define RebuildSelf(argc, argv) _RebuildSelf(__FILE__, argc, argv)

//Directory Iterators
typedef enum DirType {
    T_FILE,
    T_DIR,
    T_END,
    T_UNKNOWN,
} DirType;

void DirBegin(char* filepath);
char* DirNext(DirType* t);

void DirBeginRec(char* filepath, u32 max_depth);
char* DirNextRec(DirType* t);



#ifdef SB_IMPL
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>

// String builder Functions
void SBPushChar(StringBuilder *b, char c) {
    if (b->str.size)
        b->str.size--;
    dynPush(b->str, c);
    dynPush(b->str, 0);
}
void SBPushStr(StringBuilder *b, char *str) {
    if (b->str.size)
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

#ifdef SB_DEBUG
    printf("Cmd: ");
    for (u32 i = 0; i < list->curr_cmd.size; i++) {
        printf("%s ", list->curr_cmd.data[i]);
    }
    printf("\n");
#endif

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

static char* compiler = "cc";

void SetCompiler(char* comp) {
#ifdef SB_DEBUG
    printf("New Compiler: %s\n", comp);
#endif
    compiler = comp;
}

void AddDynamicLib(CmdList *list, Exec* e, char* libname) {
    StringBuilder b = {};
    SBPushStr(&b, "-L");
    SBPushStr(&b, libname);
    AddFlag(list, e, b.str.data);
    SBFreeString(b);
}

void IncludeDir(CmdList *list, Exec* e, char* dirname) {
    StringBuilder b = {};
    SBPushStr(&b, "-I");
    SBPushStr(&b, dirname);
    AddFlag(list, e, b.str.data);
    SBFreeString(b);
}

static FILE* base = NULL;
static u32 num_entries = 0;
void cleanupBase() {
    if (base) {
        fputc('\n', base);
        fputc(']', base);
        fputc('\n', base);
        fclose(base);
    }
}

void GenCompileCommands(Exec* e) {
    e->gen_compile_commands = 1;
    if (base == NULL) {
        base = fopen("./compile_commands.json", "w+");
        fputc('[', base);
        fputc('\n', base);
        atexit(cleanupBase);
    }
}

void PushExec(CmdList *list, Exec e) {
    //generate cmd
    StringBuilder b = {};
    Cmd comp = {};
    PushCmdArg(list, &comp, compiler);


    //push sources
    for (u32 i = 0; i < e.sources.size; i++) {
        dynPush(comp.args, e.sources.data[i]);
    }

    StringBuilder entry = {};
    if (e.gen_compile_commands) {
        SBPushStr(&entry, "{ \"directory\": \"");

        char path[PATH_MAX + 1] = {0};
        getcwd(path, PATH_MAX);
        SBPushStr(&entry, path);

        SBPushStr(&entry, "\", ");
        SBPushStr(&entry, "\"arguments\": [");
    }

    //flags
    for (u32 i = 0; i < e.flags.size; i++) {
        SBResetString(&b);
        SBPushChar(&b, '-'); //flag char
        SBPushStr(&b, &list->strs.data[e.flags.data[i]]);
        PushCmdArg(list, &comp, b.str.data);

        if (e.gen_compile_commands) {
            char* flag = b.str.data;
            //printf("flag: %s\n", flag);
            SBPushChar(&entry, '\"');
            SBPushStr(&entry, flag);
            SBPushChar(&entry, '\"');

            if (i + 1 < e.flags.size) {
                SBPushStr(&entry, ", ");
            }
        }
    }

    if (e.gen_compile_commands) {
        SBPushStr(&entry, "], \"file\":");
        for (u32 i = 0; i < e.sources.size; i++) {
            char* source = &list->strs.data[e.sources.data[i]];
            if (num_entries) fprintf(base, ",\n");
            fprintf(base, "\t%s \"%s\" }", entry.str.data, source);
            num_entries++;
        }
        SBFreeString(entry);
    }


    //output
    PushCmdArg(list, &comp, "-o");
    dynPush(comp.args, e.exe_name);
    AddCmd(list, comp);
}

//File Helpers

void ExeRelative() {
    char filepath[PATH_MAX + 1] = {0};
    if (!readlink("/proc/self/exe", filepath, PATH_MAX)) {
#ifdef SB_DEBUG
        printf("Cannot find EXE path\n");
#endif
        return;
    }

    StringBuilder b = {};

    PopDirLevel(filepath, &b);
#ifdef SB_DEBUG
    printf("Exe At: %s\n", b.str.data);
#endif
    chdir(b.str.data);

    SBFreeString(b);
}

u32 GetPathDepth(char* filepath) {
    u32 i = 0;
    if (filepath[i] == '/') i++;
    if (filepath[i] == '.' && filepath[i + 1] == '/') i += 2;

    u32 depth = 1;
    while (filepath[i]) {
        if (filepath[i] == '/') depth++;
        i++;
    }


    return depth;
}

void GetExtension(char* filepath, StringBuilder* b) {
    u32 len = strlen(filepath);
    u32 start = len;

    while (start && filepath[start] != '.') start--;

    SBResetString(b);
    if (!start) {
        dynPush(b->str, 0);
        return;
    }
    SBPushStr(b, &filepath[start]);
}

void PopDirLevel(char* filepath, StringBuilder* b) {
    u32 len = strlen(filepath);

    while (len && filepath[len] != '/') len--;

    SBResetString(b);
    if (!len) {
        dynPush(b->str, 0);
        return;
    }
    for (u32 i = 0; i < len; i++) {
        SBPushChar(b, filepath[i]);
    }
}

void ExtractBaseName(char* filepath, StringBuilder* b) {
    u32 len = strlen(filepath);

    while (len && filepath[len - 1] != '/') {
        len--;
    }

    SBResetString(b);
    if (!len) {
        dynPush(b->str, 0);
        return;
    }
    SBPushStr(b, &filepath[len]);
}

void MakeDirectory(char* filepath) {
    StringBuilder b = {0};
    SBPushStr(&b, filepath);

    while (b.str.size) {
        if (dynBack(b.str) == '/') {
            dynBack(b.str) = 0;
            MakeDirectory(b.str.data);
        }
        dynBack(b.str) = 0;
        b.str.size--;
    }
    mkdir(filepath, 0777);
    SBFreeString(b);
}

u64 GetFileTime(char* filepath) {

    struct stat buf;
    if (stat(filepath, &buf)) {
        return -1;
    }
    u64 time = buf.st_mtim.tv_sec * (1000 * 1000 * 1000);
    time += buf.st_mtim.tv_nsec;
    
    return time;
}

void _RebuildSelf(char* sourcename, u32 argc, char** argv) {
    
    u64 t1 = GetFileTime("/proc/self/exe");
    u64 t2 = GetFileTime(sourcename);


    //binary matches
    if (t1 > t2) return;

#ifdef SB_DEBUG
    printf("Rebuild Required\n");
#endif

    CmdList cmds = {};

    StringBuilder b = {};
    SBPushStr(&b, argv[0]);
    SBPushStr(&b, ".old");
    rename(argv[0], b.str.data);

    Exec self = {};
    SetName(&cmds, &self, argv[0]);
    AddSource(&cmds, &self, sourcename);
    PushExec(&cmds, self);
    ExecuteCmdList(&cmds, 1);

    execvp(argv[0], argv);

}

//Directory Iterators
#include <dirent.h>
#include <fcntl.h>

static DIR* dirpn;

void DirBegin(char* filepath) {
    dirpn = opendir(filepath);
}

char* DirNext(DirType* t) {
    struct dirent* d = NULL;
    d = readdir(dirpn);

    *t = T_END;
    if (!d) { 
        closedir(dirpn);
        return NULL; 
    }

    if (d->d_type == DT_DIR) *t = T_DIR;
    else if (d->d_type == DT_REG) *t = T_FILE;
    else *t = T_UNKNOWN;

    return d->d_name;
}

static dynArray(StringBuilder) paths = {};
static StringBuilder curr_path = {};
static StringBuilder out_path = {};
static DIR* curr = NULL;
static u32 max_depth = 0;

void DirBeginRec(char* filepath, u32 depth) {
    curr = opendir(filepath);
    SBResetString(&curr_path);
    SBPushStr(&curr_path, filepath);
    max_depth = depth;
}

char* DirNextRec(DirType* t) {
    struct dirent* d = NULL;

    while (curr || paths.size) {
        d = readdir(curr);

        if (!d) { 
            closedir(curr);
            SBFreeString(curr_path);
            curr = NULL;
            if (!paths.size) break;
            curr_path = dynBack(paths);
            curr = opendir(curr_path.str.data);
            paths.size--;
            continue;
        }

        if (strcmp(d->d_name, ".") == 0) continue;
        if (strcmp(d->d_name, "..") == 0) continue;

        switch (d->d_type) {
            case DT_DIR: 
                *t = T_DIR;
                StringBuilder new = {};
                if (curr_path.str.data) {
                    SBPushStr(&new, curr_path.str.data);
                    SBPushChar(&new, '/');
                }
                SBPushStr(&new, d->d_name);
                if (max_depth <= GetPathDepth(new.str.data)) {
                    SBFreeString(new);
                    break;
                }
                dynPush(paths, new);
            break;
            case DT_REG: *t = T_FILE; break;
            default: *t = T_UNKNOWN; break;
        }
        SBResetString(&out_path);
        SBPushStr(&out_path, curr_path.str.data);
        SBPushChar(&out_path, '/');
        SBPushStr(&out_path, d->d_name);

        return out_path.str.data;
    }

    *t = T_END;
    return NULL;
}

#endif
#endif
