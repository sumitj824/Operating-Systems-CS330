
#define ROCK        0 
#define PAPER       1 
#define SCISSORS    2 

#define STDIN 		0
#define STDOUT 		1
#define STDERR		2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include "gameUtils.h"

int getWalkOver(int numPlayers); // Returns a number between [1, numPlayers]

int N=10;

int main(int argc, char *argv[])
{
	int op=1;
	if(argc==4)
	{	
		if(argv[1][0]!='-' ||argv[1][1]!='r' || argv[1][2]!='\0' )exit(-1);
		N=atoi(argv[2]);
		op=3;
	}

	int fd=open(argv[op],O_RDONLY);
	dup2(fd,0);
	char x;
	read(0,&x,1);
	int n=0; //total number of players
	while(x!='\n' && x!='\0')
	{
		n=n*10+(x-'0');
		if(read(0,&x,1)==0)break;
	}

	// printf("%d\n",N);

	
	
	int active[n];

	int fd1[2*n]; //write for parent
	int fd2[2*n];// read for parent
    for(int i=0;i<n;i++)
    {
        if(pipe(&fd1[2*i])<0)exit(-1);
		if(pipe(&fd2[2*i]))exit(-1);
    }
	
	// printf("%d\n",n);
	for(int i=0;i<n;i++)
	{	
		active[i]=1;
		char s[100];
        int j=0;
        while (1)
        {
            char buf[2];
            if(!read(0,buf,1))break;
            if(buf[0]=='\0' || buf[0]=='\n')break;
            s[j++]=buf[0];
        }
        s[j]='\0';
		// printf("%s",s);
		printf("p%d",i);
		if(i!=n-1)printf(" ");
		int pid=fork();
		if(pid<0)exit(-1);
		if(!pid)
		{	
			dup2(fd1[2*i],0);
			dup2(fd2[2*i+1],1);
			execl(s,s,NULL);
			exit(-1);
		}
	}

	int total=n;
	while (total!=1)
	{
		printf("\n");
		int till=total;
		int skip=n;
		if(total%2!=0)
		{
			skip=getWalkOver(total)-1;//index
			till=total-1;
		}
		int a[till];//index of active players
		int k=0;
		for(int i=0;i<n;i++)
		{
			if(active[i])
			{
				if(skip==k)skip=n+1;
				else a[k++]=i;
			}
		}
		for(int i=0;i<till;i+=2)
		{
			int p1=0,p2=0; 
			char  x,y;
			for(int r=0;r<N;r++)
			{
				write(fd1[2*a[i]+1],"GO\n",3);
				read(fd2[2*a[i]],&x,1);
				write(fd1[2*a[i+1]+1],"GO\n",3);
				read(fd2[2*a[i+1]],&y,1);
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

			if(p1>=p2)active[a[i+1]]=0;
			else active[a[i]]=0;

			
		}
		total=(total+1)/2;
		int c=0;
		for(int i=0;i<n;i++)
		{
			if(active[i])c++;
		}
		int j=0;
		for(int i=0;i<n;i++)
		{
			if(active[i])
			{	j++;
				printf("p%d",i);
				if(j!=c)printf(" ");
			}
		}
	}
	

	exit(0);
}
