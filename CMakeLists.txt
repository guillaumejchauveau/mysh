cmake_minimum_required(VERSION 3.10)
project(MyShell C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(MyShell main.c utils.c command.c instruction.c instructionParser.c internalCommands.c)
target_link_libraries(MyShell readline)
