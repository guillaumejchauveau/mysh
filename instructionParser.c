#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "instruction.h"
#include "utils.h"
#include "instructionParser.h"

enum State {
  COMMAND_PATH = 0,
  COMMAND_ARG = 1,
  INPUT_REDIRECT = 2,
  OUTPUT_REDIRECT = 3,
  OUTPUT_A_REDIRECT = 4
};

void *syntaxError(char token) {
  resetCurrentInstruction();
  error(0, 0, "Syntax error near token '%c'", token);
  return NULL;
}

char *skipLine(char *input) {
  while (strchr("\n\0", *input) == NULL) {
    input++;
  }
  return input;
}

void saveToken(char *token, enum State *state, struct command *cmd) {
  if (*token == '\0') {
    return;
  }
  switch (*state) {
    case COMMAND_PATH:
      setCommandPath(cmd, token);
      break;
    case COMMAND_ARG:
      addCommandArg(cmd, token);
      break;
    case INPUT_REDIRECT:
      redirectCommandInput(cmd, token);
      break;
    case OUTPUT_REDIRECT:
      redirectCommandOutput(cmd, token);
      break;
    case OUTPUT_A_REDIRECT:
      redirectCommandOutputAppend(cmd, token);
      break;
  }
  *state = COMMAND_ARG;
  memset(token, '\0', sizeof(char) * 50); // TODO
}

int saveCommand(struct command *cmd, struct command *pipe_cmd) {
  addCommandToCurrentInstruction(cmd);
  if (cmd->path == NULL) { // Empty command.
    if (pipe_cmd == NULL) { // No previous command, empty instruction.
      return 0;
    }
    return -1; // Command was expected, syntax error.
  }
  if (pipe_cmd != NULL) {
    pipeCommands(pipe_cmd, cmd);
  }
  return 1;
}

char *parseInstruction(char *input) {
  resetCurrentInstruction();
  char token[50]; // TODO
  memset(token, '\0', sizeof(char) * 50);
  enum State state = COMMAND_PATH;
  struct command *cmd = createCommand(), *pipe_cmd = NULL;

  bool instruction_end;

  while (1) {
    switch (*input) {
      case '>': // Output redirection.
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input);
        }
        saveToken(token, &state, cmd);
        state = OUTPUT_REDIRECT;
        if (*(input + 1) == '>') { // Append mode.
          state = OUTPUT_A_REDIRECT;
          input++;
        }
        break;
      case '<': // Input redirection.
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input);
        }
        saveToken(token, &state, cmd);
        state = INPUT_REDIRECT;
        break;
      case '|':
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input);
        }
        saveToken(token, &state, cmd);

        if (saveCommand(cmd, pipe_cmd) != 1) { // A previous command is expected.
          return syntaxError(*input);
        }
        pipe_cmd = cmd;
        // Starts next command processing.
        cmd = createCommand();
        state = COMMAND_PATH;
        break;
      default:
        instruction_end = strchr("#;\n\0", *input) != NULL;

        if (instruction_end || isspace(*input)) {
          saveToken(token, &state, cmd);
          if (instruction_end) {
            if (saveCommand(cmd, pipe_cmd) < 0) {
              return syntaxError(*input);
            }
            if (*input == '\0') { // End of user input.
              return NULL;
            }
            if (*input == '#') { // Comment.
              return skipLine(input);
            }
            if (*input == ';' && *(input + 1) == ';') { // Illegal ';;'.
              return syntaxError(*input);
            }
            return input + 1;
          }
          continue;
        }
        if (strchr(RESERVED_KEYWORDS, *input)) {
          return syntaxError(*input);
        }
        strncat(token, input, 1);
    }
    input++;
  }
}
