#include "utils.h"

#include <errno.h>
#include <error.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void alloc_error() {
  error(-1, errno, "memory allocation failed");
  exit(-1);
}

void m_print(char *str) {
  size_t str_length = strlen(str);
  write(STDERR_FILENO, str, str_length * sizeof(char));
}

void close_file_descriptor(int fd) {
  if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
    if (close(fd) < 0) {
      error(-1, errno, "file descriptor closing failed");
    }
  }
}

char *copy_string(const char *src) {
  char *dest = calloc(strlen(src) + 1, sizeof(char));
  if (dest == NULL) {
    alloc_error();
  }
  strcpy(dest, src);
  return dest;
}

char *concat_string(int src_count, ...) {
  char *dest = calloc(1, sizeof(char)), *src;
  if (dest == NULL) {
    alloc_error();
  }
  *dest = '\0';
  size_t dest_length = 1;

  va_list ap;
  va_start(ap, src_count);

  for (int i = 0; i < src_count; i++) {
    src = va_arg(ap, char *);
    dest_length += strlen(src);
    dest = realloc(dest, dest_length * sizeof(char));
    if (dest == NULL) {
      alloc_error();
    }
    strcat(dest, src);
  }
  va_end(ap);

  return dest;
}

size_t add_char_to_buffer(char c, char **buffer, size_t buffer_length) {
  // Increases buffer size if it is too small.
  if (strlen(*buffer) + 2 > buffer_length) {
    buffer_length *= 2;
    *buffer = realloc(*buffer, buffer_length * sizeof(char));
    if (*buffer == NULL) {
      alloc_error();
    }
  }
  strncat(*buffer, &c, 1);
  return buffer_length;
}
