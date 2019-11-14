#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>

#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"
#include "internalCommands.h"

// TODO: Check C style.
// TODO: Readline history.
// TODO: Replace fprintf.
// TODO: Make ';<SPACE>;' illegal.
// TODO: Support '-c "line"' option.
// TODO: Support non-interactive mode.
// TODO: Dynamic instruction parser token size.
// TODO: Fix 'cat <etc/passwd'
// TODO: Fix 'cat <etc/passwd | grep>lol /bin/false'

bool prompting = false;

void sigintHandler() {
  if (prompting) {
    fprintf(stderr, "\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
  } else {
    fprintf(stderr, "Killed by signal %d.\n", SIGINT);
  }
}

int computeInstructionStatus(const int status) {
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return WEXITSTATUS(status);
}

int executeLine(char *line, int lineNmb) {
  int status = 0, charCount;
  do {
    charCount = parseInstruction(line, lineNmb);
    if (charCount < 0) {
      return 1;
    }
    line += charCount;
    status = computeInstructionStatus(executeCurrentInstruction());
  } while (*line);
  resetCurrentInstruction();
  return status;
}

char *generatePrompt() {
  return concatStr(3, "mysh:", getcwd(NULL, 0), "$ ");
}

int main() {
  program_invocation_name = "mysh";
  initCurrentInstruction();
  initPipeEndDescriptorRegistry();
  int status = 0;
  signal(SIGINT, sigintHandler);

  char *line, *prompt = generatePrompt();
  prompting = true;
  while ((line = readline(prompt))) {
    prompting = false;
    free(prompt);
    status = executeLine(line, 1);
    prompting = true;
    prompt = generatePrompt();
  }

  free(prompt);
  exitShell(status);
}
