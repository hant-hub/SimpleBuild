#define SB_IMPL
#include "sb.h"
#include <stdio.h>


int main(int argc, char* argv[]) {

    RebuildSelf(argc, argv);
    printf("next\n");

    SetCompiler("clang");

    StringBuilder b = {}; 
    SBPushChar(&b, 'a');
    SBPushChar(&b, 'b');
    SBPushChar(&b, 'c');

    SBPushStr(&b, " This is a test string");

    printf("b: %s\n", b.str.data);
    printf("size: %ld cap: %ld\n", b.str.size, b.str.cap);

    SBFreeString(b);

    CmdList cmds = {
        .cmds = {0},
        .curr_cmd = {0},
        .strs = {0},
    };

    Cmd new_cmd = {
        .args = {0},
        .fence = 0
    };

    Cmd print_cmd = {0};
    PushCmdArg(&cmds, &new_cmd, "sleep");
    PushCmdArg(&cmds, &new_cmd, "0.5");

    PushCmdArg(&cmds, &print_cmd, "echo");
    PushCmdArg(&cmds, &print_cmd, "1");

    AddCmd(&cmds, new_cmd);
    AddCmd(&cmds, new_cmd);
    //AddCmd(&cmds, (Cmd){.fence = 1});
    AddCmd(&cmds, print_cmd);
    AddCmd(&cmds, print_cmd);

    ExecuteCmdList(&cmds, 4);
    FreeCmdList(cmds);

    cmds = (CmdList){0};

    Exec e = {0};
    SetName(&cmds, &e, "build");
    AddSource(&cmds, &e, "sb.c");
    AddFlag(&cmds, &e, "g");
    PushExec(&cmds, e);

    ExecuteCmdList(&cmds, 1); 

    dynFree(new_cmd.args);
    FreeCmdList(cmds);

    MakeDirectory("tmp/thing");
    u64 t1 = GetFileTime("sb");
    u64 t2 = GetFileTime("sb.c");

    printf("FileTimes: %ld %ld\n", t1, t2);
    printf("Up To Date: %d\n", t1 > t2);

    SBResetString(&b);
    ExtractBaseName("a/b/c.c", &b);
    printf("Basename: %s\n", b.str.data);
    SBResetString(&b);
    PopDirLevel("a/b/c.c", &b);
    printf("Path: %s\n", b.str.data);
    PopDirLevel(b.str.data, &b);
    printf("Path: %s\n", b.str.data);
    SBResetString(&b);
    GetExtension("a/b/c", &b);
    printf("Ext: %s\n", b.str.data);

    u32 i = GetPathDepth("a/b/c");
    printf("depth: %d\n", i);

    //iterate over directory
    //DirBegin(".");
    //while (1) {
    //    DirType t;
    //    char* file = DirNext(&t);
    //    if (t == T_END) break;

    //    printf("File: %s ", file);
    //    switch(t) {
    //        case T_FILE: printf("file\n"); break;
    //        case T_DIR: printf("dir\n"); break;
    //        case T_UNKNOWN: printf("unknown\n"); break;
    //        case T_END: printf("end\n"); break;
    //    }
    //}

    printf("--------Recursive---------\n");
    //iterate over directory
    DirBeginRec(".", 3);
    while (1) {
        DirType t;
        char* file = DirNextRec(&t);
        if (t == T_END) break;
        if (t != T_DIR) continue;

        printf("File: %s ", file);
        switch(t) {
            case T_FILE: printf("file\n"); break;
            case T_DIR: printf("dir\n"); break;
            case T_UNKNOWN: printf("unknown\n"); break;
            case T_END: printf("end\n"); break;
        }
    }
} 
