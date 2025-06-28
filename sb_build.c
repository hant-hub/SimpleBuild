#include <stdio.h>
#define SB_IMPL
#include "sb.h"


int main(int argc, char* argv[]) {

    sb_BUILD(argc, argv) {
        sb_EXEC() {
            sb_add_file("test.c");

            sb_link_library("vulkan");

            sb_set_out("testapp");
            sb_set_optmize(0);
            sb_export_command();
            sb_set_incremental();
        }
    }
    return 0;
}
