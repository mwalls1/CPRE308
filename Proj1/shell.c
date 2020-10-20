#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
char** string_parse(char* input);
int waitBackground();
int main(int argc, char** argv)
{
	char *input = malloc(sizeof(char) * 100);
	char **args;
	char *prompt = "308sh> ";
	char wd[256];
	int bg = 0;
	pid_t childPid;
	int returnStatus;
	if(argc !=1 && argc !=3){
		printf("Not enough arguments!\n");
		exit(0);
	}
	else if(argc == 3){
		if(strcmp(argv[1], "-p")!= 0){
			printf("No prompt given!\n");
			exit(0);
		}
		else{
			prompt = argv[2];
		}
	}
	while(1){
		printf("%s", prompt);
		size_t bufferLen = 100;
		getline(&input, &bufferLen, stdin);
	        args = string_parse(input);
		int k = 0;
		bg = 0;
		while(args[k] != NULL)
		{
			if(strcmp(args[k], "&")==0)
			{
				bg = 1;
				args[k] = NULL;
				break;
				
			}
			k++;
		}
		if(strcmp(args[0], "")==0)
		{
			//do nothing
		}
		else if(strcmp(args[0], "cd")==0)
		{
			if(args[1]!=NULL)
			{
				chdir(args[1]);
			}
			else{
				chdir(getenv("HOME"));
			}
		}
		else if(strcmp(args[0],"pid")==0)
		{
			printf("%d\n", getpid());
		}
		else if(strcmp(args[0],"ppid")==0)
		{
			printf("%d\n", getppid());
		}
		else if(strcmp(args[0],"exit")==0)
		{
			printf("Goodbye!\n");
			exit(0);
		}
		else if(strcmp(args[0],"pwd")==0)
		{
			getcwd(wd, sizeof(wd));
			printf("%s\n", wd);
		}
		else
		{
			childPid = fork();
			if(childPid == 0)
			{
				int error = execvp(args[0],args);
				printf("Cannot exec %s: No such file or directory\n", args[0]);
				exit(error);
			}
			else
			{
				printf("[%d] ", childPid);
				printf("%s\n", args[0]);
				if(!bg)
				{
					waitpid(childPid,&returnStatus,0);
					if(WIFEXITED(returnStatus))
					{
						printf("[%d] %s Exit %d\n", childPid, args[0],WEXITSTATUS(returnStatus));
					}
					else
					{
						printf("[%d] %s Exit %d\n", childPid, args[0], WTERMSIG(returnStatus));
					}
				}
			}
		 }
		 waitBackground();
		
	}
}
int waitBackground()
{
	int status;
	int pid = waitpid(-1, &status, WNOHANG);
	if(pid > 0)
	{
		if(WIFEXITED(status))
		{
			printf("[%d] Exit %d\n", pid,WEXITSTATUS(status));
		}
		else
		{
			printf("[%d] Exit %d\n", pid, WTERMSIG(status));
		}
	}
	else
	{
		return 0;
	}
}
char** string_parse(char* input)
{
	char** arguments = malloc(100*sizeof(char*));
	char *delim = " ";
	char *ptr = malloc(100 * sizeof(char));
	ptr =  strtok(input, delim);
	if(ptr[strlen(ptr)-1] == '\n')
	{
		ptr[strlen(ptr)-1] = '\0';
	}
	else
	{
		ptr[strlen(ptr)] = '\0';
	}
	arguments[0] = ptr;
	int i = 1;
	ptr = strtok(NULL, delim);
	while(ptr != NULL)
	{
		if(ptr[strlen(ptr)-1] == '\n')
		{
			ptr[strlen(ptr)-1] = '\0';
		}
		else
		{
			ptr[strlen(ptr)] = '\0';
		}
		arguments[i] = ptr;
		ptr = strtok(NULL, delim);
		i++;
	}
        arguments[i] = NULL;
	return arguments;
}
