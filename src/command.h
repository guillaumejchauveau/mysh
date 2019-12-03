#ifndef MYSHELL__COMMAND_H_
#define MYSHELL__COMMAND_H_

#include <fcntl.h>
#include <sys/queue.h>

typedef struct command Command;
struct command {
  TAILQ_ENTRY(command) instruction;
  int fd0;
  int fd1;
  char *name;
  char **args;
};

void init_ped_registry();
void close_all_pipes();

Command *create_command();
void destroy_command(Command *cmd);
void set_command_name(Command *cmd, const char *name);
void add_command_arg(Command *cmd, const char *arg);
void pipe_commands(Command *cmd1, Command *cmd2);
int redirect_command_input(Command *cmd, const char *path);
int redirect_command_output(Command *cmd, const char *path);
int redirect_command_output_append(Command *cmd, const char *path);
pid_t execute_command(const Command *cmd);

#endif //MYSHELL__COMMAND_H_
