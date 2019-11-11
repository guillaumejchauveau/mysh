#include "internalCommands.h"
#include "utils.h"
#include <unistd.h>

void changeWorkingDirectory(char *path) {
  char *oldPWD = gnu_getcwd();
  if (chdir(path) < 0) {
    error(0, errno, "cd");
  }
  setenv("PWD", path, 1);
  setenv("OLDPWD", oldPWD, 1);
}
