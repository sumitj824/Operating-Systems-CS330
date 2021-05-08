#include <msg_queue.h>
#include <context.h>
#include <memory.h>
#include <file.h>
#include <lib.h>
#include <entry.h>



/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

struct msg_queue_info *alloc_msg_queue_info()
{
	struct msg_queue_info *info;
	info = (struct msg_queue_info *)os_page_alloc(OS_DS_REG);
	
	if(!info){
		return NULL;
	}
	return info;
}

void free_msg_queue_info(struct msg_queue_info *q)
{
	os_page_free(OS_DS_REG, q);
}

struct message *alloc_buffer()
{
	struct message *buff;
	buff = (struct message *)os_page_alloc(OS_DS_REG);
	if(!buff)
		return NULL;
	return buff;	
}

void free_msg_queue_buffer(struct message *b)
{
	os_page_free(OS_DS_REG, b);
}

/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/


int do_create_msg_queue(struct exec_context *ctx)
{
	/** 
	 ** TODO Implement functionality to
	 * create a message queue
	 **/
	int i=0;
	while (ctx->files[i])i++;
	struct file *f=alloc_file();   
	if(!f)return -ENOMEM;
	struct msg_queue_info *m=alloc_msg_queue_info();
	if(!m)return -ENOMEM;
	// * Intialize the data of m
	m->qsize=0;
	m->msg_buffer=alloc_buffer();
	m->member_count=1;
	m->member_pid[0]=ctx->pid;
	for(int i=0;i<8;i++)
	{
		for(int j=0;j<8;j++)m->isblocked[i][j]=0;
	}
	f->msg_queue=m;
	ctx->files[i]=f;
	return i;
}

w
int do_msg_queue_rcv(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 * *TODO Implement functionality to
	 * recieve a message
	 * 
	 **/
	if(!filep->msg_queue)return -EINVAL;
	int pid=ctx->pid;
	int flag=0;
	int s=filep->msg_queue->qsize;
	for(int i=0;i<s;i++)
	{
		if(filep->msg_queue->msg_buffer[i].to_pid==pid && !flag)
		{
			flag=1;
			msg->from_pid=filep->msg_queue->msg_buffer[i].from_pid;
			msg->to_pid=filep->msg_queue->msg_buffer[i].to_pid;

			for(int j=0;j<MAX_TXT_SIZE;j++)
			{
				msg->msg_txt[j]=filep->msg_queue->msg_buffer[i].msg_txt[j];
				if(msg->msg_txt[j]=='\0')break;
			}
		}
		if(flag)
		{
			if(i+1<s)filep->msg_queue->msg_buffer[i]=filep->msg_queue->msg_buffer[i+1];
		}
	}
	if(flag)filep->msg_queue->qsize--;
	return flag;
}


int do_msg_queue_send(struct exec_context *ctx, struct file *filep, struct message *msg)
{
	/** 
	 ** TODO Implement functionality to
	 * send a message
	 * 
	 **/
	if(!filep->msg_queue)return -EINVAL;
	int s=filep->msg_queue->member_count;
	int count=0;
	if(msg->to_pid==BROADCAST_PID)   // Broadcast 
	{
		for(int i=0;i<s;i++)
	    {	
			int to=filep->msg_queue->member_pid[i];
		    if(msg->from_pid==to)continue;
			if(filep->msg_queue->isblocked[msg->from_pid][to])continue;
			filep->msg_queue->msg_buffer[filep->msg_queue->qsize].from_pid=msg->from_pid;
			filep->msg_queue->msg_buffer[filep->msg_queue->qsize].to_pid=to;
			for(int j=0;j<MAX_TXT_SIZE;j++)
			{
				filep->msg_queue->msg_buffer[filep->msg_queue->qsize].msg_txt[j]=msg->msg_txt[j];
				if(msg->msg_txt[j]=='\0')break;
			}
			filep->msg_queue->qsize++;
			count++;
	    }
	    return count;
	}

	for(int i=0;i<s;i++)     //unicast
	{	int to=filep->msg_queue->member_pid[i];
		if(to==msg->to_pid)
		{ 
			if(filep->msg_queue->isblocked[msg->from_pid][to])return -EINVAL;
			filep->msg_queue->msg_buffer[filep->msg_queue->qsize].from_pid=msg->from_pid;
			filep->msg_queue->msg_buffer[filep->msg_queue->qsize].to_pid=to;
			for(int j=0;j<MAX_TXT_SIZE;j++)
			{
				filep->msg_queue->msg_buffer[filep->msg_queue->qsize].msg_txt[j]=msg->msg_txt[j];
				if(msg->msg_txt[j]=='\0')break;
			}
			filep->msg_queue->qsize++;
			count++;
		}

	}
	return count;

}

void do_add_child_to_msg_queue(struct exec_context *child_ctx)
{
	/** 
	 ** TODO Implementation of fork handler 
	 **/
	int pid=child_ctx->pid;
	for(int i=0;i<MAX_OPEN_FILES;i++)
	{
		if(child_ctx->files[i]->msg_queue)
		{
			
			//*add child process to msg_queue by modifying data structure u used
			//* also note that the child does not get the msgs int msg queue that were addressed to parent
			child_ctx->files[i]->msg_queue->member_pid[child_ctx->files[i]->msg_queue->member_count++]=pid;
		}
	}

}

void do_msg_queue_cleanup(struct exec_context *ctx)
{
	/** 
	 * TODO Implementation of exit handler 
	 * 
	 **/
	for(int i=0;i<MAX_OPEN_FILES;i++)
	{
		if(ctx->files[i]->msg_queue)
		{
			do_msg_queue_close(ctx,i);
		}
	}


}

int do_msg_queue_get_member_info(struct exec_context *ctx, struct file *filep, struct msg_queue_member_info *info)
{
	/** 
	 ** TODO Implementation of exit handler 
	 **/
	if(!filep->msg_queue)return -EINVAL;
	int s=filep->msg_queue->member_count;
	info->member_count=s;
	for(int i=0;i<s;i++)
	{
		info->member_pid[i]=filep->msg_queue->member_pid[i];
	}

	return 0;
}


int do_get_msg_count(struct exec_context *ctx, struct file *filep)
{
	/** 
	 ** TODO Implement functionality to
	 * return pending message count to calling process
	 **/
	if(!filep->msg_queue)return -EINVAL;
	int pid=ctx->pid;
	int count=0;

	for(int i=0;i<filep->msg_queue->qsize;i++)
	{
		if(filep->msg_queue->msg_buffer[i].to_pid==pid)count++;
	}

	return count;
}

int do_msg_queue_block(struct exec_context *ctx, struct file *filep, int pid)
{
	/** 
	 ** TODO Implement functionality to
	 * 
	 * block messages from another process 
	 **/
	if(!filep->msg_queue)return -EINVAL;
	int n=filep->msg_queue->member_count;
	int i;
	for( i=0;i<n;i++)
	{
		if(filep->msg_queue->member_pid[i]==pid)break;
	}
	if(i==n)return -EINVAL;
	filep->msg_queue->isblocked[pid][ctx->pid]=1;
	return 0;
}

int do_msg_queue_close(struct exec_context *ctx, int fd)
{
	/** 
	 ** TODO Implement functionality to
	 * remove the calling process from the message queue 
	 **/
	if(!ctx->files[fd]->msg_queue)return -EINVAL;
	int i;
	int x=ctx->files[fd]->msg_queue->member_count;
	for(i=0;i<x;i++)
	{
		if(ctx->files[fd]->msg_queue->member_pid[i]==ctx->pid)break;
	}
	ctx->files[fd]->msg_queue->member_pid[i]=ctx->files[fd]->msg_queue->member_pid[x-1];
	ctx->files[fd]->msg_queue->member_count--;
	if(x==1)
	{
		free_msg_queue_buffer(ctx->files[fd]->msg_queue->msg_buffer);
		free_msg_queue_info(ctx->files[fd]->msg_queue);
	}
	return 0;

}

