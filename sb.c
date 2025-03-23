#ifndef SB_IMPL
#include "sb.h"
#endif

fproc *fork_ = fork;
execproc* execvp_ = execvp;
exitproc* exit_ = exit;

char* sb_cmd_push_arg_sized(sb_cmd* c, char* str, uint32_t len) {
    //grow textbuffer
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

    return handle;
}

int sb_cmd_sync(sb_cmd* c) {

    //build args list
    pid_t cid = sb_cmd_async(c);    

    int status;
    waitpid(cid, &status, 0); //idk options should be fine for now
                              
    char* args[c->asize + 1]; 
    memset(args, 0, sizeof(char*) * (c->asize + 1));

    char* at = c->textbuffer;
    for (int i = 0; i < c->asize; i++) {
        args[i] = at;
        while (at[0] != 0) at++; 
        at++;
    }

    printf("%s", args[0]);
    for (int i = 1; i < c->asize; i++) {
     printf(" %s", args[i]);
    }
    printf("\n");
    
    return status;
}

pid_t sb_cmd_async(sb_cmd* c) {
    char* args[c->asize + 1]; 
    memset(args, 0, sizeof(char*) * (c->asize + 1));

    char* at = c->textbuffer;
    for (int i = 0; i < c->asize; i++) {
        args[i] = at;
        while (at[0] != 0) at++; 
        at++;
    }

    //output command

    pid_t cid = fork_();
    if (cid == 0) {
        execvp_(args[0], args);
        exit_(1);
    }

    
    return cid;
}

int sb_cmd_fence(pid_t id) {
    int status;
    waitpid(id, &status, WUNTRACED);

    return status;
}

void sb_cmd_clear_args(sb_cmd* c) {
    c->asize = 0;
    c->tsize = 0;
}

void sb_cmd_free(sb_cmd* c) {
    free(c->textbuffer);
    c->textbuffer = 0;
}

