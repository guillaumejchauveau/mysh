#include <stdio.h>
#include <signal.h>

#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"
#include "internalCommands.h"

// TODO: cat <etc/passwd
// TODO: cat <etc/passwd | grep>lol /bin/false

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
  initPipeEndDescriptorRegistry();
  int lastInstructionStatus = 0;
  signal(SIGINT, handler);
/*
  struct command *cmd, *command2, *cmd3;

  cmd = createCommand();
  setCommandPath(cmd, "echo");
  addCommandArg(cmd, "lol");
  addCommandToCurrentInstruction(cmd);

  command2 = createCommand();
  setCommandPath(command2, "cat");
  addCommandToCurrentInstruction(command2);

  pipeCommands(cmd, command2);

  cmd3 = createCommand();
  setCommandPath(cmd3, "cat");
  addCommandToCurrentInstruction(cmd3);

  pipeCommands(command2, cmd3);*/

  fprintf(stderr, "'%s'\n", parseInstruction("echo lol | cat | cat"));
  lastInstructionStatus = computeInstructionStatus(executeCurrentInstruction());
  resetCurrentInstruction();
  return lastInstructionStatus;
}
