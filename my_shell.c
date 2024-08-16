#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
bool exitTheShell = false;
pid_t backGroundProcesses[1000];
int counter = 0;
int fgGroup = 1, bgGroup = 0;

/* Splits the string by space and returns the array of tokens
 *
 */
char **tokenize(char *line)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

char *getNewPath(char *s, char *path)
{
	char arr[100];
	int i = 0;
	for (; s[i] != '\0'; i++)
		arr[i] = s[i];
	arr[i++] = '/';
	for (int j = 0; path[j] != '\0'; j++)
		arr[i++] = path[j];
	arr[i] = '\0';
	char *newpath = arr;
	return newpath;
}

int executeBackground(char **tokens)
{
	// implementing exit user command with & in the end
	if (strcmp(tokens[0], "exit") == 0)
	{
		kill(getppid(), SIGUSR1); // sent signal to parent
		return 1;
	}
	// printf("Shell: Background process %d finished\n",getpid());
	execvp(tokens[0], tokens); // executing in child

	printf("Command execution failed\n"); // will reach here only if exec command fails
	return 0;
}

// catching and handling SIGINT or CTRL + C signal (foreground should be terminated Background processes to remain as it is )
void SIGINT_Handler()
{
	int pid = fgGroup;
	kill(pid, SIGKILL);
}
// counting no. of tokens
int count_len(char **tokens)
{
	int cnt = 0;
	while (tokens[cnt++] != NULL)
		;
	return cnt - 1;
}
// handling user generated signal for the exit call
void handle_sigusr1(int sig)
{
	exitTheShell = true;
	return;
}

int main(int argc, char *argv[])
{
	char line[MAX_INPUT_SIZE];
	char **tokens;
	int i;

	signal(SIGINT, SIGINT_Handler); // listening for the CTRL + C signals

	while (1)
	{

		// reaping terminated (zombie) background processes on every iteration
		int tempStatus;
		int rv = waitpid(-1, &tempStatus, WNOHANG);
		if (rv > 0)
		{
			printf("Shell : Background terminated process %d is reaped\n", rv);
		}
		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; // terminate with new line
		tokens = tokenize(line);

		if (tokens[0] == NULL) // if no input tokens (pressed enter) start shell next line
			continue;

		// Part A Implementation of cd command
		if (strcmp(tokens[0], "cd") == 0)
		{
			char *dir = tokens[1];
			// char* currpwd;
			// currpwd=getcwd(currpwd,100);
			// printf("dir is %s\n", dir);
			// printf("currpwd is %s\n", currpwd);
			/*if(dir[0]=='.' && dir[1]=='.')
				chdir("..");
			else{
				printf("hello\n");
				char * newpath = (char *) malloc(2 + strlen(currpwd)+ strlen(dir) );
				strcpy(newpath,currpwd);
				strcat(newpath,"/");
				strcat(newpath,dir);
				printf("newpath is %s\n", newpath);
				chdir(newpath);
			}*/
			if (count_len(tokens) > 2)
			{
				printf("Invalid CD command\n");
				continue;
			}
			int changedir_status = chdir(dir);
			char *s;
			s = getcwd(s, 100);
			if (changedir_status)
				printf("Changed path to %s\n", s);
			else
				printf("Invalid CD command\n");
			continue;
		}

		int fc = fork();
		if (fc < 0)
		{ // invalid fork
			fprintf(stderr, "%s\n", "Unable to create child process");
		}
		else if (fc == 0)
		{
			// background execution of process
			if (tokens[count_len(tokens) - 1][0] == '&')
			{
				setpgid(getpid(), bgGroup); // setting this process to background process's group id
				tokens[count_len(tokens) - 1] = NULL;
				if (executeBackground(tokens))
				{
					// printf("Shell: Background process %d finished\n",getpid());
					_exit(1);
				}
			}
			else
			{ // for fore ground processes setting them to foreground group processses id
				setpgid(getpid(), fgGroup);
			}

			// implementing exit user command
			if (strcmp(tokens[0], "exit") == 0)
			{
				kill(getppid(), SIGUSR1); // sent signal to parent
				return 1;
			}

			execvp(tokens[0], tokens);			  // executing in child
			printf("Command execution failed\n"); // will reach here only if exec command fails
			_exit(1);
		}
		else
		{ // parent process code
			int status;
			// signal from child for exit system call
			struct sigaction sa;
			sa.sa_flags = SA_RESTART;
			sa.sa_handler = &handle_sigusr1; // siguser signal handler funtion
			sigaction(SIGUSR1, &sa, NULL);

			// to get the status child/(children) while they are in background execution
			if (strcmp(tokens[count_len(tokens) - 1], "&") == 0)
			{
				backGroundProcesses[counter++] = fc;		  // storing pid of BG processes into array to reap of them on exit
				int pid1_res = waitpid(fc, &status, WNOHANG); // WNOHANG BG  keep running in background
				if (WIFEXITED(status))
					printf("Background pid: %d exited \n", fc); // exited from execution went into background not terminated yet
				// wait(&status);
				continue;
			}

			int wc = waitpid(fc, &status, 0);
			if (exitTheShell)
			{ // exit command
				// reaping out all the background processes, if any, (on exiting the program)
				for (int i = 0; i < counter; i++)
					kill(backGroundProcesses[i], SIGINT);
				return 1;
			}
		}

		for (i = 0; tokens[i] != NULL; i++)
		{
			// printf("found token %s (remove this debug output later)\n", tokens[i]);
		}

		// Freeing the allocated memory
		for (i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
	}
	return 0;
}

/*
Notes

Exec System call
execlp,execvp,execve....
l-->> list of arguments
p-->> path is curent directory no need to give explicitely
v-->> vector/array of argument strings to pass
e-->> environment

Background processes concept

sleep 5 &  // bakgrounded
sleep 7 &

bg , fg
jobs , jobs -l all background processes
ctrl + z stopped ctrl + c terminated
nohup, if did fg even after terminal closed and opened again





*/
