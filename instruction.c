#include <sys/queue.h>
#include <sys/wait.h>
#include "utils.h"
#include "internalCommands.h"
#include "instruction.h"

TAILQ_HEAD(tailhead, command) current_instruction;

void initCurrentInstruction() {
  TAILQ_INIT(&current_instruction);
}

void addCommandToCurrentInstruction(struct command *cmd) {
  TAILQ_INSERT_TAIL(&current_instruction, cmd, instruction);
}

int executeCurrentInstruction() {
  struct command *cmd;
  bool doExit = false;
  for (cmd = current_instruction.tqh_first; cmd != NULL; cmd = cmd->instruction.tqe_next) {
    if (strcmp(cmd->path, "cd") == 0) {
      if (cmd->args[1] != NULL && cmd->args[2]) {
        mPrint("cd: too many arguments\n");
        return 1;
      }
      if (changeWorkingDirectory(cmd->args[1]) < 0) {
        return 1;
      }
    } else if (strcmp(cmd->path, "exit") == 0) {
      doExit = true;
      break;
    } else {
      executeCommand(cmd);
    }
  }
  closeAllPipes();

  int commandStatus = 0;
  while (wait(&commandStatus) != -1) {
  }
  if (errno != ECHILD) {
    error(-1, errno, "wait failed");
  }
  if (doExit) {
    exitShell(commandStatus);
  }

  return commandStatus;
}

void resetCurrentInstruction() {
  struct command *cmd;
  while (current_instruction.tqh_first != NULL) {
    cmd = current_instruction.tqh_first;
    TAILQ_REMOVE(&current_instruction, cmd, instruction);
    destroyCommand(cmd);
  }
}
