#ifndef MYSHELL__INSTRUCTION_H_
#define MYSHELL__INSTRUCTION_H_

#include "command.h"

void initCurrentInstruction();
void addCommandToCurrentInstruction(struct command *cmd);
int executeCurrentInstruction();
void resetCurrentInstruction();

#endif //MYSHELL__INSTRUCTION_H_
