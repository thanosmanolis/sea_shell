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
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define GREEN_BOLD "\033[1;32m"
#define YELLOW "\033[0;33m"
#define RESET_COLOR "\033[0m"

//! All functions used
void interactive_mode	  (void);
void batch_mode			  (char **argv);
void control_unit		  (char *input);
void check_delims         (char *input, int *delims, int *wrong_delims);
void replace_wrong_delims (char *input, int *wrong_delims);
int  parse_line			  (char **commands, char *input);
void parse_command		  (char *command, char **args);
int  check_pipe_redirect  (char **args);
int  exec_command		  (char **args);
int  exec_redirect		  (char **args, int state);
int  exec_pipe			  (char **args);
void parse_new		      (char **args, char **new_args1, char **new_args2, const char delim[]);

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
	printf(GREEN "****************************************\n"
		   		 "**                                    **\n"
		   		 "**    This is a custom shell in c.    **\n"
		   		 "**                                    **\n"
		   		 "****************************************\n\n" RESET_COLOR);

	char *input_str = (char *)malloc(MAX_INPUT_LENGTH*sizeof(char));

	while(1)
	{
		printf(GREEN_BOLD "manolis_8856> " RESET_COLOR);
		fgets(input_str, MAX_INPUT_LENGTH, stdin); 

		if(!strcmp(input_str, "quit\n"))
			break;
		else if(strspn(input_str, " \r\n\t"))
			continue;

		control_unit(input_str);	
	}

	printf(YELLOW "\nQuiting... ¯\\_(ツ)_/¯\n\n" RESET_COLOR);
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
	printf(GREEN "***********************************************\n"
		   		 "**                                           **\n"
		   		 "**    This is the output of a batch file.    **\n"
		   		 "**                                           **\n"
		   		 "***********************************************" RESET_COLOR "\n");

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
	int j = 0;
	while(getline(&input_str, &len, fp) != -1) 
	{
		while(input_str[j] == ' ')
			j++;
		if(input_str[j] == '#') //! Check if this line is a comment
			continue;

		if(strspn(input_str, " \r\n\t"))
			continue;
		else if(!strncmp(input_str, "quit\n", 4))
			break;
		else
			batch_inputs[i++] = strdup(input_str);
	}

    //! Pass each command to the main control unit
    for(int j=0; j<i; j++)
		control_unit(batch_inputs[j]);

    //! Quiting ... Either by "quit" command or by no more given commands
    printf(YELLOW "\nQuiting... ¯\\_(ツ)_/¯\n\n" RESET_COLOR);
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
	int   *delims 		= (int *)malloc(MAX_ARG_NUM*sizeof(int)); 		//! Array for delimiters (";" or "&&")
	int   *wrong_delims = (int *)malloc(MAX_CMD_LENGTH*sizeof(int)); 	//! Array for the index of inputs changes from "&" or ";" to "x"
	
	check_delims(input, delims, wrong_delims);	   //! Fill the array with the delimiters. Replace "&" with "x" and ";" with "y" 
	if(parse_line(commands, input))				   //! Split input if delimiters exist
    	replace_wrong_delims(input, wrong_delims); //! Replace "x" with "&" and "y" with ";"		    

	int i = 0;
	int index, status;

	do
	{
		strcpy(command,commands[i++]); 
		parse_command(command, args);      //! Split each command if spaces exist
		index = check_pipe_redirect(args); //! (-1: no pipe or redirection exist) | (0: pipe exists) | (1/2: input/output redirection exists)

		if(index == 0)
			status = exec_pipe(args);
	    else if((index == 1) || (index == 2))
	    	status = exec_redirect(args, index);
	    else
			status = exec_command(args);
	}while(commands[i] && (delims[i-1] == 2 || status == EXIT_SUCCESS)); //! If an argument is wrong, break only if the next delimiter is "&&"

	free(args);
	free(commands);
	free(command);
	free(delims);
	free(wrong_delims);
}

/*
***************************************************************************************
*    Create an array with the right delimiters. Right delimiters are the ones that    * 
*	 are a single ";" or two "&". If a wrong delimiter exists, change its value to    *
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
			}else
			{
				for(int x=i-index; x<i+1; x++)
				{
					input[x] = 'x';
					wrong_delims[y++] = x;
				}
			}			
		}else if(input[i] == ';')
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
			}else
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
***************************************************************
*    Get the whole input and split it if ";" or "&" exists    *  
***************************************************************
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
***********************************************************************************************
*    Replace all the "x" with "&" and all the "y" with ";" (back to their original values)    *
***********************************************************************************************
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
*******************************************************
*    Get each command and split it if space exists    *  
*******************************************************
*/

void parse_command(char *command, char **args)
{
	const char delim[] = " \r\n\t";
	char *token;
	int i = 0;		

	token = strtok(command, delim);
	
	do
	{ 
		args[i++] = token;

		if((i == MAX_ARG_NUM-1)	|| (args[i-1] == '\0'))
			break;

		token = strtok(NULL, delim); 
	}while(token != NULL);

	args[i] = NULL;
}

/*
*********************************************
*    Check if pipe or redirection exists    *  
*********************************************
*/

int check_pipe_redirect(char **args)
{	
	int index = -1; //! (-1: no pipe or redirection exist) | (0: pipe exists) | (1/2: input/output redirection exists)
	
	int i = 0;
	do
	{ 	
		if((i == MAX_ARG_NUM-1)	|| (args[i] == '\0'))
			break;

		//! Check if pipe exists
		if(strchr(args[i], '|'))
		{
			index = 0;
			break;
		}

		//! Check if redirection exists
		if(strchr(args[i], '<'))
		{
			index = 1;
			break;
		}else if(strchr(args[i], '>'))
		{
			index = 2;
			break;
		}
	}while(args[++i] != NULL);

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
			printf(RED "ERROR: Wrong command\n" RESET_COLOR);
			exit(EXIT_FAILURE);
		}
	}else
	{ 
		//! Parent waits for child to terminate 
		wait(&status);

		if(status != 0)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
*************************************************
*    Execute a command if redirection exists    *  
*************************************************
*/

int exec_redirect(char **args, int state)
{
	pid_t pid = fork(); 
	int fd_in, fd_out, status;
	char **redirected_args1 = (char **)malloc(MAX_ARG_NUM*sizeof(char *));
	char **redirected_args2 = (char **)malloc(MAX_ARG_NUM*sizeof(char *));

	if(pid < 0)
	{ 
		printf("\nFailed forking child");  
		return EXIT_FAILURE;
	}else if(pid == 0)
	{ 
		if(state == 1)
		{
			//! Child process: stdin redirection
			parse_new(args, redirected_args1, redirected_args2, "<\n");	
			fd_in = open(redirected_args2[0], O_RDONLY, 0);
        	close(0);
			dup(fd_in);
			close(fd_in);
		}else
		{
			//! Child process: stdout redirection
			parse_new(args, redirected_args1, redirected_args2, ">\n");
			fd_out = creat(redirected_args2[0], 0644);			
			close(1);
			dup(fd_out);
			close(fd_out);
		}

		if((execvp(redirected_args1[0], redirected_args1)) == -1)
		{
			fprintf(stderr, RED "ERROR: Wrong command\n" RESET_COLOR);
			exit(EXIT_FAILURE);
		}
	}else
	{ 
		//! Parent waits for child to terminate 
		wait(&status); 

		free(redirected_args1);
		free(redirected_args2);

		if(status != 0)
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/*
*****************************************************************************
*    Execute a command if pipe exists. This function also implements the    *
*	 cases where more than one pipes exist, or redirections also exist.     *
*****************************************************************************
*/

int exec_pipe(char **args)
{
	int fd[2], status, next_index;
	pid_t p1, p2; 
	char **piped_args1 = (char **)malloc(MAX_ARG_NUM*sizeof(char *));
	char **piped_args2 = (char **)malloc(MAX_ARG_NUM*sizeof(char *));

	parse_new(args, piped_args1, piped_args2, "|\n");

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

		if((execvp(piped_args1[0], piped_args1)) == -1)
		{
			fprintf(stderr, RED "ERROR: Wrong command\n" RESET_COLOR);
			exit(EXIT_FAILURE);
		}
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

			//! Check if there is another pipe or redirect following
			//! If pipe exists, do exec_pipe with the new array
			//! If redirect exists, do exec_redirect with the new array and index
			//! If none of them exists, continue with the execution.
			next_index = check_pipe_redirect(piped_args2);

			if(next_index == 0)
			{	
				close(fd[0]);
				wait(&status);
				exec_pipe(piped_args2);
				exit(EXIT_SUCCESS);
			}else if((next_index == 1) || (next_index == 2))
			{
				close(fd[0]);
				wait(&status);
				exec_redirect(piped_args2, next_index);
				exit(EXIT_SUCCESS);
			}else
			{
				if((execvp(piped_args2[0], piped_args2)) == -1)
				{
					fprintf(stderr, RED "ERROR: Wrong command\n" RESET_COLOR);
					exit(EXIT_FAILURE);
				}
			}
		}else
		{ 
			close(fd[0]);
			wait(&status); //! Parent waits for child 2 to terminate

			free(piped_args1);
			free(piped_args2);

			if(status != 0)
				return EXIT_FAILURE;
		} 
	}
	return EXIT_SUCCESS;
} 

/*
***************************************************************************
*    Take the arguments that were previously splitted to spaces. Split    *
*	 them again, but this time according to the new delimiter (|,>,<).    *
*    Store them in 2 new arrays (before and after the delimiter)		  *
***************************************************************************
*/

void parse_new(char **args, char **new_args1, char **new_args2, const char delim[])
{
	int state;
	char *token;
	int i = 0;		
	int j = 0;

	//! As long as there is no delimiter, just copy the exact same value
	while(!strchr(args[j], '|') && !strchr(args[j], '>') && !strchr(args[j], '<'))
		new_args1[i++] = args[j++]; 

	//! Create the array on the left of the delimiter
	token = strtok(args[j], delim);
	do
	{
		if((strcmp(args[j], "|") == 0) || (strcmp(args[j], ">") == 0) || (strcmp(args[j], "<") == 0))
		{
			state = 0;
			break;
		}else
		{
			if(args[j][0] == '|' || args[j][0] == '>' || args[j][0] == '<')
				break;
			else if(token == NULL)
			{
				token = args[++j];
				break;
			}

			state = 1;
			new_args1[i++] = token;			
		}
		
		token = strtok(NULL, delim);
	}while(token == NULL);

	new_args1[i] = NULL;

	//! Create the array on the right of the delimiter
	if(state == 0)
	{
		new_args2[0] = args[++j];
		j++;
	}else
	{
		new_args2[0] = token;
		j++;
	}

	i = 1;

	do
		new_args2[i++] = args[j++];
	while(args[j-1] != NULL);
}