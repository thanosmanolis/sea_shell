/*
*****************************************
*    Custom shell for linux terminal    *  
*****************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

//! Max array sizes
#define MAX_INPUT_LENGTH	512	// max number of characters to be supported 
#define MAX_CMD_NUM			32	// max number of commands to be supported 
#define MAX_ARG_NUM     	16	// max number of arguments to be supported 
#define MAX_ARG_LENGTH		8	// max length of each argument to be supported 
#define MAX_CMD_LENGTH		128	// max length of each command to be supported 

//! Clearing the shell using escape sequences 
#define clear() printf("\033[H\033[J") 
#define GREEN_BOLD "\033[1;32m"
#define BLUE "\033[0;34m"
#define RESET_COLOR "\033[0m"

//! All functions used
void interactive_mode	  (void);
void batch_mode			  (char **argv);
void control_unit		  (char *input);
void check_delims         (char *input, int *delims, int *wrong_delims);
void replace_wrong_delims (char *input, int *wrong_delims);
int  parse_line			  (char **commands, char *input);
int  *parse_command		  (char *command, char **args);
int  exec_command		  (char **args);
int  exec_redirect		  (char **args, int index);
int  exec_pipe			  (char **args, int index);

/*
*********************************************************
*    Check if we are at interactive or at batch mode    *  
*********************************************************
*/
int main(int argc, char **argv)
{
	clear();

	if(argc == 1)
		interactive_mode();
	else if(argc == 2)
		batch_mode(argv);
	else
		exit(EXIT_FAILURE);
}

/*
********************************************************************************
*    While at interactive mode, read the input. Exit the shell if the input    *
*    is "quit" or continue to the next one if it's empty. Else, pass it to     *
*    the main control unit, where the execution is done.                       *
********************************************************************************
*/

void interactive_mode(void)
{
	printf(BLUE "****************************************\n"
		   		"**                                    **\n"
		   		"**    This is a custom shell in c.    **\n"
		   		"**                                    **\n"
		   		"****************************************\n\n" RESET_COLOR);

	char *input_str = (char *)malloc(MAX_INPUT_LENGTH*sizeof(char));

	while(1)
	{
		printf(GREEN_BOLD "manolis_8856> " RESET_COLOR);
		fgets(input_str, MAX_INPUT_LENGTH, stdin); 

		if(strcmp(input_str, "quit\n") == 0)
			break;
		else if(strcmp(input_str, "\n") == 0)
			continue;

		control_unit(input_str);	
	}

	printf("\n¯\\_(ツ)_/¯\n\n");
	free(input_str);
}

/*
**************************************************************************************
*    While at batch mode, read the batch file line by line. Exit the shell if the    *
*    input is "quit" or continue to the next one if it's empty. Else, pass it to     *
*    the main control unit, where the execution is done.                             *    
**************************************************************************************
*/

void batch_mode(char **argv)
{
	printf(BLUE "***********************************************\n"
		   		"**                                           **\n"
		   		"**    This is the output of a batch file.    **\n"
		   		"**                                           **\n"
		   		"***********************************************\n\n" RESET_COLOR);

	size_t len = MAX_INPUT_LENGTH;
	char *input_str = (char *)malloc(len*sizeof(char));
	char **batch_inputs = (char **)malloc(len*sizeof(char *));
	char file_name[50];
	FILE *fp;

	strcpy(file_name, argv[1]);

	fp = fopen(file_name, "r");

	if(fp == NULL)
	{
		perror("Error while opening the file.\n");
		exit(EXIT_FAILURE);
	}	

	//! Store each line to the batch_inputs list
	int i = 0;
	while(getline(&input_str, &len, fp) != -1) 
	{		
		batch_inputs[i++] = strdup(input_str);
    }

    //! Pass each command to the main control unit
    for(int j=0; j<i; j++)
    {
    	if((!strncmp(batch_inputs[j], "\n", 4)))
			continue;
		else
		{
			printf(GREEN_BOLD "Command: " RESET_COLOR "%s", batch_inputs[j]);

			if(!strncmp(batch_inputs[j], "quit\n", 4))
				break;

			control_unit(batch_inputs[j]);
			printf("\n");
		}
    }

    //! Quiting ... Either by "quit" command or by no more given commands
    printf("\n¯\\_(ツ)_/¯\n\n");
	free(batch_inputs);
	free(input_str);
	fclose(fp);
}

/*
***************************************
*    This is the main control unit    *  
***************************************
*/

void control_unit(char *input)
{
	char **commands 	= (char **)calloc(MAX_CMD_NUM, sizeof(char *));	//! List with all the commands (splitted by ";" or "&&")
	char  *command 		= (char *)malloc(MAX_CMD_LENGTH*sizeof(char));	//! Array for each command
	char **args 		= (char **)malloc(MAX_ARG_NUM*sizeof(char *)); 	//! List for the the arguments of each command (splitted by spaces)
	int   *delims 		= (int *)malloc(MAX_ARG_NUM*sizeof(int)); 		//! Array for delimeters (";" or "&&")
	int   *wrong_delims = (int *)malloc(MAX_CMD_LENGTH*sizeof(int)); 	//! Array for the index of inputs changes from "&" or ";" to "x"
	
	check_delims(input, delims, wrong_delims);	   //! Fill the array with the delimeters. Replace "&" with "x" and ";" with "y" 
	if(parse_line(commands, input))				   //! Split input if delimeters exist
    	replace_wrong_delims(input, wrong_delims); //! Replace "x" with "&" and "y" with ";"		    
	
	int i = 0;
	int *index, status;
	do
	{
		int redirection = 0;
		int pipe = 0;

		strcpy(command,commands[i++]); //! Split each command if spaces exist

		index = parse_command(command, args); //! Get the index and type of the redirection, or pipe 
											  //! (if neither of them exists, its value is -1)
		
		//check if pipe or redirection exists
		if(index[0] == 0)
			redirection = 1;
		else if(index[0] == 1)
			pipe = 1;

		if(redirection == 1)
		{
			status = exec_redirect(args, index[1]);
	    }else if(pipe == 1)
	    {
	    	status = exec_pipe(args, index[1]);
	    }else
		{
			status = exec_command(args);
		}
	}while(commands[i] && (delims[i-1] == 2 || status == EXIT_SUCCESS)); //! Break ony if an argument is wrong, and the next delimeter is "&&"

	free(args);
	free(commands);
	free(command);
	free(delims);
}

/*
***************************************************************************************
*    Create an array with the right delimeters. Right delimeters are the ones that    * 
*	 are a single ";" or two "&". If a wrong delimeter exists, change its value to    *
*	 "x" if it's "&", or to "y" if it's ";" (later they'll be replaced back).         *
***************************************************************************************
*/

void check_delims(char *input, int *delims, int *wrong_delims)
{
	int j = 0;
	int y = 0;

	for(int i=0; i<strlen(input)-1; i++)
	{
		int index = 0;

		if(input[i] == '&')
		{
			while(input[i+1] == '&')
			{
				i++;
				index++;
			}

			if(index == 1)
			{
				delims[j] = 1;
				j++;
			}
			else
			{
				for(int x=i-index; x<i+1; x++)
				{
					input[x] = 'x';
					wrong_delims[y++] = x;
				}
			}			
		}
		else if(input[i] == ';')
		{
			while(input[i+1] == ';')
			{
				i++;
				index++;
			}

			if(index == 0)
			{
				delims[j] = 2;
				j++;
			}
			else
			{
				for(int x=i-index; x<i+1; x++)
				{
					input[x] = 'y';
					wrong_delims[y++] = x;
				}
			}
		}
	}	
}		

/*
********************************************************************
*    Replace back all the "x" with "&" and all tge "y" with ";"    *
********************************************************************
*/

void replace_wrong_delims(char *input, int *wrong_delims)
{
	for(int z=0; z<sizeof(wrong_delims); z++)
	{
		if(input[wrong_delims[z]] == 'x')
			input[wrong_delims[z]] = '&';
		else if(input[wrong_delims[z]] == 'y')
			input[wrong_delims[z]] = ';';
	}
}

/*
****************************************************************
*    Get the whole input and split it if ";" or "&&" exists    *  
****************************************************************
*/

int parse_line(char **commands, char *input)
{
	const char delim[] = ";&\n";

	commands[0] = strtok(input, delim);

	int i = 1;
	do
	{ 
		if ((commands[i++] = strtok(NULL, delim)) == NULL) 
			break;
	}while(i < MAX_CMD_NUM);

	return 1;
}

/*
*******************************************************
*    Get each command and split it if space exists    *  
*******************************************************
*/

int *parse_command(char *command, char **args)
{
	const char delim[] = " \n";
	char *token;
	int i = 0;
	
	int *index = malloc(2*sizeof(int)); //! The first element represents the position, and the second one represents the type 
	index[0] = -1;						//! -1: no pipe or redirection exists
	index[1] = -1;						//!  0: redirection exists
										//!  1: pipe exists				

	token = strtok(command, delim);

	do
	{ 
		args[i++] = token;

		if((i == MAX_ARG_NUM-1)	|| (args[i-1] == '\0'))
			break;
		
		//check if redirection exists
		if((args[i-1][0] == '<') || (args[i-1][0] == '>'))
		{
			index[0] = 0;
			index[1] = i-1;
		}

		//check if pipe exists
		if(args[i-1][0] == '|')
		{
			index[0] = 1;
			index[1] = i-1;
		}

		token = strtok(NULL, delim); 
	}while(token != NULL);

	args[i] = NULL;

	return index;
}

/*
************************************************************
*    Execute a command if no pipe or redirection exists    *  
************************************************************
*/

int exec_command(char **args)
{
	pid_t pid = fork();
	int status;

	if(pid < 0)
	{ 
		printf("\nFailed forking child");  
		return EXIT_FAILURE;
	}else if(pid == 0)
	{ 
		//! Child executing
		if((execvp(args[0], args)) == -1)
		{
			printf("ERROR\n");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}else
	{ 
		//! Parent waits for child to terminate 
		wait(&status); 
		if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE){
			return EXIT_FAILURE;
		}else
		{
			return EXIT_SUCCESS;
		}
	} 
}

/*
*************************************************
*    Execute a command if redirection exists    *  
*************************************************
*/

int exec_redirect(char **args, int index)
{
	pid_t pid = fork(); 
	int fd_in, fd_out, status;

	if(pid < 0)
	{ 
		printf("\nFailed forking child");  
		return EXIT_FAILURE;
	}else if(pid == 0)
	{ 
		if(args[index][0] == '>')
		{
			//! Child process: stdout redirection
			fd_out = creat(args[index + 1], 0644);			
			close(1);
			dup(fd_out);
			close(fd_out);
			args[index] = NULL;
		}else
		{
			//! Child process: stdin redirection	
			fd_in = open(args[index + 1], O_RDONLY, 0);
        	close(0);
			dup(fd_in);
			close(fd_in);
			args[index] = NULL;
		}

		if((execvp(args[0], args)) == -1)
		{
			fprintf(stderr, "ERROR\n");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
		
	}else
	{ 
		//! Parent waits for child to terminate 
		wait(&status); 
		if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE)
			return EXIT_FAILURE;
		else
			return EXIT_SUCCESS;
	}
}

/*
******************************************
*    Execute a command if pipe exists    *  
******************************************
*/

int exec_pipe(char **args, int index)
{
	int fd[2], status;
	pid_t p1, p2; 
	char **piped_arg = (char **)malloc(MAX_ARG_NUM*sizeof(char *));
	char *token;

	if(pipe(fd) < 0) 
	{ 
		printf("\nPipe could not be initialized"); 
		return EXIT_FAILURE; 
	} 

	p1 = fork(); 

	if(p1 < 0) 
	{ 
		printf("\nFailed forking child");  
		return EXIT_FAILURE;
	}else if(p1 == 0) 
	{ 
		//! Child 1 executing
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		args[index] = NULL;

		if((execvp(args[0], args)) == -1)
		{
			fprintf(stderr, "ERROR\n");
			exit(EXIT_FAILURE);
		}
		exit(EXIT_SUCCESS);
	}else 
	{ 
		close(fd[1]);
		wait(&status); //! Parent waits for child 1 to terminate 

		p2 = fork(); 

		if(p2 < 0) 
		{ 
			printf("\nCould not fork"); 
			return EXIT_FAILURE; 
		}else if(p2 == 0) 
		{ 
			//! Child 2 executing
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);

			//! Variable "piped_arg" containts the arguments after the "|" symbol
			int j = 0;
			do
			{
				token = args[++index];
				piped_arg[j++] = token;
			}while(token != NULL);

			if((execvp(piped_arg[0], piped_arg)) == -1)
			{
				fprintf(stderr, "ERROR\n");
				exit(EXIT_FAILURE);
			}
			exit(EXIT_SUCCESS); 	
		}else
		{ 
			close(fd[0]);
			wait(&status); // Parent waits for child 2 to terminate

			if(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_FAILURE)
				return EXIT_FAILURE;
			else
				return EXIT_SUCCESS;
		} 
	} 
} 