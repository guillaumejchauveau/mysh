#include <stdio.h>

#include <errno.h>
#include <error.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "instruction.h"
#include "instruction_parser.h"
#include "internal_commands.h"
#include "utils.h"

bool prompting = false;

void sigint_handler() {
  // Resets readline's input if it is currently prompting the user.
  if (prompting) {
    m_print("\n");
    rl_on_new_line();
    rl_replace_line("", 0);
    rl_redisplay();
  } else {
    m_print("Killed by signal 2.");
  }
}

int compute_instruction_status(const int status) {
  if (WIFSIGNALED(status)) {
    return 128 + WTERMSIG(status);
  }
  return WEXITSTATUS(status);
}

int execute_script(char *script, int line_id) {
  int status = 0, char_count;
  do {
    char_count = parse_instruction(script, line_id);
    if (char_count < 0) {
      return 1;
    }
    script += char_count;
    status = compute_instruction_status(execute_current_instruction());
    if (status != 0) {
      break;
    }
  } while (*script);
  reset_current_instruction();
  return status;
}

char *generate_prompt() {
  char *buf = getcwd(NULL, 0), *str;
  str = concat_string(3, "mysh:", buf, "$ ");
  free(buf);
  return str;
}

typedef enum mode Mode;
enum mode { MODE_INTERACTIVE, MODE_LINE_INPUT, MODE_FILE_INPUT };

void usage() {
  m_print("Usage:   mysh\n");
  m_print("         mysh script-file\n");
  m_print("         mysh -c instruction\n");
  exit(1);
}

int main(int argc, char **argv) {
  program_invocation_name = "mysh";
  // Initialises queues.
  init_current_instruction();
  init_ped_registry();

  signal(SIGINT, (__sighandler_t)sigint_handler);

  int status = 0, line_id = 1;
  ssize_t bytes_read;
  size_t script_buffer_length = 100;
  char *script = NULL;

  // Shell mode determination.
  Mode execution_mode = MODE_INTERACTIVE;
  char c;
  while ((c = (char)getopt(argc, argv, "c:")) != -1) {
    if (c == 'c') {
      execution_mode = MODE_LINE_INPUT;
      script = optarg;
    } else {
      usage();
      return 1;
    }
  }
  if (script == NULL) {
    if (argc == 1) {
      execution_mode = MODE_INTERACTIVE;
    } else if (argc == 2) {
      execution_mode = MODE_FILE_INPUT;
    } else {
      usage();
      return 1;
    }
  }

  char *prompt = NULL;
  int fd;
  switch (execution_mode) {
  case MODE_INTERACTIVE:
    prompt = generate_prompt();
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
      status = execute_script(script, line_id);
      prompting = true;
      prompt = generate_prompt();
    }
    free(prompt);
    break;
  case MODE_LINE_INPUT:
    status = execute_script(script, line_id);
    break;
  case MODE_FILE_INPUT:
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
      error(1, errno, "%s", argv[1]);
    }
    script = calloc(script_buffer_length, sizeof(char));
    c = '\0';
    while (1) {
      bytes_read = read(fd, &c, 1);
      if (bytes_read < 0) {
        error(-1, errno, "%s", argv[1]);
      }
      script_buffer_length =
          add_char_to_buffer(c, &script, script_buffer_length);
      if (c == '\n' || bytes_read == 0) {
        status = execute_script(script, line_id);
        if (status != 0) {
          break;
        }
        line_id++;
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
  exit_shell(status);
}
