#include "utils.h"
#include "instruction.h"

int changeWorkingDirectory(const char *path) {
  char *oldPWD = getcwd(NULL, 0);
  if (!path) {
    path = getenv("HOME");
  } else if (strcmp(path, "-") == 0) {
    path = getenv("OLDPWD");
    char *str = concatStr(2, path, "\n");
    mPrint(str);
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

void exitShell(int status) {
  resetCurrentInstruction();
  closeAllPipes();
  exit(status);
}
