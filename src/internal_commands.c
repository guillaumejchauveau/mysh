#include "internal_commands.h"

#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "instruction.h"
#include "utils.h"

int change_working_directory(const char *path) {
  char *oldPWD = getcwd(NULL, 0);
  if (!path) {
    path = getenv("HOME");
  } else if (strcmp(path, "-") == 0) {
    path = getenv("OLDPWD");
    char *str = concat_string(2, path, "\n");
    m_print(str);
    free(str);
  }
  if (chdir(path) < 0) {
    error(0, errno, "cd: %s", path);
    return -1;
  }
  setenv("PWD", path, 1);
  setenv("OLDPWD", oldPWD, 1);
  free(oldPWD);
  return 0;
}

void exit_shell(int status) {
  reset_current_instruction();
  close_all_pipes();
  exit(status);
}
