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
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "parse.h"

#define TRUE 1
#define FALSE 0
int signit = 0;
void RunCommand(int, Command *);
void stripwhite(char *);
void Commands_Execution_Defined_cmd(char**,int);
void RunCommands(Pgm *, Command *);
void Commands_Execution(Pgm *,Command *);
void Commands_ExecutionIO(Pgm * ,Command *);
void parent_handler(int signal);

int main(void)
{
  //version Final
  Command cmd;
  int parse_result;

  while (TRUE)
  {
    char *line;
    line = readline("> "); 

    /* If EOF encountered, exit shell */
    if (!line)
    {
      break;
    }
    /* Remove leading and trailing whitespace from the line */
    stripwhite(line);
    /* If stripped line not blank */
    if (*line)
    {
      add_history(line);
      parse_result = parse(line, &cmd);
      RunCommand(parse_result, &cmd);
    }

    if (signal(SIGINT, &parent_handler) < 0) {      // this is used ensure the shell does not terminate with ctrl-c
    fprintf(stderr, "ERROR: could not register parent_handler (%d)\n", errno);
    }
    /* Clear memory */
    free(line);
  }
  return 0;
}

/* Execute the given command(s).

 * Note: The function currently only prints the command(s).
 * 
 * TODO: 
 * 1. Implement this function so that it executes the given command(s).
 * 2. Remove the debug printing before the final submission.
 */
void RunCommand(int parse_result, Command *cmd)
{
  RunCommands(cmd->pgm,cmd); // calling the function for executing the commands by passing the pointer to the first command in the linked list
}

void RunCommands(Pgm *p, Command *cmd){
  char **pl = p->pgmlist;     // obtaining the command in the first element of the list
  int Defined_Cmds = 2 ,switchDefinedArg = 0,CmdChecker= 0;
  char* RegisterOfCmds[Defined_Cmds];	 // Defining the list of user defined commands
  RegisterOfCmds[0] = "exit";
  RegisterOfCmds[1] = "cd";
  
  for (int i = 0; i < Defined_Cmds; i++) {      // iteration for the identification of user defined commands
     if (strcmp(pl[0], RegisterOfCmds[i]) == 0) {
         switchDefinedArg = i + 1;
         CmdChecker=1;
         break;
      }
  }  
 
  if(CmdChecker==1){         // function call for executing the user defined commands
       Commands_Execution_Defined_cmd(pl,switchDefinedArg);
  }
  if(!cmd->rstdin && !cmd->rstdout && CmdChecker!=1)  // function call for executing simple, piped or background commands
  {
  	int pid=fork();
  	int stat=0;
  	if(pid==0){
  	Commands_Execution(cmd->pgm,cmd);
  	}
  	else
  	{
  		if(!cmd->background){
  			waitpid(pid,&stat,0);
  		}
  		else{
  	 		signal(SIGCHLD,SIG_IGN);
  	 	}
  	 }
  }
  if(cmd->rstdin || cmd ->rstdout){       // function call for executing IO redirection commands
  	Commands_ExecutionIO(cmd ->pgm ,cmd);
  } 
  
}

void Commands_Execution(Pgm * p, Command *cmd)  // function for executing simple, piped and backgroundcommands
{
  char **pl = p->pgmlist;
  int pd[2];
  if (p->next!=NULL){
    pipe(pd);
    int cpid = fork();
    if (cpid == -1) perror("fork");

    if (cpid == 0){ // Child command
     close(pd[0]);
     dup2(pd[1], STDOUT_FILENO);
     close(pd[1]);
     Commands_Execution(p->next,cmd);

    }else{ //Parent command
     close(pd[1]);
     dup2(pd[0], STDIN_FILENO);
     close(pd[0]);
     if(execvp(pl[0], pl) < 0) {
     perror("Invalid command\n");
     exit(0);
     }
    }
     
  }else{
  if(cmd->background){    // checking for background processes and ignoring the its termination with ctrl-c
  	signal(SIGINT,SIG_IGN);
  	}
    if (execvp(pl[0], pl) < 0) {
     perror("Invalid command\n");
     exit(0);
       }     
  }
}

void Commands_Execution_Defined_cmd(char** parsed, int switchDefinedArg){   // function for user defined commands
switch (switchDefinedArg) {
    case 1:
        exit(0);
    case 2:
    	if(parsed[1]==NULL) {
    	chdir( getenv("HOME"));
        	
        }
        else {
        if(chdir(parsed[1])!=0)
        printf("lsh: %s: %s: No such file or directory\n", parsed[0],parsed[1]);
        }
        break;
    default:
        break;
    }
}
void Commands_ExecutionIO(Pgm * p,Command *cmd){   //function for IO commands
char **pl = p->pgmlist;
    // Forking a child
    int pd[2];
    pipe(pd);
    pid_t pid = fork();    
    if (pid == -1) {
        printf("Failed to fork child");
        return;
    } else if (pid == 0) {
    	if (cmd->rstdout != NULL) { //if the user-inputted string contained '>'
    		close(pd[0]);
   		pd[1]=open(cmd->rstdout, O_WRONLY | O_CREAT, 0777);
   		if(pd[1] == -1){
   			return;
  	 	}
   		dup2(pd[1], STDOUT_FILENO);
   		close(pd[1]);
   	}
    	if(p->next){ 
   	  Commands_ExecutionIO(p ->next,cmd);
   	}
    	if (cmd->rstdin != NULL) { //if the user-inputted string contained '<'
    		close(pd[1]);
        	pd[0] = open(cmd->rstdin, O_RDONLY, 0);
        	if(pd[0] == -1){
   			return;
  	 	}
        	dup2(pd[0], STDIN_FILENO);
        	close(pd[0]);
    	}
    	if (execvp(pl[0], pl) < 0) { 	    
            printf("\nCould not execute command..");       
        }
        exit(0);
    } else {
    	 wait(NULL);
	 return;
    }
}
void parent_handler(int signal) 
{
  // Stop parent from being terminated by SIGINT
  printf("\n>"); 
}

/* Strip whitespace from the start and end of a string. 
 *
 * Helper function, no need to change.
 */
void stripwhite(char *string)
{
  register int i = 0;

  while (isspace(string[i]))
  {
    i++;
  }

  if (i)
  {
    memmove(string, string + i, strlen(string + i) + 1);
  }

  i = strlen(string) - 1;
  while (i > 0 && isspace(string[i]))
  {
    i--;
  }

  string[++i] = '\0';
}
