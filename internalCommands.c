#include "utils.h"
#include <unistd.h>
#include <string.h>
#include "internalCommands.h"

void changeWorkingDirectory(const char *path) {
  char *oldPWD = gnu_getcwd();
  if (path == NULL || strcmp(path, "") == 0) {
    path = getenv("HOME");
  } else if (strcmp(path, "-") == 0) {
    path = getenv("OLDPWD");
  }
  if (chdir(path) < 0) {
    error(0, errno, "cd");
  }
  setenv("PWD", path, 1);
  setenv("OLDPWD", oldPWD, 1);
  free(oldPWD);
}
