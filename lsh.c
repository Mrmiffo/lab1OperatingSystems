/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
//Include pwd.h to get home dir
#include <pwd.h>
#include "parse.h"

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);
void execute(Pgm* pgm, int bg, int doPipe);
void signalHandler(int signalNumber);
void changeDirectory(char* arg);
/* When non-zero, this global means the user is done using this program. */
int done = 0;
int real_stdout;
int real_stdin;
/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;
  //Save STDIN/OUT
  real_stdin = dup(STDIN_FILENO);
  real_stdout = dup(STDOUT_FILENO);
  //Redirect signals to custom handler.
  signal(SIGCHLD, signalHandler);
  signal(SIGINT, signalHandler);
  while (!done) {
   
    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
        
		if (n==-1) {
			PrintCommand(n, &cmd);
		} else if (n==1){
			//PrintCommand(n, &cmd);
			// If cd or exit do something else 
			if(strcmp("cd",*cmd.pgm->pgmlist)==0)
			{
				changeDirectory(*(++cmd.pgm->pgmlist));
			} else if(strcmp("exit",*cmd.pgm->pgmlist)==0)
			{
				exit(0);
			} else 
			{
				//Execute command
				FILE *input = NULL;
				FILE *output= NULL;
				if(cmd.rstdin)
				{
					input = fopen(cmd.rstdin, "r");
					dup2(fileno(input), STDIN_FILENO );

				}
				if(cmd.rstdout) 
				{
					output = fopen(cmd.rstdout, "w");
					dup2(fileno(output), STDOUT_FILENO);

				}
				execute(cmd.pgm, cmd.bakground, 0);
				if(input)  fclose(input);
				if(output) fclose(output);
			}
		}	
	  }
    }
    
    if(line) {
      free(line);
    }
  }
  return 0;
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}

//Execute is called recursivly by to first execute the last pgmlist in a chain of pgm. 
void execute(Pgm* pgm, int bg, int doPipe)
{
	//If pgm is null then the prev pgm was the last pgm in the list. Return and start execute the prev pgm.
	if (pgm == NULL)
	{
		return;
	}
	//Recursivly call the next pgm until the last is found. If there are several pgm in the command all but the first should pipe.
	execute(pgm->next, bg, 1);

	//Setup the pipe variable but only create a pipe if doPipe is true.
	int pipefd[2];
	if(doPipe) pipe(pipefd);
	
	//Fork and execute
	switch(fork())
	{
	case -1:
	// ERROR
		perror("Fork failed");
	    break;
	case 0:
	// CHILD
	
	if(doPipe)
	{
		//The the information is suposed to be piped, channel STDOUT to the write end of the pipe.
		close(1);
		close(pipefd[0]);
		dup2(pipefd[1], 1);
		close(pipefd[1]);	
	}
	//Check if the process is supposed to run in background, if so decouple the child process from the main process tree.
	if(bg) {
		setsid();
	}
	//Execute the command. Using a exec with p to search the path was OK according to supervisor. (We originally searched path manually)
	execvp(*(pgm->pgmlist), pgm->pgmlist);
	//If exec fails, print error and exit the child process.
	perror("Execute failed");
	exit(EXIT_FAILURE);
	    break;
	default: 
	// PARENT
		if(!bg) 
		{
			//If not in background, wait for the child process to finish.
			wait(NULL);
		}
		if(doPipe)
		{
			//If the information is piped, channel STDIN to the pipe.
			close(pipefd[1]);
			dup2(pipefd[0], STDIN_FILENO);
			close(pipefd[0]);
						
		}
		else
		{
			//Reset STDIN/OUT to point to original input output incae they were changed by pipe.
			dup2(real_stdin, STDIN_FILENO );
			dup2(real_stdout, STDOUT_FILENO);
		}
	}

}
//Custom signal handler to that proccess SIGCHLD from background processes in order to avoid zombies.
void signalHandler(int signalNumber)
{
	switch(signalNumber)
	{
	case SIGCHLD:
		wait(NULL);
		break;
	default:
		//Do nothing on other signals. 
		//If SIGINT is received it will be ignored in order to not kill the shell. Although it will kill child processes not run in background as per default behavior of SIGINT.
		break;
	}
}

//Custom method to handle the 'cd' command. Takes the arguments of the command as input.
void changeDirectory(char* arg)
{
	if ('\0' == arg || '~' == *arg)
	{
		//If not argument or argument is ~ move to home dir.
		struct passwd *pw = getpwuid(getuid());
		chdir(pw->pw_dir);
	}
	else if(strcmp("..",arg) == 0)
	{
		//.. = move to prev directory
		//Extract the current directory
		char cwd[1024];
		getcwd(cwd, sizeof(cwd));
		//Find the last instance of '/'
		char *ptr = cwd;
		int last=0;
		while(*ptr!='\0')
		{
			if(*ptr == '/')
				last = ptr-cwd;
			ptr++;
		}
		//Replace the last instance of '/' with '\0' in order to shorten the path to the prev directory.
		cwd[last] = '\0';
		//Set the new dir
		chdir(cwd);

	}
	else if('/' == *arg)
	{
		//If the first char in the argument is / then an exact path is given from root. Simply set the new dir to that path.
		if (chdir(arg) < 0) perror("Change dir failed");
		
	}
	else
	{
		//Else the argument given is the folder to move into, concatenate the folder name to the current path and change dirr.
		char nPath[1024];
		getcwd(nPath, sizeof(nPath));
		strcat(nPath, "/");
		strcat(nPath, arg);
		if(chdir(nPath) < 0) perror("Change dir failed");
	}

}


