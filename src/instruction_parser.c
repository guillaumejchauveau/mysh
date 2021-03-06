#include "instruction_parser.h"

#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instruction.h"
#include "utils.h"

typedef enum state State;

enum state {
  STATE_COMMAND_PATH,
  STATE_COMMAND_ARG,
  STATE_INPUT_REDIRECT,
  STATE_OUTPUT_REDIRECT,
  STATE_OUTPUT_A_REDIRECT
};

int syntax_error(char src, int line_id, char *token) {
  char _lineNmb[10];
  sprintf(_lineNmb, "%d", line_id);
  char *str = concat_string(5, "error:", _lineNmb, ": syntax error near '",
                            &src, "'\n");
  m_print(str);
  free(str);
  reset_current_instruction();
  free(token);
  return -1;
}

int token_error(char *token, int line_id) {
  char line_id_str[10];
  sprintf(line_id_str, "%d", line_id);
  char *str = concat_string(7, "error:", line_id_str, ": invalid token '",
                            token, "' : ", strerror(errno), "\n");
  m_print(str);
  free(str);
  reset_current_instruction();
  free(token);
  return -1;
}

int skip_line(char *input, int char_count) {
  while (strchr("\n\0", *input) == NULL) {
    input++;
    char_count++;
  }
  return char_count;
}

int save_token(char *token, State *state, Command *cmd) {
  if (*token == '\0') {
    return 0; // Empty token.
  }
  switch (*state) {
  case STATE_COMMAND_PATH:
    set_command_name(cmd, token);
    break;
  case STATE_COMMAND_ARG:
    add_command_arg(cmd, token);
    break;
  case STATE_INPUT_REDIRECT:
    if (redirect_command_input(cmd, token) < 0) {
      error(0, errno, "%s", token);
      return -1; // Error.
    }
    break;
  case STATE_OUTPUT_REDIRECT:
    if (redirect_command_output(cmd, token) < 0) {
      error(0, errno, "%s", token);
      return -1; // Error.
    }
    break;
  case STATE_OUTPUT_A_REDIRECT:
    if (redirect_command_output_append(cmd, token) < 0) {
      error(0, errno, "%s", token);
      return -1; // Error.
    }
    break;
  }
  if (cmd->name == NULL) {
    // Goes back to the first state if a redirection appeared before the command
    // name.
    *state = STATE_COMMAND_PATH;
  } else {
    *state = STATE_COMMAND_ARG;
  }
  *token = '\0';
  return 1; // OK.
}

int save_command(Command *cmd, Command *pipe_cmd) {
  if (cmd->name == NULL) {  // Empty command.
    if (pipe_cmd == NULL) { // No previous command, empty instruction.
      return 0;
    }
    return -1; // Command was expected, syntax error.
  }
  if (pipe_cmd != NULL) {
    pipe_commands(pipe_cmd, cmd);
  }
  add_command_to_current_instruction(cmd);
  return 1;
}

/**
 * State-based parser. Either tokens like '>', ';'... or the state are used to
 * determine how to construct commands and apply redirections. The resulting
 * command structures are saved in the current instruction queue.
 */
int parse_instruction(char *input, int line_id) {
  reset_current_instruction();

  size_t token_buffer_length = 4;
  char *token = calloc(token_buffer_length, sizeof(char));
  if (token == NULL) {
    alloc_error();
  }
  *token = '\0';

  int char_count = 0;
  State state = STATE_COMMAND_PATH;
  Command *cmd = create_command(), *pipe_cmd = NULL;

  bool instruction_end;

  while (1) {
    switch (*input) {
    case '>': // Output redirection.
      if (save_token(token, &state, cmd) < 0) {
        return token_error(token, line_id);
      }
      if (state >= STATE_INPUT_REDIRECT) { // Normal token expected for previous
                                           // instruction.
        return syntax_error(*input, line_id, token);
      }
      state = STATE_OUTPUT_REDIRECT;
      if (*(input + 1) == '>') { // Append mode.
        state = STATE_OUTPUT_A_REDIRECT;
        input++;
        char_count++;
      }
      break;
    case '<': // Input redirection.
      if (save_token(token, &state, cmd) < 0) {
        return token_error(token, line_id);
      }
      if (state >= STATE_INPUT_REDIRECT) { // Normal token expected for previous
                                           // instruction.
        return syntax_error(*input, line_id, token);
      }
      state = STATE_INPUT_REDIRECT;
      break;
    case '|':
      if (save_token(token, &state, cmd) < 0) {
        return token_error(token, line_id);
      }
      if (state >= STATE_INPUT_REDIRECT) { // Normal token expected for previous
                                           // instruction.
        return syntax_error(*input, line_id, token);
      }

      if (save_command(cmd, pipe_cmd) != 1) { // A previous command is expected.
        return syntax_error(*input, line_id, token);
      }
      pipe_cmd = cmd;
      // Starts next command processing.
      cmd = create_command();
      state = STATE_COMMAND_PATH;
      break;
    default:
      instruction_end = strchr("#;\n\0", *input) != NULL;

      if (instruction_end || isspace(*input)) {
        if (save_token(token, &state, cmd) < 0) {
          return token_error(token, line_id);
        }
        if (instruction_end) {
          free(token);
          token = NULL;
          if (save_command(cmd, pipe_cmd) < 0) {
            return syntax_error(*input, line_id, token);
          }
          if (*input == '\0') { // End of user input.
            return char_count;
          }
          if (*input == '#') { // Comment.
            return skip_line(input, char_count);
          }
          if (*input == ';') { // Illegal ';;'.
            int i = 1;
            while (isspace(input[i]) && input[i] != '\n' && input[i] != '\0') {
              i++;
            }
            if (input[i] == ';') {
              return syntax_error(*input, line_id, token);
            }
            return char_count + i;
          }
          return char_count + 1;
        }
        break;
      }
      // Reserved tokens.
      if (strchr("\"\'\\${}()[]*?~&=!", *input)) {
        return syntax_error(*input, line_id, token);
      }
      token_buffer_length =
          add_char_to_buffer(*input, &token, token_buffer_length);
    }
    input++;
    char_count++;
  }
}
