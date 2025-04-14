#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>

#ifndef SB_IMPL
#include "sb.h"
#endif
static uint32_t pendingCommands = 0; 

#if defined(WIN32) || defined(__WIN32__)
#include <windows.h>


#else
#include <sys/types.h>
#include <sys/wait.h>
#include "unistd.h"
#include "fcntl.h"
#include "sys/stat.h"

int sb_should_rebuild(const char* srcpath, const char* binpath) {
    struct stat srcstat;
    struct stat binstat;

    if (stat(srcpath, &srcstat)) {
        printf("Can't Find Source\n");
        exit(1);
    }
    if (stat(binpath, &binstat)) {
        printf("No Preexisting Binary\n");
        binstat.st_mtim.tv_sec = INT_MAX;
    }

    return srcstat.st_mtim.tv_sec > binstat.st_mtim.tv_sec;
}


void sb_rebuild_self(int argc, char* argv[], const char* srcpath) {
    int should = sb_should_rebuild(srcpath, argv[0]);
    if (!should) return; //run current script
                         
    sb_cmd* c = &(sb_cmd){0};
    sb_cmd_push(c, "cc", srcpath, "-o", argv[0]);

    if (sb_cmd_sync(c)) {
        exit(1); //error out
    }

    sb_cmd_clear_args(c);
    sb_cmd_free(c);
    execvp(argv[0], argv); //switch to new build
}

int sb_cmd_sync(sb_cmd* c) {
    //build args list
    char* args[256]; 
    memset(args, 0, sizeof(char*) * (c->asize + 1));

    char* at = c->textbuffer;
    for (int i = 0; i < c->asize; i++) {
        args[i] = at;
        while (at[0] != 0) at++; 
        at++;
    }

    //output command
    printf("%s", args[0]);
    for (int i = 1; i < c->asize; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");

    pid_t cid = fork();
    if (cid == 0) {
        execvp(args[0], args);
        exit(1);
    }

    int status;
    pid_t out = waitpid(cid, &status, 0); //idk options should be fine for now
    
    return out == -1;
}

int sb_cmd_fence() {
    int status;
    for (int i = 0; i < pendingCommands; i++) {
        waitpid(-1, &status, WUNTRACED);
        if (WEXITSTATUS(status)) return status;
    }
    pendingCommands = 0;

    return status;
}

void sb_cmd_async(sb_cmd* c) {
    pendingCommands++;
    char* args[256]; 
    memset(args, 0, sizeof(char*) * (c->asize + 1));

    char* at = c->textbuffer;
    for (int i = 0; i < c->asize; i++) {
        args[i] = at;
        while (at[0] != 0) at++; 
        at++;
    }

    //output command
    printf("%s", args[0]);
    for (int i = 1; i < c->asize; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");

    pid_t cid = fork();
    if (cid == 0) {
        execvp(args[0], args);
        exit(1);
    }
}

#endif

int sb_cmd_sync_and_reset(sb_cmd* c) {
    int status = sb_cmd_sync(c);
    sb_cmd_clear_args(c);
    return status;
}

void sb_cmd_async_and_reset(sb_cmd* c) {
    sb_cmd_async(c);
    sb_cmd_clear_args(c);
}

void sb_cmd_push_args(sb_cmd* c, uint32_t num, ...) {
    //grow textbuffer
    //printf("%d\n", num);
    va_list args;
    va_start(args, num);

    for (int i = 0; i < num; i++) { 
        const char* str = va_arg(args, const char*);
        uint32_t len = strlen(str) + 1;
        if (c->tsize + len > c->tcap) {
            uint32_t newsize = c->tcap ? c->tcap : SB_MIN_TEXT_SIZE;
            while (newsize < c->tsize + len) newsize *= 2;

            c->textbuffer = (char*)realloc(c->textbuffer, newsize);
            c->tcap = newsize;
        }
        //push string
        char* handle = &c->textbuffer[c->tsize];
        memcpy(handle, str, len);
        c->tsize += len;
        c->asize++;
    }

    va_end(args);
}

void sb_cmd_clear_args(sb_cmd* c) {
    c->asize = 0;
    c->tsize = 0;
}

void sb_cmd_free(sb_cmd* c) {
    free(c->textbuffer);
    c->textbuffer = 0;
}

void sb_cmd_push_chars(sb_cmd* c, uint32_t len, const char* str) {
    printf("len: %d\n", len);
    if (c->tsize + len > c->tcap) {
        uint32_t newsize = c->tcap ? c->tcap : SB_MIN_TEXT_SIZE;
        while (newsize < c->tsize + len) newsize *= 2;

        c->textbuffer = (char*)realloc(c->textbuffer, newsize);
        c->tcap = newsize;
    }
    //push string
    char* handle = &c->textbuffer[c->tsize];
    memcpy(handle, str, len);
    c->tsize += len;
}


void sb_cmd_pushf(sb_cmd* c, const char* __restrict format, ...) {
    va_list args;
    va_start(args, format);
    
    uint32_t p = 0; 
    while (format[p]) {
        if (format[p] != '%') {
            sb_cmd_push_chars(c, 1, (char[]){format[p]});
            p++;
            continue;
        }
        p++;

        switch (format[p]) {
            case 'x':
                {
                    p++;
                    uint32_t d = va_arg(args, uint32_t);
                    int numdigits = 0;
                    uint32_t test = d;
                    while (test) {
                        test /= 16;
                        numdigits += 1;
                    }
                    for (int i = numdigits - 1; i >= 0; i--) { 
                        uint32_t digit = d;
                        for (int j = 0; j < i; j++) {
                            digit /= 16;
                        }
                        digit %= 16;
                        if (digit <= 9) { 
                            sb_cmd_push_chars(c, 1, (char[]){digit + '0'});
                        } else {
                            digit -= 10;
                            sb_cmd_push_chars(c, 1, (char[]){digit + 'a'});
                        }
                    }
                    break;
                }
            case 'c':
                {
                    p++;
                    char ch = va_arg(args, int);
                    sb_cmd_push_chars(c, 1, &ch);
                    break;
                }
            case 'd':
                {
                    p++;
                    int d = va_arg(args, int);
                    if (d < 0) {
                        sb_cmd_push_chars(c, 1, "-");
                        d *= -1;
                    }

                    int numdigits = 0;
                    int test = d;
                    while (test) {
                        test /= 10;
                        numdigits += 1;
                    }
                    for (int i = numdigits - 1; i >= 0; i--) { 
                        int digit = d;
                        for (int j = 0; j < i; j++) {
                            digit /= 10;
                        }
                        digit %= 10;
                        sb_cmd_push_chars(c, 1, (char[]){digit + '0'});
                    }
                    break;
                }
            case 's':
                {
                    p++;
                    char* string = va_arg(args, char*);
                    sb_cmd_push_chars(c, strlen(string), string);
                    break;
                }
            default:
                {
                    sb_cmd_push_chars(c, 2, (char[]){format[p-1], format[p]});
                    p++;
                }
        }
    }

    sb_cmd_push_chars(c, 1, &(char){0});
    c->asize++;
    va_end(args);
}
