/* -----------------------------------------------------------------------------
FILE: yosh.c
 
NAME: Nathaniel Koehler

DESCRIPTION: A shell that essentially contains many of the basic necessary
elements. This includes the jobs, exit, history, kill, cd, and help commands.
This shell also supports 
-------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h" // local file include declarations for parse-related structs
#include <wait.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wordexp.h>
#include <limits.h>
#include <signal.h>

enum BUILTIN_COMMANDS
{
	NO_SUCH_BUILTIN = 0,
	EXIT,
	JOBS,
	HISTORY,
	KILL,
	CD,
	HELP
};

struct job { // an object to hold all the relevant information to create a linked list and job storage
	int num;
	char *command;
	pid_t pid;
	int mode;
	struct job *nextjob;
};

struct job *head; // the very start of the jobs linked list
int modHistory = 0;

/* -----------------------------------------------------------------------------
FUNCTION: buildPrompt()
DESCRIPTION:
-------------------------------------------------------------------------------*/
char *buildPrompt() {
	char * prompt = (char * ) malloc(PATH_MAX);
	char cwd[PATH_MAX];
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		strcpy(prompt, "{yosh}:");
		strcat(prompt, cwd);
		strcat(prompt, "$ ");
		return prompt;
	} else {
		perror("getcwd() error");
		exit(1);
	}
	
	
	return prompt;
}

/* -----------------------------------------------------------------------------
FUNCTION: redirectionTester(parseInfo *info)
DESCRIPTION: returns a new input file descriptor for input redirection or 0
if there is either valid input or output redirection or no redirection taking
place. This will return 0 if there is no input redirection. 
-------------------------------------------------------------------------------*/
int redirectionTester(parseInfo *info) {
	wordexp_t p;
	int potentialInput = 0;
	if (info->boolInfile) {
		if (info->boolInfile > 1) {
			fprintf(stderr, "Ambiguous input redirect.\n");
			return -1;
		}
		if (info->inFile[0] != '\0') {
			if (strstr(info->inFile, "~")) {
				wordexp(info->inFile, &p, 0);
				strcpy(info->inFile, p.we_wordv[0]);
				wordfree(&p);
			}
			if( access(info->inFile, F_OK ) != -1 ) {
				int fd = open(info->inFile, O_RDONLY, 0644);
				if (fd == -1) {
					perror("open");
					return -1;
				}
				dup2(fd, 0);
				potentialInput = fd;
				
			} else {
				fprintf(stderr, "%s: No such file or directory.\n", info->inFile);
				return -1;
			}
		} else {
			fprintf(stderr, "Missing name for redirect.\n");
			return -1;
		}
	}
	if (info->boolOutfile) {
		if (info->boolOutfile > 1) {
			fprintf(stderr, "Ambiguous output redirect.\n");
			return -1;
		}
		if (info->outFile[0] != '\0') {
			if (strstr(info->outFile, "~")) {
				wordexp(info->outFile, &p, 0);
				strcpy(info->outFile, p.we_wordv[0]);
				wordfree(&p);
			}
			if( access(info->outFile, F_OK ) == -1 ) {
				int fd = open(info->outFile, O_RDWR|O_CREAT|O_APPEND, 0644);
				if (fd == -1) {
					perror("open");
					return -1;
				}
				dup2(fd, 1);
			} else {
				fprintf(stderr, "%s: File exists.\n", info->outFile);
				return -1;
			}
		} else {
			fprintf(stderr, "Missing name for redirect.\n");
			return -1;
		}
	}
	return potentialInput;
}

/* -----------------------------------------------------------------------------
FUNCTION: jobID(int getID)
DESCRIPTION: returns a job based on it's numerical ID. This assumes there exists
a job with an ID of the parameter getID. This returns the job with the ID of said
integer paramter, and returns NULL if that is not found before the loop 
terminates by reaching NULL through assigning temp to its own nextjob.
-------------------------------------------------------------------------------*/
struct job *jobID(int getID) {
	struct job *temp = (struct job*) malloc(sizeof(struct job)); // creates a temporary object to loop until the ID is found
	for (temp = head; temp != NULL; temp = temp->nextjob) {
		if (getID == temp->num) {
			return temp;
		}
	}
	return NULL;
}

/* -----------------------------------------------------------------------------
FUNCTION: isBuild()
DESCRIPTION:
-------------------------------------------------------------------------------*/
int isBuiltInCommand(char *cmd)
{
	if (strncmp(cmd, "exit", strlen("exit")) == 0) {
		return EXIT;
	}
	if (strncmp(cmd, "history", strlen("history")) == 0) {
		return HISTORY;
	}
	if (strncmp(cmd, "cd", strlen("cd")) == 0) {
		return CD;
	}
	if (strncmp(cmd, "help", strlen("help")) == 0) {
		return HELP;
	}
	if (strncmp(cmd, "jobs", strlen("jobs")) == 0) {
		return JOBS;
	}
	if (strncmp(cmd, "kill", strlen("kill")) == 0) {
		return KILL;
	}
	return NO_SUCH_BUILTIN;
}

/* -----------------------------------------------------------------------------
FUNCTION:int executeBuiltInCommand(char *command, char **argv) {
DESCRIPTION:
-------------------------------------------------------------------------------*/
int executeBuiltInCommand(char *command, char ** argv, int out) {
	if (isBuiltInCommand(command) == HISTORY) { // the history command
		
		int count = 0, start = 0;
		while(argv[++count] != NULL);
		if (count == 3) {
			if (strcmp(argv[1],"-s") == 0) {
				int buffsize = atoi(argv[2]);
				if (buffsize < 1) {
					fprintf (stderr, "History buffer must be an integer of at least 1\n");
				} else {
					modHistory = buffsize;
				}
			} else {
				fprintf (stderr, "history %s: Unknown arguments for history: %s\n", argv[1], argv[1]);
			}
			if (out != 0) { // breaks out if pipes are involved
				exit(0); // exits and kills the child process
			}
			return 0;
		} else if (count == 2) {
			start = -1;
			
		}

		register HIST_ENTRY ** hist_list;
		register int i;
		time_t tt;
		FILE* fp; // only useful when built-in commands are being piped
		char timestr[128];

		hist_list = history_list();
		if (hist_list) {
			if (out != 0)
				fp = fdopen(out, "w"); // creates a file stream if a pipe exists and history is to redirect it's own output
			
			if (start == -1) {
				int j = 0;
				for (i = 0; hist_list[i]; i++) { 
					j++;
				}
				start = j - atoi(argv[1]);
				if (start < 0) {
					start = 0;
				}
			}

			for (i = start; hist_list[i]; i++) {
				tt = history_get_time (hist_list[i]);

				if (tt)
					strftime (timestr, sizeof (timestr), "%a %R", localtime(&tt));
				else
					strcpy (timestr, "??");

				if (out != 0) { // occurs if the intended output is not standard output
					fprintf (fp, "%.4d: %s: %s\n", i + history_base, timestr, hist_list[i]->line);
				} else { // occurs when the intended output is standard output
					printf ("%.4d: %s: %s\n", i + history_base, timestr, hist_list[i]->line);
				}
			}
			if (out != 0) { // breaks out if pipes are involved
				exit(0);
			}
			return 0;
		}
	} else if (isBuiltInCommand(command) == CD) { // the cd command
		int count = 0; 
		while(argv[++count] != NULL);
		if (count < 2) {
			fprintf (stderr, "Usage: CD destination\n");
			if (out != 0) { // breaks out if pipes are involved
				exit(0); // exits and kills the child process
			}
			return 0;
		}
		wordexp_t result;
		char * loc = argv[1];
		
		if (loc == NULL) {
			loc = "~";
		}
		wordexp(loc, &result, 0);
		char ** words = result.we_wordv;
		chdir(words[0]);
		wordfree(&result);
		
		return 0;

	} else if (isBuiltInCommand(command) == HELP) { // the help command
		if (out != 0) {
			FILE* fp; // only useful when built-in commands are being piped
			fp = fdopen(out, "w"); // creates a file stream if a pipe exists and help is to redirect it's own output
			fprintf (fp, "jobs\t\t\t\t\t\t\t\ttDisplays a list of background jobs\n");
			fprintf (fp, "cd [directory name]\t\t\t\t\t\tMoves into a [directory name], if it exists\n");
			fprintf (fp, "history [OPTIONAL -s num or OPTIONAL num]\t\t\tdisplays the command history. -s num sets the history buffer. num lists num elements\n");
			fprintf (fp, "exit\t\t\t\t\t\t\t\texits out if there are no background commands running\n");
			fprintf (fp, "kill [num or %%num]\t\t\t\t\t\tkills the process with pid num or job %%num\n");
			fprintf (fp, "help\t\t\t\t\t\t\t\tthis! displays a list of commands\n");
			exit(0); // breaks out if pipes are involved
		}
		printf("jobs\t\t\t\t\t\t\t\tDisplays a list of background jobs\n");
		printf("cd [directory name]\t\t\t\t\t\tMoves into a [directory name], if it exists\n");
		printf("history [OPTIONAL -s num or OPTIONAL num]\t\t\tdisplays the command history. -s num sets the history buffer. num lists num elements\n");
		printf("exit\t\t\t\t\t\t\t\texits out if there are no background commands running\n");
		printf("kill [num or %%num]\t\t\t\t\t\tkills the process with pid num or job %%num\n");
		printf("help\t\t\t\t\t\t\t\tthis! displays a list of commands\n");
		return 0;
	} else if (isBuiltInCommand(command) == JOBS) { // the jobs command
		if (head != NULL) {
			FILE* fp; // only useful when built-in commands are being piped
			if (out != 0) {
				fp = fdopen(out, "w"); // creates a file stream if a pipe exists and help is to redirect it's own output
			}
			char *mode = (char *) malloc(sizeof(char) * 11);
			struct job *jobstruct = (struct job*) malloc(sizeof(struct job));
			int i = 1;
			for (jobstruct = head; jobstruct != NULL; jobstruct = jobstruct->nextjob) {
				if (jobstruct->mode == 3) {
					if (jobstruct != head) {
						if (jobstruct->nextjob != NULL) {
							jobID(i - 1)->nextjob = jobstruct->nextjob;
						} else {
							jobID(i - 1)->nextjob = NULL;
							free(jobstruct);
						}
					} else { // when the current job is equal to head
						if (jobstruct->nextjob != NULL) {
							head = jobstruct->nextjob;
							free(jobstruct);
						} else {
							free(jobstruct);
							head = NULL;
							break;
						}
					}
					continue;
				}
				else if (jobstruct->mode == 2) {
					strcpy(mode, "Terminated");
					jobstruct->mode = 3;
				} else {
					if (kill(jobstruct->pid, 0) == 0) {
						strcpy(mode, "Running");
					}
					else {
						strcpy(mode, "Done");
						jobstruct->mode = 3;
					}
				}
				jobstruct->num = i; // sets the id of jobstruct
				i++; // incriments here to prevent cleared jobs from interfering
				//free(jobstruct);
				if (out != 0) { // breaks out if pipes are involved
					fprintf (fp, "[%d]\t%d\t%s\t%s\n", jobstruct->num, jobstruct->pid, mode, jobstruct->command);
				} else {
					printf("[%d]\t%d\t%s\t%s\n", jobstruct->num, jobstruct->pid, mode, jobstruct->command);
				}
			}
		}
		if (out != 0) { // breaks out if pipes are involved
			exit(0); // exits and kills the child process
		}
		return 0;
	} else if (isBuiltInCommand(command) == KILL) { // the kill command
		int count = 0; 
		while(argv[++count] != NULL);
		if (count < 2) {
			fprintf (stderr, "Usage: Kill %%number\n");
			if (out != 0) { // breaks out if pipes are involved
				exit(0); // exits and kills the child process
			}
			return 0;
		}
		if (strlen(argv[1]) < 1) {
			fprintf (stderr, "Usage: Kill %%number\n");
			if (out != 0) { // breaks out if pipes are involved
				exit(0); // exits and kills the child process
			}
			return 0;
		}
		struct job *tempjob = (struct job*) malloc(sizeof(struct job)); // tempjob for finding size
		if (argv[1][0] == '%') {
			int id = atoi(argv[1] + 1);
			tempjob = jobID(id);
		} else { 
			int pid = atoi(argv[1]);
			for (tempjob = head; tempjob != NULL; tempjob = tempjob->nextjob) {
				if (pid == tempjob->pid) {
					break;
				}
			}
		}
		if (tempjob == NULL) {
			fprintf (stderr, "yosh: kill: (%s) - No such process\n", argv[1]);
		} else {
			if (kill(tempjob->pid, SIGKILL) < 0) {
				perror("kill");
			} else {
				tempjob->mode = 2;
				if (out != 0) {
					FILE* fp; // only useful when built-in commands are being piped
					fp = fdopen(out, "w"); // creates a file stream if a pipe exists and help is to redirect it's own output
					fprintf(fp, "Process killed: %d\n", tempjob->pid);
				}
				printf("Process killed: %d\n", tempjob->pid);
			}
		}
		if (out != 0) { // breaks out if pipes are involved
			exit(0); // exits and kills the child process
		}
		return 0;
 	} else if (isBuiltInCommand(command) == EXIT) { // the exit command
	 	if (head != NULL) {
			 struct job *jobstruct = (struct job*) malloc(sizeof(struct job));
			for (jobstruct = head; jobstruct != NULL; jobstruct = jobstruct->nextjob) {
				if (kill(jobstruct->pid, 0) == 0) {
					if (out != 0) {
						FILE* fp; // only useful when built-in commands are being piped
						fp = fdopen(out, "w"); // creates a file stream if a pipe exists and help is to redirect it's own output
						fprintf(fp, "There are still jobs running!\n");
						return 0;
					}
					printf("There are still jobs running!\n");
					return 0;
				}
			}
		}
		exit(0); // exits and kills the child process
	}
	return 0;
}

/* -----------------------------------------------------------------------------
FUNCTION: void executeCommand(char *command, char **argv) {
DESCRIPTION: The executeCommand is designed to differentiate built-in commands
and external commands, and is called exclusively by the main method. The
context in which it is called is the most important part of this command, as it
is designed to only be called within the child process of a fork, therefore must
terminate itself eventually. Therefore this command does not return anything,
except when an error occurs with an external or built-in command. The char
pointer command refers to the command in question, while the char pointer pointer
refers to the arguements of the command.
-------------------------------------------------------------------------------*/
int executeCommand(char *command, char **argv) {
	if (isBuiltInCommand(command)) {
		return executeBuiltInCommand(command, argv, 1);
	}
	return execvp(command, argv);
}

void handle_sigchld( int s )
{
    /* execute non-blocking waitpid, loop because we may only receive
     * a single signal if multiple processes exit around the same time.
     */
    while( waitpid(0, NULL, WNOHANG) > 0) ;
}

/* -----------------------------------------------------------------------------
FUNCTION: int pipingHandler(char ** argv , int in, int out) {
DESCRIPTION: Handles the creation of pipes that are designed to allow for
input and output redirection of a potentially infinite number of commands tied
together, the first of which is likely standard input and the last of which
is likely standard output. The char pointer pointer argv is the command and all 
its arguements. The integer in is a file pointer to the intended input. The
integer out is a file pointer to the intended output. This method forks, and the
child process terminates itself using the methods execvp or 
executeBuiltInCommand. The parent returns the pid of the parent process.
-------------------------------------------------------------------------------*/
int pipingHandler(char ** argv , int in, int out) {
	int pid;
	if ((pid = fork()) == 0) { // forks for piping
		
		if (in != 0) { // only occurs if the start of the pipe is standard input
			dup2(in, STDIN_FILENO); // makes sure standard input is sent to fds[0] or the start of the pipe
			close(in);
		}

		if (out != 1) { // occurs if the end of the pipe is standard output
			dup2(out, STDOUT_FILENO); // makes sure standard output is sent to fds[1] or the end of the pipe
			close(out);
		}
		if (!isBuiltInCommand(argv[0])) {
			return execvp(argv[0], argv); // runs execvp for non-built in commands
		} else { // occurs when the command is not built in
			return executeBuiltInCommand(argv[0], argv, 1);
		}
	}
	waitpid(pid, NULL, 0);
	return pid;
}

/* -----------------------------------------------------------------------------
FUNCTION: main()
DESCRIPTION: the main command of the terminal -- initialization is contained
within this command as well and argv should be empty and argc should be 1. There
should be no command-line arguments for this shell.
-------------------------------------------------------------------------------*/
int main(int argc, char **argv) {
	parseInfo *info;		 // info stores all the information returned by parser.
	struct commandType *com; // com stores command name and Arg list for one command.
	
	head = NULL;
	
	char *cmdLine;
	pid_t childPid;
	int status; // A pointer to the location where status information for the terminating process is to be stored

	int fds[2];
	int new_desc;

	fprintf(stdout, "This is the YOSH version 0.1\n");

	signal(SIGCHLD, handle_sigchld);

	while (1) {
		// insert your code here

		cmdLine = readline(buildPrompt());
		if (cmdLine == NULL) {
			fprintf(stderr, "Unable to read command\n");
			continue;
		}

		// insert your code about history and !x !-x here
		char *buffer;
		using_history();
		if (modHistory != 0) {
			stifle_history(modHistory);
		} else {
			stifle_history(10);
		}
		int result = history_expand(cmdLine, &buffer);
		add_history(buffer);
		strncpy(cmdLine, buffer, sizeof(cmdLine) - 1);
		
		// calls the parser
		info = parse(cmdLine);
		if (info == NULL) {
			free(cmdLine);
			continue;
		}

		if (result)
	    	fprintf (stderr, "%s\n", buffer);

		free(buffer);


		// prints the info struct
		//print_info(info);

		com = &info->CommArray[0];

		//insert your code here / commands etc.
		int i, j;
		wordexp_t p;
		if (com->VarNum > 1) {
			for (i = 1; i < com->VarNum; i++) {
				if (strstr(com->VarList[i], "~")) {
					wordexp(com->VarList[i], &p, 0);
					com->VarList[i] = strdup(p.we_wordv[0]);
					wordfree(&p);
				}
			}
		}
		
		for (i = 0; i <= info->pipeNum; i++) { // removes all '' and "" for things like grep 'txt$' and grep "txt$"
			int max = (&(info->CommArray[i]))->VarNum; // the number of arguments for each command in the loop
			for (j = 0; j < max; j++) {
				if (((&(info->CommArray[i]))->VarList[j][0] == '\'' && 
				(&(info->CommArray[i]))->VarList[j][strlen((&(info->CommArray[i]))->VarList[j]-1)] == '\'') ||
				((&(info->CommArray[i]))->VarList[j][0] == '"' && 
				(&(info->CommArray[i]))->VarList[j][strlen((&(info->CommArray[i]))->VarList[j]-1)] == '"')) {
					memmove((&(info->CommArray[i]))->VarList[j], (&(info->CommArray[i]))->VarList[j]+1, strlen((&(info->CommArray[i]))->VarList[j]));
					(&(info->CommArray[i]))->VarList[j][strlen((&(info->CommArray[i]))->VarList[j])-1] = '\0';
				}
			}
		} 

		//com contains the info. of the command before the first "|"
		
		if ((com == NULL) || (com->command == NULL)) {
			free_info(info);
			free(cmdLine);
			continue;
		}
			
		//com->command tells the command name of com
		else if (info->pipeNum == 0 && isBuiltInCommand(com->command)) {
			int childNeeded = 0;
			if (info->boolInfile || info->boolOutfile) {
				childNeeded = 1; // determines if a fork is needed for input redir purposes
			}
			if (childNeeded == 1) {
				childPid = fork(); // creates the fork to allow dup2 to function properly
				if (childPid == 0) {
					if (redirectionTester(info) == -1) { // tests and implements input redirection
						continue; // exits preemptively before commands are run
					}
					executeBuiltInCommand(com->command, com->VarList, 1); //calls execvp
				}
				waitpid(childPid, NULL, 0);
			} else {
				executeBuiltInCommand(com->command, com->VarList, 0); //calls execvp
			}
		} else {
			childPid = fork();
			if (childPid == 0) {
				int potentialInput = redirectionTester(info); // retrieves potential input redirection
				if (potentialInput == -1) { // tests and implements input redirection
					exit(0); // exits preemptively before commands are run
				}
				new_desc = 0; // sets the starting input to standard input or potential input redirection
				if (info->pipeNum > 0) {
					int i;
					for (i=0; i < info->pipeNum; ++i) {
						pipe(fds);
						pipingHandler((&(info->CommArray[i]))->VarList, new_desc, fds[1]);
						close(fds[1]);
						new_desc = fds[0];
					}
					if (new_desc != 0)
						dup2(new_desc, 0);
					executeCommand((&(info->CommArray[i]))->command, (&(info->CommArray[i]))->VarList); //calls execvp
				} else {
					executeCommand(com->command, com->VarList); //calls execvp
				}
			} else {
				if (info->boolBackground)  {
					int i = 0;
					int numjobs = 1;
					char *fullcommand = (char *) malloc(NAME_MAX);
					struct job *newjob = (struct job*) malloc(sizeof(struct job)); // creates a job object newjob
					strcpy(fullcommand, "");
					while(com->VarList[i] != NULL) {
						strcat(fullcommand, com->VarList[i]);
						strcat(fullcommand, " ");
						i++;
					}
					newjob->command = fullcommand; // stores the values to the new object
					
					newjob->pid = childPid;
					newjob->mode = 0;
					newjob->nextjob = NULL;
					if (head != NULL) { // determines if there are any jobs running
						struct job *tempjob = (struct job*) malloc(sizeof(struct job)); // tempjob for finding size
						for (tempjob = head; tempjob != NULL; tempjob = tempjob->nextjob) {
							numjobs++; // finds the current jobID
							if (tempjob->nextjob == NULL) {
								tempjob->nextjob = newjob;
								break;
							}
						}
						newjob->num = numjobs;
					} else { // sets head to the only job
						newjob->num = 1;
						head = newjob;
						continue;
					}
					continue;
				} else {
					waitpid(childPid, &status, 0);
					if (WIFSIGNALED(status)){
						fprintf(stderr, "Error\n");
					}
				}
			}
		}
		free_info(info);
		free(cmdLine);

	} /* while(1) */
}
