#include "instruction.h"

#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>
#include <sys/wait.h>

#include "internal_commands.h"
#include "utils.h"

TAILQ_HEAD(tailhead, command) current_instruction;

void init_current_instruction() { TAILQ_INIT(&current_instruction); }

void add_command_to_current_instruction(Command *cmd) {
  TAILQ_INSERT_TAIL(&current_instruction, cmd, instruction);
}

int execute_current_instruction() {
  Command *cmd;
  bool do_exit = false;
  for (cmd = TAILQ_FIRST(&current_instruction); cmd != NULL;
       cmd = TAILQ_NEXT(cmd, instruction)) {
    // Filters internal commands.
    if (strcmp(cmd->name, "cd") == 0) {
      if (cmd->args[1] != NULL && cmd->args[2]) {
        m_print("cd: too many arguments\n");
        return 1;
      }
      if (change_working_directory(cmd->args[1]) < 0) {
        return 1;
      }
    } else if (strcmp(cmd->name, "exit") == 0) {
      // Exit only when all previous commands are terminated.
      do_exit = true;
      break;
    } else {
      execute_command(cmd);
    }
  }
  close_all_pipes();

  int cmd_status = 0;
  while (wait(&cmd_status) != -1) {
  }
  if (errno != ECHILD) {
    error(-1, errno, "wait failed");
  }
  if (do_exit) {
    exit_shell(cmd_status);
  }

  return cmd_status;
}

void reset_current_instruction() {
  Command *cmd;
  while (TAILQ_FIRST(&current_instruction) != NULL) {
    cmd = TAILQ_FIRST(&current_instruction);
    TAILQ_REMOVE(&current_instruction, cmd, instruction);
    destroy_command(cmd);
  }
}
