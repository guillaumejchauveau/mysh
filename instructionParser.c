#include "utils.h"
#include "instruction.h"
#include "instructionParser.h"

enum State {
  COMMAND_PATH = 0,
  COMMAND_ARG = 1,
  INPUT_REDIRECT = 2,
  OUTPUT_REDIRECT = 3,
  OUTPUT_A_REDIRECT = 4
};

int syntaxError(char src, int lineNmb, char *token) {
  resetCurrentInstruction();
  free(token);
  fprintf(stderr, "error:%d: syntax error near '%c'\n", lineNmb, src);
  return -1;
}

int skipLine(char *input, int charCount) {
  while (strchr("\n\0", *input) == NULL) {
    input++;
    charCount++;
  }
  return charCount;
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
  if (cmd->path == NULL) {
    *state = COMMAND_PATH;
  } else {
    *state = COMMAND_ARG;
  }
  *token = '\0';
}

int saveCommand(struct command *cmd, struct command *pipe_cmd) {
  if (cmd->path == NULL) { // Empty command.
    if (pipe_cmd == NULL) { // No previous command, empty instruction.
      return 0;
    }
    return -1; // Command was expected, syntax error.
  }
  if (pipe_cmd != NULL) {
    pipeCommands(pipe_cmd, cmd);
  }
  addCommandToCurrentInstruction(cmd);
  return 1;
}

int parseInstruction(char *input, int lineNmb) {
  resetCurrentInstruction();

  char *token = calloc(1, sizeof(char));
  if (token == NULL) {
    allocError();
    return -1;
  }
  *token = '\0';

  int charCount = 0;
  enum State state = COMMAND_PATH;
  struct command *cmd = createCommand(), *pipe_cmd = NULL;

  bool instruction_end;

  while (1) {
    switch (*input) {
      case '>': // Output redirection.
        saveToken(token, &state, cmd);
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input, lineNmb, token);
        }
        state = OUTPUT_REDIRECT;
        if (*(input + 1) == '>') { // Append mode.
          state = OUTPUT_A_REDIRECT;
          input++;
          charCount++;
        }
        break;
      case '<': // Input redirection.
        saveToken(token, &state, cmd);
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input, lineNmb, token);
        }
        state = INPUT_REDIRECT;
        break;
      case '|':
        saveToken(token, &state, cmd);
        if (state > 1) { // Normal token expected for previous instruction.
          return syntaxError(*input, lineNmb, token);
        }

        if (saveCommand(cmd, pipe_cmd) != 1) { // A previous command is expected.
          return syntaxError(*input, lineNmb, token);
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
            free(token);
            token = NULL;
            if (saveCommand(cmd, pipe_cmd) < 0) {
              return syntaxError(*input, lineNmb, token);
            }
            if (*input == '\0') { // End of user input.
              return charCount;
            }
            if (*input == '#') { // Comment.
              return skipLine(input, charCount);
            }
            if (*input == ';' && *(input + 1) == ';') { // Illegal ';;'.
              return syntaxError(*input, lineNmb, token);
            }
            return charCount + 1;
          }
          break;
        }
        if (strchr(RESERVED_KEYWORDS, *input)) {
          return syntaxError(*input, lineNmb, token);
        }
        token = realloc(token, (strlen(token) + 2) * sizeof(char));
        if (token == NULL) {
          allocError();
          return -1;
        }
        strncat(token, input, 1);
    }
    input++;
    charCount++;
  }
}
