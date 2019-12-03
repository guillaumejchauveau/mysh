add_executable(mysh
        main.c
        utils.c
        command.c
        instruction.c
        instruction_parser.c
        internal_commands.c)
target_link_libraries(mysh readline)
