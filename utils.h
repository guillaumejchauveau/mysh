#ifndef MYSHELL__UTILS_H_
#define MYSHELL__UTILS_H_

#include <stdlib.h>
#include <errno.h>
#include <error.h>

#define OUTPUT_FILE_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

extern char *program_invocation_name;
void closeFileDescriptor(int fd);
char *gnu_getcwd();

#endif //MYSHELL__UTILS_H_
