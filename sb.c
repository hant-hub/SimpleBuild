#define SB_IMPL
#include "sb.h"
#include <stdio.h>


int main(int argc, char* argv[]) {

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
    PushCmdArg(&cmds, &new_cmd, "1");

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

} 
