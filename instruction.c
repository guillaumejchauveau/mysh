#include "instruction.h"
#include "utils.h"
#include <sys/wait.h>

TAILQ_HEAD(tailhead, command) current_instruction;

void initCurrentInstruction() {
  TAILQ_INIT(&current_instruction);
}

void addCommandToCurrentInstruction(struct command *cmd) {
  TAILQ_INSERT_TAIL(&current_instruction, cmd, instruction);
}

int executeCurrentInstruction() {
  struct command *cmd;
  for (cmd = current_instruction.tqh_first; cmd != NULL; cmd = cmd->instruction.tqe_next) {
    executeCommand(cmd);
  }
  for (cmd = current_instruction.tqh_first; cmd != NULL; cmd = cmd->instruction.tqe_next) {
    closeFileDescriptor(cmd->fd0);
    closeFileDescriptor(cmd->fd1);
  }

  int commandStatus;
  while (wait(&commandStatus) != -1) {
  }
  if (errno != ECHILD) {
    error(-1, errno, "Wait failed");
  }

  return commandStatus;
}

void resetCurrentInstruction() {
  struct command *cmd;
  while (current_instruction.tqh_first != NULL) {
    cmd = current_instruction.tqh_first;
    TAILQ_REMOVE(&current_instruction, cmd, instruction);
    free(cmd);
  }
}
