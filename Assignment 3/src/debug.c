// #define DEBUG
#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>




/*****************************HELPERS******************************************/

/* 
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info)); 
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}

/*
 * frees a debug_info struct 
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}

/*
 * allocates memory to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers)); 
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/* 
 * allocate a node for breakpoint list 
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait_and_continue
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	child_ctx->dbg = NULL;	
	child_ctx->state = WAITING;	
}


/******************************************************************************/





/******************************************************************************/

/* This is the int 0x3 handler
 * Hit from the childs context
 */
long int3_handler(struct exec_context *ctx)
{	
	dprintk(" _____  int3 handler is called ________\n");
	struct exec_context *p=get_ctx_by_pid(ctx->ppid),*cur=ctx;
	//manually do the push %rbp instruction
	ctx->regs.entry_rsp -= 0x8;
	*(u64 *)ctx->regs.entry_rsp = ctx->regs.rbp;
	p->regs.rax=(ctx->regs.entry_rip)-1;
	ctx->state=WAITING;
	p->state=READY;
	schedule(p);
	return 0;		
}

/*
 * Exit handler.
 * Called on exit of Debugger and Debuggee 
 */

void free_list(struct breakpoint_info *cur)
{
	struct breakpoint_info *nxt;
	while (cur)
	{
		nxt=cur->next;
		free_breakpoint_info(cur);
		cur=nxt;
	}
}


void debugger_on_exit(struct exec_context *ctx)
{
	if(ctx->dbg==NULL)//child
	{	
		dprintk(" _____  child exit is called    _________\n");
		struct exec_context *p=get_ctx_by_pid(ctx->ppid);
		p->regs.rax=CHILD_EXIT;
		p->state=READY;
	}
	else
	{	
		dprintk(" ______   Parent exit is called  ________\n");
		struct breakpoint_info *cur,*nxt;
		free_list(ctx->dbg->head);
		os_free(ctx->dbg->bt,8*MAX_BACKTRACE);
		free_debug_info(ctx->dbg);
	}
	
}

/*
 * called from debuggers context
 * initializes debugger state
 */


int do_become_debugger(struct exec_context *ctx)
{
	struct debug_info *b=alloc_debug_info();
	if(b==NULL)
	{
		dprintk(" _______  dbg info not allocated  ________\n");
		return -1;
	}
	b->child_reg=alloc_regs();
	if(b->child_reg==NULL)
	{
		dprintk(" _______  child_reg not allocated  ________\n");
		return -1;
	}
	b->head=NULL;
	b->bp_num=0;
	b->total_bp=0;
	ctx->dbg=b;
	
	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr)
{	
	
	for(struct breakpoint_info *h=ctx->dbg->head;h!=NULL;h=h->next)
	{
		if(h->addr==(unsigned long long)addr)
		{
			h->status=1;
			return 0;
		}
	}
	if(ctx->dbg->total_bp==MAX_BREAKPOINTS)return -1; //already max
	struct breakpoint_info  *b=alloc_breakpoint_info();
	if(!b)return -1;
	b->addr=(unsigned long long)addr;
	*(char *)addr=INT3_OPCODE;
	b->num= ++ctx->dbg->bp_num;
	b->status=1;
	b->next=NULL;
	ctx->dbg->total_bp++;
	struct breakpoint_info *h=ctx->dbg->head;
	if(h==NULL)ctx->dbg->head=b;
	else
	{
		while(h->next)h=h->next;
		h->next=b;
	}
	return 0;
}




/*
 * called from debuggers context
 */
int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	struct breakpoint_info *h=ctx->dbg->head, *p=NULL;

	if(h && h->addr==(unsigned long long)addr)
	{
		ctx->dbg->head=h->next;
		ctx->dbg->total_bp--;	
		return 0;
	}

	while (h && h->addr!=(unsigned long long)addr)
	{
		p=h;
		h=h->next;
	}
	if(!h)return -1; //not present
	p->next=h->next;
	ctx->dbg->total_bp--;
	return 0;
}

/*
 * called from debuggers context
 */
int do_enable_breakpoint(struct exec_context *ctx, void *addr)
{
	for(struct breakpoint_info *h=ctx->dbg->head;h!=NULL;h=h->next)
	{
		if(h->addr==(unsigned long long)addr)
		{
			if(!h->status) *(char *)addr=INT3_OPCODE;
			h->status=1;
			return 0;
		}
	}
	
	return -1;
}

/*
 * called from debuggers context
 */
int do_disable_breakpoint(struct exec_context *ctx, void *addr)
{
	
	
	for(struct breakpoint_info *h=ctx->dbg->head;h!=NULL;h=h->next)
	{	
		if(h->addr==(u64 )addr)
		{
			
			if (h->status)
			{
				*(char *)addr = PUSHRBP_OPCODE;
				h->status = 0;
			}	
			return 0;
		}
	}
	dprintk(" _______   Disable Breakpoints called   _______\n");
	
	return -1;
}

/*
 * called from debuggers context
 */ 
int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *x)
{	
	if(ctx->dbg==NULL)return -1;
	if(x==NULL)return -1;

	struct breakpoint_info *h=ctx->dbg->head;
	for(int i=0;i<ctx->dbg->total_bp;i++,h=h->next)
	{	int j=0;
		x[i].addr=h->addr;
		x[i].status=h->status;
		j++;
		x[i].num=h->num;
		
	}
	dprintk(" _______   Info Breakpoints called   _______\n");
	return ctx->dbg->total_bp;
}

/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	if(regs==NULL)return -1;
	dprintk(" _______   Do info registers called   _______\n");
	struct registers *child=ctx->dbg->child_reg;
	regs->r15=child->r15;
	regs->r14=child->r14;
	regs->r13=child->r13;
	regs->r12=child->r12;
	regs->r11=child->r11;
	regs->r10=child->r10;
	regs->r9=child->r9;
	regs->r8=child->r8;
	regs->rbp=child->rbp;
	regs->rdi=child->rdi;
	regs->rsi=child->rsi;
	regs->rdx=child->rdx;
	regs->rcx=child->rcx;
	regs->rbx=child->rbx;
	regs->rax=child->rax;
	regs->entry_rip=child->entry_rip;
	regs->entry_cs=child->entry_cs;
	regs->entry_rflags=child->entry_rflags;
	regs->entry_rsp=child->entry_rsp;
	regs->entry_ss=child->entry_ss;
	
	return 0;
}

/* 
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{
	
	return ctx->dbg->bt_count;
}

/*
 * When the debugger calls wait
 * it must move to WAITING state 
 * and its child must move to READY state
 */


s64 do_wait_and_continue(struct exec_context *ctx)
{
	//TODO:  wait and continue is very important
	dprintk(" _______   Schedule child   _______\n");

	ctx->state=WAITING;
	struct exec_context *p=get_ctx_list(),*child;
	
	for(int i=0;i<MAX_PROCESSES;i++)
	{	
		if(p[i].ppid==ctx->pid)
		{
			child=&p[i];
			break;
		}
	}
	child->state=READY;
	schedule(child);

	return  0;
}


