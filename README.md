# seaShell

## Description
This is a shell developed in C. Pipes and redirections are supported. There are two ways to run it.

### Interactive mode
User writes commands for execution, one by one.

### Batch mode
User writes a batch file. Each line should be a command. Then the shell executes the commands, line by line.

## How to run it

While into the local repo directory, type make in order to get an executable.

### Interactive mode
1. `>./seaShell` in order to initialize the shell
2. ... Write commands ...
3. `>quit` in order to quit

### Batch mode
1. Create a file and write commands in it
2. `>./seaShell *batchfile_name*` in order to initialize
3. The shell will execute the commands the file contains, and then terminate 