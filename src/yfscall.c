#include "../include/yfs.h"
#include <stdlib.h>
#include <comp421/yalnix.h>

int YfsOpen(void* addr, int pid) {
    // copy the file name from client
    void* pathname = malloc(sizeof(MAXPATHNAMELEN));
    
    if(CopyFrom(pid, pathname, addr, MAXPATHNAMELEN) == ERROR)
        return ERROR;

    // parse the file name 
    ParsePathName(1, pathname);

    return 0;
}