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
static char* compiler = NULL; 

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <windows.h>
#include <processthreadsapi.h>
#include <timezoneapi.h>
#include <minwinbase.h>
#include <fileapi.h>
#include <errhandlingapi.h>
#include <winerror.h>
#include <winnt.h>
#include <minwindef.h>
#include <handleapi.h>
#include <winbase.h>


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
    printf("async: %s\n", c->textbuffer);
    
    STARTUPINFO startInfo;
    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(STARTUPINFO);
    startInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    startInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pinfo;
    ZeroMemory(&pinfo, sizeof(PROCESS_INFORMATION));

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
        printf("failed: %ld\n", GetLastError());
        return 0;
    }
    CloseHandle(pinfo.hThread);
    return pinfo.hProcess;
}

int sb_cmd_wait(SB_PHANDLE h) {
    if (!h) return 0;
    DWORD result = WaitForSingleObject(h, INFINITE);
    return result;
}

int sb_cmd_sync(sb_cmd* c) {
    SB_PHANDLE h = sb_cmd_async(c);
    return sb_cmd_wait(h);
}

int sb_should_rebuild(const char* srcpath, const char* binpath) {
    HANDLE src_fd = CreateFile(srcpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE dst_fd = CreateFile(binpath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (src_fd == INVALID_HANDLE_VALUE) {
        printf("Failed to Find Source\n");
        return 0;
    }

    if (dst_fd == INVALID_HANDLE_VALUE) {
        printf("Failed to Find Binary\n");
        //default to rebuild if missing binary
        return 1;
    }

    //src time
    ULARGE_INTEGER src_time;
    int result = GetFileTime(src_fd, NULL, NULL, (LPFILETIME)&src_time);
    if (!result) {
        printf("failed to get source time\n");
        return 0;
    }

    //bin time
    ULARGE_INTEGER bin_time;
    result = GetFileTime(dst_fd, NULL, NULL, (LPFILETIME)&bin_time);
    if (!result) {
        printf("failed to get bin time\n");
        //default to rebuild if missing
        return 1;
    }
    CloseHandle(src_fd);
    CloseHandle(dst_fd);

    return bin_time.QuadPart < src_time.QuadPart;
}

void sb_rebuild_self(int argc, char* argv[], const char* srcpath) {
    int rebuild = sb_should_rebuild(srcpath, argv[0]);
    //up to date!!
    if (!rebuild) {
        printf("no rebuild\n");
        return;
    }

    //rename self 
    char newBin[MAX_PATH] = {0};
    snprintf(newBin, sizeof(newBin), "%s.old", argv[0]);
    if (!MoveFileEx(argv[0], newBin, MOVEFILE_REPLACE_EXISTING)) {
        printf("Error: %ld\n", GetLastError());
    }

    sb_cmd* c = &(sb_cmd){0};
    sb_pick_compiler();
    sb_cmd_push(c, compiler, srcpath);
    sb_cmd_sync_and_reset(c);

    //delete obj file
    char temp[MAX_PATH] = {0};
    int num = snprintf(temp, sizeof(temp), "%s", argv[0]);
    while (temp[num] != '.') num--;
    snprintf(&temp[num], sizeof(temp) - num, ".obj");
    printf("%s\n", temp);
    BOOL result = DeleteFile(temp);
    if (!result) {
        printf("Failed to delete obj: %ld\n", GetLastError());
    }
    
    //delete .old
    //doesn't work, maybe later?
    //result = MoveFileEx(newBin, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    //if (!result) {
    //    printf("Failed to delete old: %ld\n", GetLastError());
    //}

    //start new version
    sb_cmd_push(c, argv[0]);
    sb_cmd_sync_and_reset(c);
    exit(0);

    return;
}

#else
#include <sys/types.h>
#include <sys/wait.h>
#include "unistd.h"
#include "fcntl.h"
#include "sys/stat.h"

SB_FHANDLE sb_fopen(char* file, char* mode) {
    return fopen(file, mode);
}

void sb_fclose(SB_FHANDLE h) { 
    fclose(h);
}

//set up for now, do more research later,
//maybe set up automatical console allocation
//later?
void sb_fprintf(SB_FHANDLE h, char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(h, format, args);
    va_end(args);
}

// Super platform specific
// definitely needs platform
// stuff
// Non-recursive
void sb_mkdir(char* path) {
    //full public
    mkdir(path, S_IRWXU);
}

// Also platform specific, must implement
void sb_rename(char* filepath, char* name) {
    rename(filepath, name);  
}

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

SB_PHANDLE sb_cmd_async(sb_cmd* c) {
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
    return cid;
}

#endif

void sb_pick_compiler() {
//TODO(ELI): Add clang and gcc versions for windows
//TODO(ELI): Add more detection for linux
#ifdef _WIN32
    #ifdef _MSC_VER
        compiler = "cl.exe";
    #endif
#else
        compiler = "cc";
#endif
}

void sb_set_compiler(char* str) {
    compiler = str;
}

char* sb_compiler() {
    return compiler;
}

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
