# yosh.c

## Summary

This program is essentially a rudimentary shell that contains many
of the basic necessary elements to create a shell. This includes
methods the jobs, exit, history, kill, cd, and help commands. This 
shell also supportspiping and input redirection for both built-in 
and external commands.

## Details

CODER: 

Nathan (Nathaniel) Koehler

CREDIT/Contributions:

Professor Hybinette and the CS 1730 team at UGA for Project 4, 
which was extensively used, modified, and integrated into parts 
of the code in yosh.c
The man page for wordexp, which provided instuctions as to how
it all 
https://linux.die.net/man/3/wordexp
The friends at Geeks for Geeks which I referenced for lists in yosh.c
through https://www.geeksforgeeks.org/getopt-function-in-c-to-parse-command-line-arguments/
The good people at gnu.org which also helped with testing file types: 
https://www.gnu.org/software/libc/manual/html_node/Testing-File-Access.html

## Compile The Project (After Forking)

How to compile:

### `gcc -g -Wno-parentheses -Wno-format-security -DHAVE_CONFIG_H -DRL_LIBRARY_VERSION='"8.1"' -I/home/myid/ingrid/usr/local/include -c yosh.c`
### `gcc -g -Wno-parentheses -Wno-format-security -DHAVE_CONFIG_H -DRL_LIBRARY_VERSION='"8.1"' -I/home/myid/ingrid/usr/local/include -c parse.c`
### `gcc -g -Wall -o yosh yosh.o parse.o /home/myid/ingrid/usr/local/lib/libreadline.a -ltermcap -lcurses`


OR use the included makefile:

### `make clean`
### `make`

How to run with gcc:

### `yosh` or `./yosh`

or with make (within the project folder):

### `yosh` or `./yosh`
