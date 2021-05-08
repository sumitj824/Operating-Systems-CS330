#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include<wait.h>

int executeCommand (char * cmd)
{
	
	pid_t pid;
	int status;
	char *st=strtok(cmd," ");
	char *args[100];
	int i=0;
	while (st!=NULL)
	{	
		args[i]=st;
		i++;
		st=strtok(NULL," ");
	}
	args[i]=NULL;

	char *s= getenv("CS330_PATH");

	pid=fork();
	if(pid<0)
	{
		return -1;
	}
	if(!pid)//child
	{
		char *token=strtok(s,":");
		while (token!=NULL)
		{
			char path[100];
			strcpy(path,token);
			strcat(path,"/");
			strcat(path,args[0]);
			execv(path,args); 
			token=strtok(NULL,":");
		}
	   printf("UNABLE TO EXECUTE\n");
	   return -1;
	}
	wait(&status);
	return WEXITSTATUS(status);
}

int main (int argc, char *argv[])
{
	return executeCommand(argv[1]);
}