#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/

void free_file_object(struct file *filep)
{
	if(filep)
	{
		os_page_free(OS_DS_REG ,filep);
		stats->file_objects--;
	}
}

struct file *alloc_file()
{
	struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
	file->fops = (struct fileops *) (file + sizeof(struct file)); 
	bzero((char *)file->fops, sizeof(struct fileops));
	file->ref_count = 1;
	file->offp = 0;
	stats->file_objects++;
	return file; 
}

void *alloc_memory_buffer()
{
	return os_page_alloc(OS_DS_REG); 
}

void free_memory_buffer(void *ptr)
{
	os_page_free(OS_DS_REG, ptr);
}

/* STDIN,STDOUT and STDERR Handlers */

/* read call corresponding to stdin */

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
	kbd_read(buff);
	return 1;
}

/* write call corresponding to stdout */

static int do_write_console(struct file* filep, char * buff, u32 count)
{
	struct exec_context *current = get_current_ctx();
	return do_write(current, (u64)buff, (u64)count);
}

long std_close(struct file *filep)
{
	filep->ref_count--;
	if(!filep->ref_count)
		free_file_object(filep);
	return 0;
}
struct file *create_standard_IO(int type)
{
	struct file *filep = alloc_file();
	filep->type = type;
	if(type == STDIN)
		filep->mode = O_READ;
	else
		filep->mode = O_WRITE;
	if(type == STDIN){
		filep->fops->read = do_read_kbd;
	}else{
		filep->fops->write = do_write_console;
	}
	filep->fops->close = std_close;
	return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
	int fd = type;
	struct file *filep = ctx->files[type];
	if(!filep){
		filep = create_standard_IO(type);
	}else{
		filep->ref_count++;
		fd = 3;
		while(ctx->files[fd])
			fd++; 
	}
	ctx->files[fd] = filep;
	return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/

/* File exit handler */
	void do_file_exit(struct exec_context *ctx)
{
	/*
	*TODO the process is exiting. Adjust the refcount
	of files*/
	for(int i=0;i<MAX_OPEN_FILES;i++)
	{
		do_file_close(ctx->files[i]);
	}
	

}

/*Regular file handlers to be written as part of the assignment*/


static int do_read_regular(struct file *filep, char * buff, u32 count)
{
	
	int ret_fd = -EINVAL; 
	if(!(filep->inode->is_valid) )return ret_fd;
	if(filep->mode& O_READ!=O_READ )return -EACCES;
	int x= flat_read(filep->inode,buff,count,&(filep->offp));
	(filep->offp)+=x;
	return x;
}

/*write call corresponding to regular file */

static int do_write_regular(struct file *filep, char * buff, u32 count)
{
	
	int ret_fd = -EINVAL; 
	if(!(filep->inode->is_valid) )return ret_fd;
	if(filep->mode& O_READ!=O_READ )return -EACCES;
	int x=flat_write(filep->inode,buff,count,&(filep->offp));
	(filep->offp)+=x;
	return x;
}	

long do_file_close(struct file *filep)
{

	if(!(filep->ref_count)) return -EINVAL;
	filep->ref_count--;
	if(!filep->ref_count)
		free_file_object(filep);
	return 0;

}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{

	int ret_fd = -EINVAL; 
	if(whence==SEEK_SET)
	{	int x=offset;
		if(x>filep->inode->file_size ) return ret_fd;
		filep->offp=x;
		ret_fd=x;
	}
	if(whence==SEEK_CUR)
	{	
		int x=filep->offp+offset;
		if(x>filep->inode->file_size  ) return ret_fd;
		filep->offp=x;
		ret_fd=x;
	}
	if(whence==SEEK_END)
	{
		int x=filep->inode->file_size +offset;
		if(x>filep->inode->file_size ) return ret_fd;
		filep->offp=x;
		ret_fd=x;
	}

	return ret_fd;
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{

	int ret_fd = -EINVAL; 
	struct inode* i=lookup_inode(filename);
	if(!i)
	{
		if((flags&O_CREAT)==O_CREAT)i=create_inode(filename,mode);
		else return ret_fd;
	}
	int fd=3;
	while (ctx->files[fd])fd++;

	struct file * f=alloc_file();
	f->inode=i;
	f->offp=0;
	f->mode=flags;
	f->fops->read=do_read_regular;
	f->fops->write=do_write_regular;
	f->fops->close=do_file_close;
	f->fops->lseek=do_lseek_regular;
	ctx->files[fd]=f;
	return fd;
}

/**
 * Implementation dup 2 system call;
 */
int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
	
	int ret_fd = -EINVAL; 
	if(!current->files[oldfd])return ret_fd;
	if(oldfd==newfd)return newfd;
	if(current->files[newfd])do_file_close(current->files[newfd]);
	current->files[newfd]=current->files[oldfd];
 	return newfd;
}

int do_sendfile(struct exec_context *ctx, int outfd, int infd, long *offset, int count) {
	
	if(ctx->files[infd]==0 || ctx->files[outfd]==0)return -EINVAL;
	if(!(ctx->files[infd]->mode &O_READ)  || !(ctx->files[outfd]->mode &O_WRITE))return -EACCES;

	if(offset==NULL)
	{
		char *buf=alloc_memory_buffer();
		int x=do_read_regular(ctx->files[infd],buf,count);
		x=do_write_regular(ctx->files[outfd],buf,count);
		free_memory_buffer(buf);
		return x;
	}
	else
	{
		char *buf=alloc_memory_buffer();
		if(!ctx->files[infd]->inode->is_valid)return -EINVAL;
		int x=flat_read(ctx->files[infd]->inode,buf,count,(int *)offset);
		x=do_write_regular(ctx->files[outfd],buf,count);
		free_memory_buffer(buf);
		return x;
	}

	return -EINVAL;
}


