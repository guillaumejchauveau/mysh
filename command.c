#include "utils.h"
#include "command.h"

struct pipeEndDescriptor {
  TAILQ_ENTRY(pipeEndDescriptor) pipeEndDescriptors;
  int fd;
};

TAILQ_HEAD(tailhead, pipeEndDescriptor) registered_pipe_end_descriptors;

void initPipeEndDescriptorRegistry() {
  TAILQ_INIT(&registered_pipe_end_descriptors);
}

void registerPipeEndDescriptor(int fd) {
  struct pipeEndDescriptor *ped = calloc(1, sizeof(struct pipeEndDescriptor));
  if (ped == NULL) {
    allocError();
    return;
  }
  ped->fd = fd;
  TAILQ_INSERT_TAIL(&registered_pipe_end_descriptors, ped, pipeEndDescriptors);
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
    error(-1, errno, "pipe creation failed");
  }
  // Do not pipe if redirected.
  if (cmd1->fd1 == STDOUT_FILENO) {
    cmd1->fd1 = pipe_fd[1];
  }
  if (cmd2->fd0 == STDIN_FILENO) {
    cmd2->fd0 = pipe_fd[0];
  }
  registerPipeEndDescriptor(pipe_fd[0]);
  registerPipeEndDescriptor(pipe_fd[1]);
}

int redirectCommandInput(struct command *cmd, const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return -1;
  }
  closeFileDescriptor(cmd->fd0);
  cmd->fd0 = fd;
  return 0;
}

int redirectCommandOutput(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    return -1;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
  return 0;
}

int redirectCommandOutputAppend(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    return -1;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
  return 0;
}

void closeAllPipes() {
  struct pipeEndDescriptor *ped;
  while (registered_pipe_end_descriptors.tqh_first != NULL) {
    ped = registered_pipe_end_descriptors.tqh_first;
    TAILQ_REMOVE(&registered_pipe_end_descriptors, ped, pipeEndDescriptors);
    closeFileDescriptor(ped->fd);
    free(ped);
  }
}

void closeUnrelatedPipes(const struct command *cmd) {
  struct pipeEndDescriptor *ped;
  struct pipeEndDescriptor *next = registered_pipe_end_descriptors.tqh_first;
  while (next != NULL) {
    if (next->fd != cmd->fd0 && next->fd != cmd->fd1) {
      ped = next;
      TAILQ_REMOVE(&registered_pipe_end_descriptors, ped, pipeEndDescriptors);
      closeFileDescriptor(ped->fd);
      free(ped);
      next = registered_pipe_end_descriptors.tqh_first;
    } else {
      next = next->pipeEndDescriptors.tqe_next;
    }
  }
}

pid_t executeCommand(const struct command *cmd) {
  pid_t fPid = fork();
  if (fPid == 0) {
    dup2(cmd->fd0, STDIN_FILENO);
    dup2(cmd->fd1, STDOUT_FILENO);
    closeUnrelatedPipes(cmd);
    if (execvp(cmd->path, cmd->args) < 0) {
      if (errno == ENOENT) {
        error(127, errno, "%s", cmd->path);
      } else {
        error(-1, errno, "execution failed");
      }
    }
  } else if (fPid < 0) {
    error(-1, errno, "fork failed");
  }
  return fPid;
}
