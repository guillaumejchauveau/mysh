#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/queue.h>
#include <sys/wait.h>

#define OUTPUT_FILE_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

void closeFileDescriptor(int fd) {
  if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
    if (close(fd) < 0) {
      perror("File descriptor closing failed");
      exit(-1);
    }
  }
}

void myshError(const char *msg, int status) {
  fprintf(stderr, "mysh: %s: %s\n", strerror(errno), msg);
}

struct command {
  TAILQ_ENTRY(command) commands;
  int fd0;
  int fd1;
  int input_pipe_write_d;
  int output_pipe_read_d;
  char *path;
  char **args;
};

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
      perror("Execution failed");
      exit(-1);
    }
  } else if (fPid < 0) {
    perror("Fork failed");
    exit(-1);
  }
  return fPid;
}

void pipeCommands(struct command *cmd1, struct command *cmd2) {
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    perror("Pipe creation failed");
    exit(-1);
  }
  cmd1->fd1 = pipe_fd[1];
  cmd2->fd0 = pipe_fd[0];
  cmd1->output_pipe_read_d = pipe_fd[0];
  cmd2->input_pipe_write_d = pipe_fd[1];
}

void redirectCommandInput(struct command *cmd, const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    myshError(path, 1);
    return;
  }
  closeFileDescriptor(cmd->fd0);
  cmd->fd0 = fd;
}

void redirectCommandOutput(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    myshError(path, 1);
    return;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
}

void redirectCommandOutputAppend(struct command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    myshError(path, 1);
    return;
  }
  closeFileDescriptor(cmd->fd1);
  cmd->fd1 = fd;
}

void handler(int signum) {
  fprintf(stderr, "Signal %d\n", signum);
}

int main() {
  signal(SIGINT, handler);

  TAILQ_HEAD(tailhead, command) commands;
  TAILQ_INIT(&commands);
  struct command *command, *command2;

  command = createCommand();
  command->path = "/bin/cat";
  char *args[] = {"cat", NULL};
  command->args = args;
  TAILQ_INSERT_TAIL(&commands, command, commands);

  command2 = createCommand();
  command2->path = "/bin/grep";
  char *args2[] = {"grep", "/bin/false", NULL};
  command2->args = args2;
  TAILQ_INSERT_TAIL(&commands, command2, commands);

  pipeCommands(command, command2);

  for (command = commands.tqh_first; command != NULL; command = command->commands.tqe_next) {
    executeCommand(command);
  }
  for (command = commands.tqh_first; command != NULL; command = command->commands.tqe_next) {
    closeFileDescriptor(command->fd0);
    closeFileDescriptor(command->fd1);
  }

  int commandStatus;
  while (wait(&commandStatus) != -1) {
    fprintf(stderr, "Command exited with status %d\n", commandStatus);
  }

  if (errno != ECHILD) {
    perror("Wait failed");
    exit(-1);
  }

  while (commands.tqh_first != NULL) {
    command = commands.tqh_first;
    TAILQ_REMOVE(&commands, command, commands);
    free(command);
  }
}
