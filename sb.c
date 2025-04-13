#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <winerror.h>

#ifndef SB_IMPL
#include "sb.h"
#endif


static uint32_t pendingCommands = 0; 
static char* compiler = NULL; 

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <windows.h>
#include <processthreadsapi.h>
#include <minwinbase.h>


SB_PHANDLE sb_cmd_async(sb_cmd* c) {
    pendingCommands++;
    char* args[256]; 
    memset(args, 0, sizeof(char*) * (c->asize + 1));

    char* at = c->textbuffer;
    for (int i = 0; i < c->asize; i++) {
        args[i] = at;
        while (at[0] != 0) at++; 
        at[0] = ' ';
        at++;
    }
    at--;
    at[0] = 0;

    //output command
    printf("%s", args[0]);
    for (int i = 1; i < c->asize; i++) {
        printf(" %s", args[i]);
    }
    printf("\n");
    
    STARTUPINFO startInfo;
    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    startInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pinfo;
    ZeroMemory(&pinfo, sizeof(PROCESS_INFORMATION));

    LPSTR s;

    BOOL success = CreateProcessA(
            NULL,
            c->textbuffer,
            NULL,
            NULL,
            TRUE,
            0,
            NULL,
            NULL,
            &startInfo,
            &pinfo
            );

    if (!success) {
        printf("failed\n");
        return 0;
    }
    CloseHandle(pinfo.hThread);
    return pinfo.hProcess;
}

int sb_cmd_fence(SB_PHANDLE h) {
    if (!h) return 0;
    DWORD result = WaitForSingleObject(h, INFINITE);
    return result;
}

int sb_cmd_sync(sb_cmd* c) {
    SB_PHANDLE h = sb_cmd_async(c);
    return sb_cmd_fence(h);
}

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

SB_PHANDLE sb_cmd_async_and_reset(sb_cmd* c) {
    SB_PHANDLE h = sb_cmd_async(c);
    sb_cmd_clear_args(c);
    return h;
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


