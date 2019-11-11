#include <stdio.h>
#include <signal.h>

#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"
#include "internalCommands.h"

void handler(int signum) {
  fprintf(stderr, "Signal %d\n", signum);
}

int computeInstructionStatus(int status) {
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return WEXITSTATUS(status);
}

int main() {
  program_invocation_name = "mysh";
  initCurrentInstruction();
  int lastInstructionStatus = 0;
  signal(SIGINT, handler);

  struct command *cmd, *command2;

  cmd = createCommand();
  cmd->path = "/bin/cat";
  char *args[] = {"cat", NULL};
  cmd->args = args;
  addCommandToCurrentInstruction(cmd);

  command2 = createCommand();
  command2->path = "/bin/grep";
  char *args2[] = {"grep", "/bin/false", NULL};
  command2->args = args2;
  addCommandToCurrentInstruction(command2);

  pipeCommands(cmd, command2);

  lastInstructionStatus = computeInstructionStatus(executeCurrentInstruction());
  resetCurrentInstruction();
  return lastInstructionStatus;
}
