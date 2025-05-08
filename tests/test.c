#include <stdio.h>
#define SB_IMPL
#include "../sb.h"


int main(int argc, char* argv[]) {


    sb_BUILD(argc, argv) {
        sb_target_dir("build");
        sb_EXEC() {
            sb_set_out("app");

            sb_add_file("tests/testsrc.c");
            sb_add_header("sb.h");

            sb_add_flag("g");

            //sb_set_incremental();
        }
        //sb_fence();
        //sb_CMD() {
        //    sb_cmd_main("./build/app");
        //}
    }
    //printf("%s\n", sb_basename("test"));
}
