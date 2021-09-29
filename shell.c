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

enum BUILTIN_COMMANDS { NO_SUCH_BUILTIN=0, EXIT, JOBS, HISTORY, CD, KILL, HELP };


/* -----------------------------------------------------------------------------
FUNCTION: buildPrompt()
DESCRIPTION:
-------------------------------------------------------------------------------*/
char * buildPrompt()
{
  return  "{sysprogisgreat} ";
}
 
/* -----------------------------------------------------------------------------
FUNCTION: isBuild()
DESCRIPTION:
-------------------------------------------------------------------------------*/
int isBuiltInCommand( char * cmd )
{
	if( strncmp(cmd, "exit", strlen( "exit" ) ) == 0 )
	{
		return EXIT;
	}
	if( strncmp(cmd, "jobs", strlen( "jobs" ) ) == 0 )
	{
		return JOBS;
	}
	if( strncmp(cmd, "history", strlen( "history" ) ) == 0 )
	{
		return HISTORY;
	}
	if( strncmp(cmd, "cd", strlen( "cd" ) ) == 0 )
	{
		return CD;
	}
	if( strncmp(cmd, "kill", strlen( "kill" ) ) == 0 )
	{
		return KILL;
	}
	if( strncmp(cmd, "help", strlen( "help" ) ) == 0 )
	{
		return HELP;
	}
	return NO_SUCH_BUILTIN;
}




/* -----------------------------------------------------------------------------
FUNCTION: main()
DESCRIPTION:
-------------------------------------------------------------------------------*/
int main( int argc, char **argv )
{
  char * cmdLine;
  parseInfo *info; 		// info stores all the information returned by parser.
  struct commandType *com; 	// com stores command name and Arg list for one command.

  fprintf( stdout, "This is the SHELL version 0.1\n" ) ;


  while(1)
  {

    	// insert your code here

    	cmdLine = readline( buildPrompt() ) ;
    	if( cmdLine == NULL ) 
		{
      		fprintf(stderr, "Unable to read command\n");
      		continue;
    	}

    	// insert your code about history and !x !-x here

    	// calls the parser
    	info = parse( cmdLine );
        if( info == NULL )
		{	
      		free(cmdLine);
      		continue;
    	}

    	// prints the info struct
    	print_info( info );

    	//com contains the info. of the command before the first "|"
    	com = &info->CommArray[0];
    	if( (com == NULL)  || (com->command == NULL)) 
    	{
      		free_info(info);
      		free(cmdLine);
      		continue;
    	} 
		
		if ( isBuiltInCommand( com->command ) == EXIT ) //com->command tells the command name of com
		{
      		exit(1);
    	}

  //insert your code here / commands etc.



  free_info(info);

  free(cmdLine);

  }/* while(1) */



}
  





