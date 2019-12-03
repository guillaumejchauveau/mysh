#include "command.h"

#include <errno.h>
#include <error.h>
#include <stdlib.h>
#include <unistd.h>

#include "utils.h"

#define OUTPUT_FILE_PERMS S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

/*
 * PipeEndDescriptor is a structure holding the file descriptor belonging to a
 * pipe, either its read or write end. Used to make a list of all the file
 * descriptors associated with the pipes created by the program.
 */
typedef struct pipe_end_descriptor PipeEndDescriptor;
struct pipe_end_descriptor {
  TAILQ_ENTRY(pipe_end_descriptor) peds;
  int fd;
};
/*
 * registered_peds is the list of all the file descriptors associated with the pipes created by the
 * program.
 */
TAILQ_HEAD(tailhead, pipe_end_descriptor) registered_peds;
/*
 * init_ped_registry() initializes the list of pipe ends.
 */
void init_ped_registry() {
  TAILQ_INIT(&registered_peds);
}
/*
 * register_ped(int fd) adds a pipe end to the list of pipe ends.
 */
void register_ped(int fd) {
  PipeEndDescriptor *ped = calloc(1, sizeof(PipeEndDescriptor));
  if (ped == NULL) {
    alloc_error();
  }
  ped->fd = fd;
  TAILQ_INSERT_TAIL(&registered_peds, ped, peds);
}
/*
 * close_all_pipes() closes all the file descriptors involved in the pipes
 * created by the program. The list of pipe end descriptors is emptied and the
 * memory freed.
 */
void close_all_pipes() {
  struct pipe_end_descriptor *ped;
  while (registered_peds.tqh_first != NULL) {
    ped = registered_peds.tqh_first;
    TAILQ_REMOVE(&registered_peds, ped, peds);
    close_file_descriptor(ped->fd);
    free(ped);
  }
}
/*
 * close_unrelated_pipes(const Command *cmd) is similar to close_all_pipes() but
 * excludes the file descriptors used by the specified command.
 */
void close_unrelated_pipes(const Command *cmd) {
  struct pipe_end_descriptor *ped;
  struct pipe_end_descriptor *next = registered_peds.tqh_first;
  while (next != NULL) {
    if (next->fd != cmd->fd0 && next->fd != cmd->fd1) {
      ped = next;
      TAILQ_REMOVE(&registered_peds, ped, peds);
      close_file_descriptor(ped->fd);
      free(ped);
      next = registered_peds.tqh_first;
    } else {
      next = next->peds.tqe_next;
    }
  }
}

/*
 * create_command() initializes a new command structure.
 */
Command *create_command() {
  Command *cmd = calloc(1, sizeof(Command));
  if (cmd == NULL) {
    alloc_error();
  }
  cmd->args = calloc(2, sizeof(char *));
  if (cmd->args == NULL) {
    alloc_error();
  }

  cmd->args[0] = NULL; /* Command arg0. */
  cmd->args[1] = NULL; /* Arguments array termination for execvp(2). */

  cmd->fd0 = STDIN_FILENO;
  cmd->fd1 = STDOUT_FILENO;
  return cmd;
}
/*
 * destroy_command(Command *cmd) frees all memory blocks associated to the
 * specified command.
 */
void destroy_command(Command *cmd) {
  free(cmd->name);
  for (int i = 0; cmd->args[i] != NULL; i++) {
    free(cmd->args[i]);
  }
  free(cmd->args);
  free(cmd);
}
/*
 * set_command_name(Command *cmd, const char *name) replaces the name of the
 * specified command. The old name is freed from memory.
 */
void set_command_name(Command *cmd, const char *name) {
  free(cmd->name);
  cmd->name = copy_string(name);
  free(cmd->args[0]);
  cmd->args[0] = copy_string(name);
}
/*
 * add_command_arg(Command *cmd, const char *arg) adds an argument to be passed
 * to the specified command when executing.
 */
void add_command_arg(Command *cmd, const char *arg) {
  size_t args_length = 0;
  while (cmd->args[args_length] != NULL) {
    args_length++;
  }
  args_length += 2; /* New arg + NULL. */

  cmd->args = realloc(cmd->args, args_length * sizeof(char *));
  if (cmd->args == NULL) {
    alloc_error();
  }

  cmd->args[args_length - 2] = copy_string(arg);
  cmd->args[args_length - 1] = NULL;
}
/*
 * pipe_commands(Command *cmd1, Command *cmd2) creates a new pipe and sets file
 * descriptors 0 and 1 for the two provided commands, if they have not been
 * redirected to a file descriptor different than the default ones.
 * The pipe ends are registered in the pipe end descriptors registry.
 */
void pipe_commands(Command *cmd1, Command *cmd2) {
  int pipe_fd[2];
  if (pipe(pipe_fd) < 0) {
    error(-1, errno, "pipe creation failed");
  }
  /* Do not pipe if redirected. */
  if (cmd1->fd1 == STDOUT_FILENO) {
    cmd1->fd1 = pipe_fd[1];
  }
  if (cmd2->fd0 == STDIN_FILENO) {
    cmd2->fd0 = pipe_fd[0];
  }
  register_ped(pipe_fd[0]);
  register_ped(pipe_fd[1]);
}
/*
 * redirect_command_input(Command *cmd, const char *path) sets the file
 * descriptor 0 of the command to the specified file path opened for read.
 */
int redirect_command_input(Command *cmd, const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    return -1;
  }
  close_file_descriptor(cmd->fd0);
  cmd->fd0 = fd;
  return 0;
}
/*
 * redirect_command_output(Command *cmd, const char *path) sets the file
 * descriptor 1 of the command to the specified file path opened for write with
 * truncate.
 */
int redirect_command_output(Command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    return -1;
  }
  close_file_descriptor(cmd->fd1);
  cmd->fd1 = fd;
  return 0;
}
/*
 * redirect_command_output_append(Command *cmd, const char *path) sets the file
 * descriptor 1 of the command to the specified file path opened for write
 * without truncate.
 */
int redirect_command_output_append(Command *cmd, const char *path) {
  int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, OUTPUT_FILE_PERMS);
  if (fd < 0) {
    return -1;
  }
  close_file_descriptor(cmd->fd1);
  cmd->fd1 = fd;
  return 0;
}
/*
 * execute_command(const Command *cmd) executes the specified command in a child
 * process. The command's path is resolved by execvp(2). File descriptors 0 and
 * 1 are replaced by those set in the command's structure.
 */
pid_t execute_command(const Command *cmd) {
  pid_t fork_pid = fork();
  if (fork_pid == 0) {
    dup2(cmd->fd0, STDIN_FILENO);
    dup2(cmd->fd1, STDOUT_FILENO);
    close_unrelated_pipes(cmd);
    if (execvp(cmd->name, cmd->args) < 0) {
      if (errno == ENOENT) {
        error(127, errno, "%s", cmd->name);
      } else {
        error(-1, errno, "execution failed");
      }
    }
  } else if (fork_pid < 0) {
    error(-1, errno, "fork failed");
  }
  return fork_pid;
}

#undef OUTPUT_FILE_PERMS
