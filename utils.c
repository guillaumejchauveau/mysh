#include "utils.h"

#include <unistd.h>
#include <string.h>

void closeFileDescriptor(int fd) {
  if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
    if (close(fd) < 0) {
      error(-1, errno, "File descriptor closing failed");
    }
  }
}

char *gnu_getcwd() {
  size_t size = 100;

  while (1) {
    char *buffer = (char *) malloc(size);
    if (getcwd(buffer, size) == buffer)
      return buffer;
    free(buffer);
    if (errno != ERANGE)
      return 0;
    size *= 2;
  }
}

void *allocError() {
  error(-1, errno, "Memory allocation failed");
  return NULL;
}

char *cpyStr(const char *arg) {
  size_t arg_l = strlen(arg) + 1;
  char *saved_arg = malloc(arg_l * sizeof(char));
  if (saved_arg == NULL) {
    return allocError();
  }
  strcpy(saved_arg, arg);
  return saved_arg;
}
