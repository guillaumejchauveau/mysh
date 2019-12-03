#ifndef MYSHELL__UTILS_H_
#define MYSHELL__UTILS_H_

#include <stddef.h>
#include <sys/stat.h>

extern char *program_invocation_name;
void m_print(char *str);
void close_file_descriptor(int fd);
void alloc_error();
char *copy_string(const char *src);
char *concat_string(int src_count, ...);
size_t add_char_to_buffer(char c, char **buffer, size_t buffer_length);

#endif //MYSHELL__UTILS_H_
