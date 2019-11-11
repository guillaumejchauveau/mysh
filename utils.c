#include "utils.h"

#include <unistd.h>

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
