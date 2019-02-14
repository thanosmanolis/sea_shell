# seaShell

## Description
This is a shell developed in C. 

There are two modes:

+ **Interactive mode**: User writes commands for execution, one by one.
+ **Batch mode**: User writes a batch file. Each line should be a command. Then the shell reads the file and executes the commands, line by line.

## Some general rules
1. You can write multiple commands, splitted by ";" or "&&".
2. Pipes and redirections are supported. Multiple pipes, and a combination of multiple pipes with a redirection in the end is supported, but multiple redirections are not.
3. Extra spaces and tabs do not really make any diference. 

## How to run it

#### Interactive mode
1. `> bin/seaShell` in order to initialize the shell
2. `> ... Insert random commands ...`
3. `> quit` in order to quit

#### Batch mode
1. Create a batch file and write commands in it
2. `> bin/seaShell *batchfile_name*` in order to initialize
3. The shell will execute the commands the file contains, and then terminate