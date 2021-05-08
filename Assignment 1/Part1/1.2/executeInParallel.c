#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <unistd.h>
#include<wait.h>
#include<fcntl.h>

int executeCommand (char * cmd)
{
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


int execute_in_parallel(char *infile, char *outfile)
{
	int fd1,fd2;
	fd1=open(infile,O_RDONLY);
    if(fd1==-1)
    {
        return -1;//file is not opening
    }
	dup2(fd1,0);
	fd2=open(outfile,O_WRONLY|O_CREAT,0644);
    if(fd2==-1)
    {
        return -1;//file is not opening
    }
	dup2(fd2,1);
	
    int fd[200];
    for(int i=0;i<100;i++)
    {
        if(pipe(&fd[2*i])<0)return -1;
    }

    int i=0;
    while (1)
    {
        char s[100];
        int j=0;
        while (1)
        {
            char buf[2];
            if(!read(0,buf,1))break;
            if(buf[0]=='\0' || buf[0]=='\n')break;
            s[j++]=buf[0];
        }
        if(j==0)break;
        s[j]='\0';
        pid_t pid=fork();
        if(!pid)
        {       
            dup2(fd[2*i+1],1);
            executeCommand(s);
            exit(-1);
        }
        i++;
    }
    
    while (wait(NULL)!=-1)
    {
        //wait to finish all
    }
    
    
    for(int j=0;j<i;j++)
    {
        char buf[1000];
        int r=read(fd[2*j],buf,1000);
        buf[r]='\0';
        printf("%s",buf);

    }
    
	return 0;
}

int main(int argc, char *argv[])
{
	return execute_in_parallel(argv[1], argv[2]);
}
