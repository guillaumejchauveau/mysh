#ifndef MYSHELL__UTILS_H_
#define MYSHELL__UTILS_H_

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <error.h>
#include <err.h>

#define OUTPUT_FILE_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

extern char *program_invocation_name;
void mPrint(char *str);
void closeFileDescriptor(int fd);
void *allocError();
char *cpyStr(const char *src);
char *concatStr(int srcCount, ...);
int addToStrBuffer(char c, char **buffer, size_t buffer_l);

#endif //MYSHELL__UTILS_H_
