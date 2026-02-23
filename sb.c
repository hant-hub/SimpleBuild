#define SB_IMPL
#include "sb.h"
#include <stdio.h>


int main(int argc, char* argv[]) {

    StringBuilder b = {}; 
    SBPushChar(&b, 'a');
    SBPushChar(&b, 'b');
    SBPushChar(&b, 'c');

    SBPushStr(&b, " This is a test string");

    printf("b: %s\n", b.data);
    printf("size: %d cap: %d\n", b.size, b.cap);

    SBFreeString(b);

    CmdList cmds = {};

    Cmd new_cmd = {};
    //new_cmd.prog = AddStr(&cmds, "neofetch");
    //AddCmd(&cmds, new_cmd);

} 
