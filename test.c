#include <limits.h>
#include <time.h>
#include <stdio.h>
#define SB_IMPL
#include "sb.h"
#include "fcntl.h"
#include "sys/stat.h"




int GetFileTime(char* filepath) {
    struct stat statbuf = {0};

    if (stat(filepath, &statbuf)) {
        printf("failed\n");
        return INT_MAX;
    }

    printf("%s\n", ctime(&statbuf.st_mtim.tv_sec));
    return statbuf.st_mtim.tv_sec;
}


int ShouldRebuild(char* srcpath, char* binpath) {
    int srctime = GetFileTime(srcpath);
    int bintime = GetFileTime(binpath);
    return srctime > bintime;
}




int main(int argc, char* argv[]) {
    sb_cmd* c = &(sb_cmd){0};

    int time = GetFileTime(__FILE__);
    printf("Time Modified: %d\n", time);

    printf("Should Rebuild: %d\n", ShouldRebuild(__FILE__, argv[0]));

    sb_cmd_push_arg(c, "cc");
    sb_cmd_push_arg(c, "src/test.c");
    sb_cmd_push_arg(c, "-o");
    sb_cmd_push_arg(c, "build/app");

    if (sb_cmd_sync(c)) {
        exit_(1);
    }

    sb_cmd_free(c);
    return 0;
}



