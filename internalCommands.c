#include "utils.h"
#include "instruction.h"
#include "internalCommands.h"

void changeWorkingDirectory(const char *path) {
  char *oldPWD = getcwd(NULL, 0);
  if (!path) {
    path = getenv("HOME");
  } else if (strcmp(path, "-") == 0) {
    path = getenv("OLDPWD");
    fprintf(stderr, "%s\n", path);
  }
  if (chdir(path) < 0) {
    error(0, errno, "cd");
  }
  setenv("PWD", path, 1);
  setenv("OLDPWD", oldPWD, 1);
  free(oldPWD);
}

void exitShell(int status) {
  resetCurrentInstruction();
  closeAllPipes();
  fprintf(stderr, "\n");
  exit(status);
}
