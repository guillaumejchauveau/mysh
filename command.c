#include <fcntl.h>
#include <string.h>
#include "utils.h"
#include "command.h"

void *allocError() {
  error(-1, errno, "Memory allocation failed");
  return NULL;
}

char *cpyStr(const char *arg) {
  size_t arg_l = strlen(arg) + 1;
  char *saved_arg = malloc(arg_l * sizeof(char));
  if (saved_arg == NULL) {
    return allocError();
  }
  strcpy(saved_arg, arg);
  return saved_arg;
}

struct command *createCommand() {
  struct command *cmd = calloc(1, sizeof(struct command));
  if (cmd == NULL) {
    return allocError();
  }
  cmd->args = calloc(2, sizeof(char *));
  if (cmd->args == NULL) {
    return allocError();
  }

  cmd->args[0] = NULL; // Command arg0.
  cmd->args[1] = NULL; // Arguments array termination for execvp(2).

  cmd->fd0 = STDIN_FILENO;
  cmd->fd1 = STDOUT_FILENO;
  cmd->input_pipe_write_d = -1;
  cmd->output_pipe_read_d = -1;
  return cmd;
}

void destroyCommand(struct command *cmd) {
  free(cmd->path);
  for (int i = 0; cmd->args[i] != NULL; i++) {
    free(cmd->args[i]);
  }
  free(cmd->args);
  free(cmd);
}

void setCommandPath(struct command *cmd, const char *path) {
  free(cmd->path);
  cmd->path = cpyStr(path);
  free(cmd->args[0]);
  cmd->args[0] = cpyStr(path);
}

void addCommandArg(struct command *cmd, const char *arg) {
  size_t args_l = 0;
  while (cmd->args[args_l] != NULL) {
    args_l++;
  }
  args_l += 2; // New arg + NULL.

  cmd->args = realloc(cmd->args, args_l * sizeof(char *));
  if (cmd->args == NULL) {
    allocError();
    return;
  }

  cmd->args[args_l - 2] = cpyStr(arg);
  cmd->args[args_l - 1] = NULL;
}

void pipeCommands(struct command *cmd1, struct command *cmd2) {
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    error(-1, errno, "Pipe creation failed");
  }
  // Do not pipe if redirected.
  if (cmd1->fd1 == STDOUT_FILENO) {
    cmd1->fd1 = pipe_fd[1];
  }
  if (cmd2->fd0 == STDIN_FILENO) {
    cmd2->fd0 = pipe_fd[0];
  }
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
    if (execvp(cmd->path, cmd->args) < 0) {
      if (errno == ENOENT) {
        error(127, errno, "%s", cmd->path);
      } else {
        error(-1, errno, "Execution failed");
      }
    }
  } else if (fPid < 0) {
    error(-1, errno, "Fork failed");
  }
  return fPid;
}
