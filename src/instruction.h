#ifndef MYSHELL__INSTRUCTION_H_
#define MYSHELL__INSTRUCTION_H_

#include "command.h"

void init_current_instruction();
void add_command_to_current_instruction(Command *cmd);
int execute_current_instruction();
void reset_current_instruction();

#endif // MYSHELL__INSTRUCTION_H_
