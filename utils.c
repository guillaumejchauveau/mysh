#include "utils.h"
#include <stdarg.h>

void *allocError() {
  error(-1, errno, "memory allocation failed");
  return NULL;
}

void mPrint(char *str) {
  size_t __l = strlen(str);
  write(STDERR_FILENO, str, __l * sizeof(char));
}

void closeFileDescriptor(int fd) {
  if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
    if (close(fd) < 0) {
      error(-1, errno, "file descriptor closing failed");
    }
  }
}

char *cpyStr(const char *src) {
  char *dest = calloc(strlen(src) + 1, sizeof(char));
  if (dest == NULL) {
    return allocError();
  }
  strcpy(dest, src);
  return dest;
}

char *concatStr(int srcCount, ...) {
  char *dest = calloc(1, sizeof(char)), *src;
  if (dest == NULL) {
    return allocError();
  }
  *dest = '\0';
  size_t dest_l = 1;

  va_list ap;
  va_start(ap, srcCount);

  for (int i = 0; i < srcCount; i++) {
    src = va_arg(ap, char *);
    dest_l += strlen(src);
    dest = realloc(dest, dest_l * sizeof(char));
    if (dest == NULL) {
      return allocError();
    }
    strcat(dest, src);
  }
  va_end(ap);

  return dest;
}

int addToStrBuffer(char c, char **buffer, size_t buffer_l) {
  if (strlen(*buffer) + 2 > buffer_l) {
    buffer_l *= 2;
    *buffer = realloc(*buffer, buffer_l * sizeof(char));
    if (*buffer == NULL) {
      allocError();
      return -1;
    }
  }
  strncat(*buffer, &c, 1);
  return buffer_l;
}
