#ifndef MYSHELL__COMMAND_H_
#define MYSHELL__COMMAND_H_

#include <sys/queue.h>
#include <unistd.h>

struct command {
  TAILQ_ENTRY(command) instruction;
  int fd0;
  int fd1;
  int input_pipe_write_d;
  int output_pipe_read_d;
  char *path;
  char **args;
};
struct command *createCommand();
pid_t executeCommand(const struct command *cmd);
void pipeCommands(struct command *cmd1, struct command *cmd2);
void redirectCommandInput(struct command *cmd, const char *path);
void redirectCommandOutput(struct command *cmd, const char *path);
void redirectCommandOutputAppend(struct command *cmd, const char *path);

#endif //MYSHELL__COMMAND_H_
