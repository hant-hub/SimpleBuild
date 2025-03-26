#include <stdio.h>
#define SB_IMPL
#include "sb.h"





int main(int argc, char* argv[]) {
    AUTO_REBUILD(argc, argv);

    sb_cmd* c = &(sb_cmd){0};
    sb_cmd_push(c, "echo", "test1");
    sb_cmd_async(c);

    sb_cmd_clear_args(c);
    sb_cmd_push(c, "echo", "test2");
    sb_cmd_async(c);

    if (sb_cmd_fence(2)) {
        exit(1);
    }

    sb_cmd_clear_args(c);
    sb_cmd_push(c, "echo", "test3");
    sb_cmd_async(c);

    if (sb_cmd_fence(1)) {
        exit(1);
    }


    return 0;
}



