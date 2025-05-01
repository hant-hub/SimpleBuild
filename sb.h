#ifndef SB_BUILD_H
#define SB_BUILD_H

#include <linux/limits.h>
#include <stdint.h>

/*
 * The main idea is that sb_cmd
 * will differentiate each
 * command. On Linux each command
 * will be seperated by Null terminators
 * to allow for easy construction of the
 * arguement vector
 *
 * On Windows each argument is seperated by 
 * spaces, and each command by a null
 * terminator allowing for easy usage here.
 *
 */

#ifdef _MSC_VER
typedef void* FHANDLE;
#else
typedef int FHANDLE;
#endif

typedef enum sb_file_mode {
    sbf_READ,
    sbf_WRITE,
    sbf_READWRITE,
} sb_file_mode;

typedef enum sb_file_flags {
    sbf_APPEND,
    sbf_CREATE,
    sbf_TRUNC,
} sb_file_flags;

//file system
FHANDLE sb_open(char* file, sb_file_mode mode, sb_file_flags flags);
void sb_fprintf(FHANDLE f, char* format, ...);
void sb_close(FHANDLE f);

char* sb_get_cwd();

//Memory allocation
void* sb_alloc(uint64_t size);
void* sb_realloc(void* p, uint64_t size);
void sb_free(void* p);

//String Utilities

typedef struct sb_sized_string {
    char* string;
    uint32_t size;
} sb_sized_string;

//Null terminated
uint32_t sb_strlen(const char* s);
uint32_t sb_strcmp(const char* s1, const char* s2);

//Sized
void sb_strcpy(char* dst, const sb_sized_string s);


/*
 * Non specific Build Functions
 */

int sb_build_start();
void sb_build_end();

int sb_cmd_start();
void sb_cmd_end();

void _sb_cmd_main(sb_sized_string cmd);
void _sb_cmd_opt(sb_sized_string opt);
uint32_t _sb_cmd_arg(sb_sized_string arg);

void sb_autobuild(int argc, char* argv[], char* src);

/* forces all previous commands to finish before continuing*/
void sb_fence();


/*
 * Compiler specific functions 
 */

int sb_start_exec();
void sb_stop_exec();

void _sb_add_file(sb_sized_string f);
void _sb_set_out(sb_sized_string f);

void _sb_add_include_path(sb_sized_string f);
void _sb_add_library_path(sb_sized_string f);
void _sb_link_library(sb_sized_string f);

void sb_set_optmize(uint32_t level);
void sb_export_command();

 /* 
 * will run through build logic but won't submit to 
 * execution list. Useful for creating compile
 * commands without actually compiling
 */
void sb_dry_run();



/*
 * Spooky Macros
*/

#define sb_BUILD(argc, argv) \
    for (int i = (sb_build_start(), sb_autobuild(argc, argv, __FILE__), 0); i == 0; (sb_build_end(), i++))

#define sb_CMD() \
    for (int i = sb_cmd_start(); i == 0; (sb_cmd_end(), i++))

#define sb_EXEC() \
    for (int i = sb_start_exec(); i == 0; (sb_stop_exec(), i++))

#define sb_add_include_path(x) \
    _sb_add_include_path((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_add_library_path(x) \
    _sb_add_library_path((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_link_library(x) \
    _sb_link_library((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_add_file(x) \
    _sb_add_file((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_set_out(x) \
    _sb_set_out((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_cmd_main(x) \
    _sb_cmd_main((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_cmd_opt(x) \
    _sb_cmd_opt((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#define sb_cmd_arg(x) \
    _sb_cmd_arg((sb_sized_string){ \
            .string = x,\
            .size = sb_strlen(x),\
    })

#ifdef SB_IMPL
/*
 * Just for now, The plan is to use internal macro switches.
 * This is to see if it makes a more readable cross platform
 * file. That way all the code for a specific function
 * is in a single place
 *
 */
#ifdef _MSC_VER
//Windows
#else
//Everything else (currently just linux)
#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#endif

#include <stdarg.h>

typedef struct sb_cmd {
    int32_t start;
    int32_t length;
} sb_cmd;

typedef struct sb_cmd_list {
    //text buffer
    uint32_t size;
    uint32_t cap;
    char* cmd;

    //indicies
    uint32_t isize;
    uint32_t icap;
    sb_cmd* indicies;
} sb_cmd_list;

FHANDLE sb_open(char* file, sb_file_mode mode, sb_file_flags flags) {
    int o_mode = 0;
    switch (mode) {
        case sbf_READ: 
            o_mode = O_RDONLY;
            break;
        case sbf_WRITE: 
            o_mode = O_WRONLY;
            break;
        case sbf_READWRITE: 
            o_mode = O_RDWR;
            break;
    }

    if (flags & sbf_CREATE) {
        o_mode |= O_CREAT;
    }

    if (flags & sbf_TRUNC) {
        o_mode |= O_TRUNC;
    }

    return open(file, o_mode, 0644);
}

char cwd[PATH_MAX] = {0}; 

char* sb_get_cwd() {
   if (!cwd[0]) {
        getcwd(cwd, PATH_MAX);    
   }
   return cwd;
}

void sb_fprintf(FHANDLE f, char* format, ...) {
    va_list args;
    va_start(args, format);
    vdprintf(f, format, args);
    va_end(args);
}

void sb_close(FHANDLE f) {
    close(f);
}

void sb_strcpy(char* dst, const sb_sized_string s) {
    memcpy(dst, s.string, s.size);
}

uint32_t sb_strlen(const char* s) {
    return strlen(s);
}

uint32_t sb_strcmp(const char* s1, const char* s2) {
    return strcmp(s1, s2);
}

void* sb_alloc(uint64_t size) {
    return malloc(size);
}
void* sb_realloc(void* p, uint64_t size) {
    return realloc(p, size);
}
void sb_free(void* p) {
    return free(p);
}

/* ----------------------------------------------------
 * Global Variables
 *
 * VERY IMPORTANT
 *
*/

//may expand later
typedef struct exe_info {
    int export_commands;
    int dry;

    //for each file, store the index,
    //so that we can create a compile
    //commands entry

    uint32_t fsize;
    uint32_t fcap;
    uint32_t* files;
} exe_info;


sb_cmd_list cmd_list = {0};
sb_cmd curr_cmd = {0};
exe_info curr_exe = {0};

#ifdef _MSC_VER
int compile_cmds = NULL;

char separator = ' ';
char flag = '/';
#else
int compile_cmds = 0;

char separator = 0;
char flag = '-';
#endif


#ifdef _MSC_VER
//msvc
sb_sized_string compiler = (sb_sized_string) {
    .string = "cl.exe",
    .size = 6
};
#elif defined(__GNUC__)
//gcc
sb_sized_string compiler = (sb_sized_string) {
    .string = "cc",
    .size = 2
};
#elif defined(__clang__)
//clang
sb_sized_string compiler = (sb_sized_string) {
    .string = "clang",
    .size = 5
};
#endif

//-----------------------------------------------------

int sb_build_start() {
    //reset top pointers,
    //don't mess with pointers
    cmd_list.size = 0;
    cmd_list.isize = 0;

    curr_exe = (exe_info){0};


    return 0;
}


void sb_build_end() {
    //execute commands
    //in parrallel
    
    for (uint32_t i = 0; i < cmd_list.isize; i++) {
        sb_cmd idx = cmd_list.indicies[i];
        
        if (idx.start < 0) {
            //fence
            while (waitpid(0, NULL, 0) > 0);
            continue;
        }

        pid_t p = fork();
        if (p) continue;
        char* file = &cmd_list.cmd[idx.start];
        
        //build list
        char** args = sb_alloc(sizeof(char*) * idx.length + 1);

        char* cur = &cmd_list.cmd[idx.start];
        for (uint32_t j = 0; j < idx.length; j++) {
            args[j] = cur;
            printf("%s ", cur);
            //consume
            while (cur[0]) cur++;
            cur++;
        }
        printf("\n");

        args[idx.length] = 0;
        execvp(file, args);
    }
    while (waitpid(0, NULL, 0) > 0);

    if (compile_cmds) {
        sb_fprintf(compile_cmds, "\n]\n");
        sb_close(compile_cmds);
    }
}

int sb_cmd_start() {
    curr_cmd.start = cmd_list.size;
    curr_cmd.length = 0;
    return 0;
}

void sb_cmd_end() {
    if (cmd_list.isize + 1 > cmd_list.icap) {
        cmd_list.icap = cmd_list.icap ? cmd_list.icap * 2 : 16;
        cmd_list.indicies = sb_realloc(cmd_list.indicies, cmd_list.icap * sizeof(sb_cmd));
    }
    
    if (curr_exe.fsize && curr_exe.dry) {
        cmd_list.size = curr_cmd.start;
        return;
    }

    cmd_list.indicies[cmd_list.isize++] = curr_cmd;
}

void _sb_cmd_main(sb_sized_string cmd) {
    curr_cmd.length++;

    if (cmd_list.size + cmd.size > cmd_list.cap) {
        cmd_list.cap = cmd_list.cap ? cmd_list.cap * 2 : 256;
        cmd_list.cmd = sb_realloc(cmd_list.cmd, cmd_list.cap);
    }

    sb_strcpy(&cmd_list.cmd[cmd_list.size], cmd);
    cmd_list.size += cmd.size;
    cmd_list.cmd[cmd_list.size] = 0;
    cmd_list.size += 1;
}

void _sb_cmd_opt(sb_sized_string opt) {
    curr_cmd.length++;
    if (cmd_list.size + opt.size + 2 > cmd_list.cap) {
        cmd_list.cap = cmd_list.cap ? cmd_list.cap * 2 : 256;
        cmd_list.cmd = sb_realloc(cmd_list.cmd, cmd_list.cap);
    }
    
    cmd_list.cmd[cmd_list.size - 1] = separator;
    cmd_list.cmd[cmd_list.size++] = flag;

    sb_strcpy(&cmd_list.cmd[cmd_list.size], opt);
    cmd_list.size += opt.size;
    cmd_list.cmd[cmd_list.size] = separator;
    cmd_list.size += 1;

}

uint32_t _sb_cmd_arg(sb_sized_string arg) {
    curr_cmd.length++;

    if (cmd_list.size + arg.size + 1 > cmd_list.cap) {
        cmd_list.cap = cmd_list.cap ? cmd_list.cap * 2 : 256;
        cmd_list.cmd = sb_realloc(cmd_list.cmd, cmd_list.cap);
    }

    cmd_list.cmd[cmd_list.size - 1] = separator;

    sb_strcpy(&cmd_list.cmd[cmd_list.size], arg);
    uint32_t index = cmd_list.size;
    cmd_list.size += arg.size;
    cmd_list.cmd[cmd_list.size] = separator;
    cmd_list.size += 1;

    return index;
}

void sb_autobuild(int argc, char* argv[], char* src) {
    //test if should rebuild
    
    struct stat srcinfo;
    struct stat bininfo;

    stat(argv[0], &bininfo);
    stat(src, &srcinfo);

    //rebuild based on compiler
    int rebuild = 0;
    sb_EXEC() {
        sb_add_file(src);
        sb_set_out(argv[0]);

        sb_export_command();
        if (srcinfo.st_mtim.tv_sec <= bininfo.st_mtim.tv_sec) {
            printf("No Rebuild\n");
            sb_dry_run();
            sb_stop_exec();
            sb_start_exec();
            sb_set_out(argv[0]);
            sb_cmd_opt("DSB_IMPL");
            sb_add_file("sb.h");
            sb_dry_run();
            sb_export_command();
        } else {
            rebuild = 1;
            printf("Rebuild\n");
        }
    }

    if (rebuild) {
        sb_build_end();

        execlp(argv[0], argv[0], NULL);
    }
}

void sb_fence() {
    if (cmd_list.isize + 1 > cmd_list.icap) {
        cmd_list.icap = cmd_list.icap ? cmd_list.icap * 2 : 16;
        cmd_list.indicies = sb_realloc(cmd_list.indicies, cmd_list.icap * sizeof(sb_cmd));
    }

    cmd_list.indicies[cmd_list.isize++] = (sb_cmd){
        .start = -1
    };
}

int sb_start_exec() {
    sb_cmd_start();
    _sb_cmd_main(compiler);
    curr_exe.fsize = 0;
    curr_exe.export_commands = 0;
    curr_exe.dry = 0;

    return 0;
}

//TODO(ELI): push exec index into exe info
void _sb_add_file(sb_sized_string f) {
    uint32_t index = _sb_cmd_arg(f);
    if (curr_exe.fsize + 1 > curr_exe.fcap) {
        curr_exe.fcap = curr_exe.fcap ? curr_exe.fcap * 2 : 2;
        curr_exe.files = sb_realloc(curr_exe.files, curr_exe.fcap);
    }

    curr_exe.files[curr_exe.fsize++] = index;
}

void _sb_set_out(sb_sized_string f) {
    sb_cmd_opt("o");
    _sb_cmd_arg(f);
}

void sb_set_optmize(uint32_t level) {
    char* op;
    switch (level) {
        default:
        case 0: return;

        case 1: op = "O1"; 
                break;
        case 2: op = "O2";
                break;
        case 3: op = "O3";
                break;
    }
    _sb_cmd_opt((sb_sized_string){
            .string = op,
            .size = sb_strlen(op)
    });
}

//TODO(ELI): Look into automatically enclosing
//path in parentheses to ensure it works with
//spaces
void _sb_add_include_path(sb_sized_string f) {
    sb_cmd_opt("-I");
    _sb_cmd_arg(f);
}

void _sb_add_library_path(sb_sized_string f) {
    sb_cmd_opt("-L");
    _sb_cmd_arg(f);
}

void _sb_link_library(sb_sized_string f) {
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "l%s", f.string);
    _sb_cmd_opt((sb_sized_string){
            .string = cmd,
            .size = sb_strlen(cmd),
    });
}

void sb_export_command() {
    curr_exe.export_commands = 1;
}

void sb_dry_run() {
    curr_exe.dry = 1;
}

//TODO(ELI): Set up to add a compile_commands
//entry for each file in the executable.
void sb_stop_exec() {
    if (curr_exe.export_commands) {
        //write out compile_commands file
        if (!compile_cmds) {
            compile_cmds = sb_open("compile_commands.json", sbf_WRITE, sbf_CREATE | sbf_TRUNC);
            sb_fprintf(compile_cmds, "[\n");
        } else {
            sb_fprintf(compile_cmds, ",\n");
        }

        for (int i = 0; i < curr_exe.fsize; i++) {
            sb_fprintf(compile_cmds, "{\n");

            sb_fprintf(compile_cmds, "\t\"directory\": \"%s\",\n", sb_get_cwd());
            sb_fprintf(compile_cmds, "\t\"file\": \"%s\",\n", &cmd_list.cmd[curr_exe.files[i]] );
            sb_fprintf(compile_cmds, "\t\"arguments\": [");

            sb_cmd idx = curr_cmd;
            char* cur = &cmd_list.cmd[idx.start];

            for (uint32_t j = 0; j < idx.length; j++) {
                sb_fprintf(compile_cmds, "\"%s\"", cur);
                //consume
                while (cur[0] != separator) cur++;
                cur++;

                if (j + 1 < idx.length) {
                    sb_fprintf(compile_cmds, ",");
                }
            }
            sb_fprintf(compile_cmds, "],\n");
            sb_fprintf(compile_cmds, "}");

            if (i + 1 < curr_exe.fsize) {
                sb_fprintf(compile_cmds, ",\n");
            }
        }

    }

    sb_cmd_end();
    curr_exe.fsize = 0;
    curr_exe.export_commands = 0;
    curr_exe.dry = 0;
}



#endif
#endif
