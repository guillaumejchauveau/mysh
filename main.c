#include <stdio.h>
#include <signal.h>

#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"
#include "internalCommands.h"

void handler(int signum) {
  fprintf(stderr, "Signal %d\n", signum);
}

int computeInstructionStatus(const int status) {
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
  setCommandPath(cmd, "cat");
  addCommandToCurrentInstruction(cmd);
  redirectCommandInput(cmd, "/etc/passwd");

  command2 = createCommand();
  setCommandPath(command2, "grep");
  addCommandArg(command2, "/bin/false");
  addCommandToCurrentInstruction(command2);
  redirectCommandOutput(command2, "lol");

  pipeCommands(cmd, command2);

  //parseInstruction("cat /etc/passwd");
  lastInstructionStatus = computeInstructionStatus(executeCurrentInstruction());
  resetCurrentInstruction();
  return lastInstructionStatus;
}
