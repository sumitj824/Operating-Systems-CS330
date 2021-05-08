#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>



int main(int argc, char *argv[])
{
	int fd1[2],fd2[2],fd3[2],fd4[2];
	
	if(pipe(fd1)<0)exit(-1);
	if(pipe(fd2)<0)exit(-1);
	if(pipe(fd3)<0)exit(-1);
	if(pipe(fd4)<0)exit(-1);
	

	pid_t pid=fork();
	if(pid<0)exit(-1);
	if(!pid)  // for player1
	{	
		dup2(fd1[0],0);
		dup2(fd2[1],1);
		if(execl(argv[1],argv[1],NULL))exit(-1);
		
	}

	pid=fork();
	if(pid<0)exit(-1);
	if(!pid)   //for player2
	{	
		dup2(fd3[0],0);
		dup2(fd4[1],1);
		if(execl(argv[2],argv[2],NULL))exit(-1);
	}



	int p1=0,p2=0;  //p1 -> player1_score , p2->player2_score
	char  x,y;
	for(int i=0;i<10;i++)
	{
		write(fd1[1],"GO\n",3);
		write(fd3[1],"GO\n",3);
		read(fd2[0],&x,1);
		read(fd4[0],&y,1);
		if(x==y)continue;
		if(x=='0')
		{
			if(y=='1')p2++;
			else p1++;
		}
		if(x=='1')
		{
			if(y=='2')p2++;
			else p1++;
		}
		if(x=='2')
		{
			if(y=='0')p2++;
			else p1++;
		}
	}
	
	
	printf("%d %d",p1,p2);



	exit(0);
}

