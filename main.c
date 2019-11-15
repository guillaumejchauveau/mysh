#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"
#include "internalCommands.h"

// TODO: Check C style.
// TODO: Replace fprintf.
// TODO: Make ';<SPACE>;' illegal.
// TODO: Fix 'cat <etc/passwd'
// TODO: Fix 'cat <etc/passwd | grep>lol /bin/false'
// TODO: Add usage message.

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

int executeScript(char *script, int lineNmb) {
  int status = 0, charCount;
  do {
    charCount = parseInstruction(script, lineNmb);
    if (charCount < 0) {
      return 1;
    }
    script += charCount;
    status = computeInstructionStatus(executeCurrentInstruction());
  } while (*script);
  resetCurrentInstruction();
  return status;
}

char *generatePrompt() {
  return concatStr(3, "mysh:", getcwd(NULL, 0), "$ ");
}

enum mode {
  INTERACTIVE,
  LINE_INPUT,
  FILE_INPUT
};

void usage() {
  fprintf(stderr, "Usage: \n");
  exit(1);
}

int main(int argc, char **argv) {
  program_invocation_name = "mysh";
  initCurrentInstruction();
  initPipeEndDescriptorRegistry();
  signal(SIGINT, sigintHandler);

  int status = 0, line_number = 1, bytes_read;
  size_t script_buffer_l = 100;
  char *script = NULL;

  enum mode m = INTERACTIVE;
  int c;
  while ((c = getopt(argc, argv, "c:")) != -1) {
    if (c == 'c') {
      m = LINE_INPUT;
      script = optarg;
    } else {
      usage();
      return 1;
    }
  }
  if (script == NULL) {
    if (argc == 1) {
      m = INTERACTIVE;
    } else if (argc == 2) {
      m = FILE_INPUT;
    } else {
      usage();
      return 1;
    }
  }

  char *prompt;
  int fd;
  switch (m) {
    case INTERACTIVE:
      prompt = generatePrompt();
      prompting = true;
      while ((script = readline(prompt))) {
        prompting = false;
        free(prompt);
        for (char *l = script; *l != '\0'; l++) {
          if (!isspace(*l)) {
            add_history(script);
            break;
          }
        }
        status = executeScript(script, line_number);
        prompting = true;
        prompt = generatePrompt();
      }
      free(prompt);
      break;
    case LINE_INPUT:
      status = executeScript(script, line_number);
      break;
    case FILE_INPUT:
      fd = open(argv[1], O_RDONLY);
      if (fd < 0) {
        error(1, errno, "%s", argv[1]);
      }
      script = calloc(script_buffer_l, sizeof(char));
      char c = '\0';
      while (1) {
        bytes_read = read(fd, &c, 1);
        if (bytes_read < 0) {
          error(-1, errno, "%s", argv[1]);
        }
        addToStrBuffer(c, &script, script_buffer_l);
        if (c == '\n' || bytes_read == 0) {
          status = executeScript(script, line_number);
          if (status != 0) {
            break;
          }
          line_number++;
          *script = '\0';
        }
        if (bytes_read == 0) {
          break;
        }
      }
      close(fd);
      free(script);
      break;
  }

  exitShell(status);
}
