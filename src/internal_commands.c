#include "internal_commands.h"

#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "instruction.h"
#include "utils.h"

int change_working_directory(const char *path) {
  char *oldPWD = getcwd(NULL, 0);
  if (oldPWD == NULL) {
    error(-1, errno, "Working directory retrieval failed");
  }
  if (path == NULL) {
    path = getenv("HOME");
    if (path == NULL) {
      error(0, errno, "cd: cannot go to HOME, variable not set");
      return -1;
    }
  } else if (strcmp(path, "-") == 0) {
    path = getenv("OLDPWD");
    if (path == NULL) {
      path = oldPWD; // Do not change.
    }
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
