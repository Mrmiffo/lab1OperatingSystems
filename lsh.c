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
#include <readline/readline.h>
#include <readline/history.h>
//Needed for functions such as stat.
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
//END
#include "parse.h"

/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

/* When non-zero, this global means the user is done using this program. */
int done = 0;

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
			PrintCommand(n, &cmd);
			//Do good stuffz
			//Find the paths on the system
			char *path = getenv("PATH");
			char *savedPath = path;
			int stringStart = 0;
			int stringEnd = 0;
			int notFound = 1;
			char *binary = *(cmd.pgm->pgmlist);
			printf("%s \n", path);
			while (notFound){
				if (*savedPath == ':' || *savedPath == '\0'){
					//Create a new string to extract the path into.
					int stringLength = stringEnd-stringStart;
					char subString[stringLength+1];
					//Add the subPath into the new string.
					memcpy(subString, &path[stringStart],stringLength);
					subString[stringLength] = '\0';	
printf("%s \n",subString);
printf("%s \n", binary);
					//Create a new string for the path+binary.
					int newPathLength = strlen(binary)+strlen(subString);
					char newPath[newPathLength+2];
					//Create the path to the binary.
					strcpy(newPath,subString);
					strcat(newPath,"/");
					strcat(newPath,binary);
printf("%s \n",newPath);
					//Check if the binary exist in the path.
					notFound = access(newPath,F_OK);
					if (!notFound) {
						runProcess(newPath);
					}
					if (*savedPath == '\0') break;
					//Increase stringStart to indicate the next path in PATH.
					stringStart = 1+stringEnd;
				}
				stringEnd++;
				savedPath++;
			}
			if (notFound) printError();
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

void printError(){
	
}

void runProcess(char *path){
	int pid;
	if (pid = fork()){
		//Parent
		wait(NULL);
	} else {
		//Child
		execve(path,"\0");
	}
}