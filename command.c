#include "command.h"

#include "utils.h"
#include <fcntl.h>

struct command *createCommand() {
  struct command *cmd = malloc(sizeof(struct command));
  cmd->fd0 = STDIN_FILENO;
  cmd->fd1 = STDOUT_FILENO;
  cmd->input_pipe_write_d = -1;
  cmd->output_pipe_read_d = -1;
  return cmd;
}

pid_t executeCommand(const struct command *cmd) {
  pid_t fPid = fork();
  if (fPid == 0) {
    dup2(cmd->fd0, STDIN_FILENO);
    dup2(cmd->fd1, STDOUT_FILENO);
    if (cmd->input_pipe_write_d != -1) {
      closeFileDescriptor(cmd->input_pipe_write_d);
    }
    if (cmd->output_pipe_read_d != -1) {
      closeFileDescriptor(cmd->output_pipe_read_d);
    }
    if (execv(cmd->path, cmd->args) < 0) {
      error(-1, errno, "Execution failed");
    }
  } else if (fPid < 0) {
    error(-1, errno, "Fork failed");
  }
  return fPid;
}

void pipeCommands(struct command *cmd1, struct command *cmd2) {
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    error(-1, errno, "Pipe creation failed");
  }
  cmd1->fd1 = pipe_fd[1];
  cmd2->fd0 = pipe_fd[0];
  cmd1->output_pipe_read_d = pipe_fd[0];
  cmd2->input_pipe_write_d = pipe_fd[1];
}

void redirectCommandInput(struct command *cmd, const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    error(0, errno, "%s", path);
    return;
  }
  closeFileDescriptor(cmd->fd0);
  cmd->fd0 = fd;
}

void redirectCommandOutput(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    error(0, errno, "%s", path);
    return;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
}

void redirectCommandOutputAppend(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    error(0, errno, "%s", path);
    return;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
}
