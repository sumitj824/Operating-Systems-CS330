#include<types.h>
#include<mmap.h>
int present;

struct vm_area* create_vm_area(u64 start_addr, u64 end_addr, u32 flags, u32 mapping_type)
{
	struct vm_area *new_vm_area = alloc_vm_area();
	new_vm_area-> vm_end = end_addr;
	new_vm_area-> vm_start = start_addr;
	new_vm_area->mapping_type = mapping_type;
	new_vm_area-> access_flags = flags;
	return new_vm_area;
}
//ok
u64 * make_entry(struct exec_context *ctx, u64 addr, u64 error_code,u64 type)
{
	
	if(type==NORMAL_PAGE_MAPPING)  // * 4 level
	{
		// get base addr of pgdir
		u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
		u64 *entry;
		u64 *prev_entry;
		u64 pfn;
		// set User and Present flags
		// set Write flag if specified in error_code
		u64 ac_flags = 0x5 | (error_code & 0x2);
		
		// find the entry in page directory
		entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		prev_entry=entry;
		if((*entry & 0x1)==0) {
			// allocate PUD
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
		}else{
			// PGD->PUD Present, access it
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		prev_entry=entry;
		if((*entry & 0x1)==0) {
			// allocate PMD
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
		}else{
			// PUD->PMD Present, access it
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
		prev_entry=entry;
		if((*entry & 0x1)==0) {
			// allocate PLD 
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
		}else{
			// PMD->PTE Present, access it
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}
		entry = vaddr_base + ((addr & PTE_MASK) >> PTE_SHIFT);
		if((*entry & 0x1)==0)
		{
			pfn = os_pfn_alloc(USER_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
		}
		
		return prev_entry;
	}
	else  // TODO: 3 level  need to implement for hp
	{
		
		u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
		u64 *entry;
		u64 pfn;
		// set User and Present flags
		// set Write flag if specified in error_code
		u64 ac_flags = 0x5 | (error_code & 0x2);
		u64 * prev_entry;
		// find the entry in page directory
		entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		prev_entry=entry;

		if((*entry & 0x1)==0) {
			// allocate PUD
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
		}else{
			// PGD->PUD Present, access it
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		prev_entry=entry;

		if((*entry & 0x1)==0) {
			// allocate PTE
			pfn = os_pfn_alloc(OS_PT_REG);
			*entry = (pfn << PTE_SHIFT) | ac_flags;
			vaddr_base = osmap(pfn);
		}else{
			// PUD->PTE Present, access it
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}	

		entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);  //!doubt
		prev_entry=entry;
		// since this fault occured as frame was not present, we don't need present check here
		if((*entry & 0x1)==0)
		{	
			vaddr_base=os_hugepage_alloc();
			pfn = get_hugepage_pfn(vaddr_base);
			*entry = (pfn << HUGEPAGE_SHIFT) | ac_flags | (1<<7);
		}
		else present=1;
		return prev_entry;
	}


	

}


void free(struct exec_context *ctx,u64 addr,u64 type)
{	

	
	if(type==HUGE_PAGE_MAPPING)   // TODO: 3 level for hp need to change
	{
		u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
		u64 *entry;
		u64 pfn;
		entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		if((*entry & 1)==0) {
			return;
		}
		else {
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		if((*entry & 1)==0) {
			return;
		}
		else {
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		} return ;

		entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);  //!doubt
		if((*entry & 1)==0) {
			return;
		}
		else
		{	
			pfn=(*entry >> HUGEPAGE_SHIFT) & 0xFFFFFFFF;
			vaddr_base= (u64 *)(pfn*(1<<21));
			os_hugepage_free(vaddr_base);
			*entry=(*entry)^1;
		}
	}
	else  //* 4 level for normal pages
	{
		u64 *vaddr_base = (u64 *)osmap(ctx->pgd);
		u64 *entry;
		u64 pfn;
		entry = vaddr_base + ((addr & PGD_MASK) >> PGD_SHIFT);
		if((*entry & 1)==0) {
			return;
		}
		else
		{
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PUD_MASK) >> PUD_SHIFT);
		if((*entry & 1)==0) {
			return;
		}
		else {
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}

		entry = vaddr_base + ((addr & PMD_MASK) >> PMD_SHIFT);
		if((*entry & 1)==0) {
			return;
		}
		else{
			pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
			vaddr_base = (u64 *)osmap(pfn);
		}
		entry = vaddr_base + ((addr & PTE_MASK) >> PTE_SHIFT);
		if(*entry & 1)
		{
			pfn=((*entry) >>PTE_SHIFT)& 0xFFFFFFFF;
			*entry=(*entry)^1;
			os_pfn_free(USER_REG,pfn);
		}
	}
	
	//flush from TLB(cache)
	asm volatile (
	"invlpg (%0);" 
	:: "r"(addr) 
	: "memory"
	); 

}


void free_range(struct exec_context *ctx,u64 start,u64 end,u64 type)
{
	if(type==NORMAL_PAGE_MAPPING)
	{
		for(u64 i=start;i<end;i+=(1<<12))
		{
			free(ctx,i,type);
		}
	}
	else
	{
		for(u64 i=start;i<end;i+=(1<<21))
		{
			free(ctx,i,type);
		}
	}
}


int vm_area_pagefault(struct exec_context *ctx, u64 addr, int error_code)
{
	struct vm_area *v=ctx->vm_area->vm_next;
	while (v)
	{
		if(addr>= v->vm_start && addr< v->vm_end)break;
		v=v->vm_next;
	}
	if(v==NULL  || error_code&1 )return -1; 

	make_entry(ctx,addr,error_code,v->mapping_type);
	return 1;
}


void sorted_Insert(struct vm_area *dummy, struct vm_area *node)
{	
	struct vm_area *head=dummy->vm_next, *next,*cur;
	if(!head || head->vm_start>=node->vm_end )
	{
		node->vm_next=head;
		dummy->vm_next=node;
		// merge
		next=node->vm_next;
		if(next && node->vm_end==next->vm_start && node->access_flags==next->access_flags && node->mapping_type==next->mapping_type)
		{
			next->vm_start=node->vm_start;
			dummy->vm_next=next;
			dealloc_vm_area(node);
		}
	}
	else
	{
		while (head->vm_next!=NULL && head->vm_next->vm_start< node->vm_end)
		{
			head=head->vm_next;
		}
		node->vm_next=head->vm_next;
		head->vm_next=node;
		//merge

		next=head->vm_next;
		cur=head->vm_next;
		if(head->vm_end==next->vm_start && head->access_flags==next->access_flags && head->mapping_type==next->mapping_type)
		{
			head->vm_end=next->vm_end;
			head->vm_next=next->vm_next;
			dealloc_vm_area(next);
			next=head->vm_next;
			if(next!=NULL && head->vm_end==next->vm_start && head->access_flags==next->access_flags && head->mapping_type==next->mapping_type)
			{
				head->vm_end=next->vm_end;
				head->vm_next=next->vm_next;
				dealloc_vm_area(next);
			}
		}
		else if(cur->vm_next!=NULL && cur->vm_end==cur->vm_next->vm_start && cur->access_flags==cur->vm_next->access_flags && cur->mapping_type==cur->vm_next->mapping_type)
		{
			cur->vm_end=cur->vm_next->vm_end;
			cur->vm_next=cur->vm_next->vm_next;
			dealloc_vm_area(cur->vm_next);
		}
		
		

	}
}


long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{	
	if(length<=0)return -1;
	int page=(length+4096-1)/4096;
	if(!current->vm_area) //dummy node
	{
		current->vm_area=create_vm_area(MMAP_AREA_START,MMAP_AREA_START+4096,0x4,NORMAL_PAGE_MAPPING);
	}
	if(flags==MAP_FIXED)
	{
		if(!addr  || addr%4096)return -1;
		struct vm_area *v=current->vm_area->vm_next;
		while (v)
		{
			if(v->vm_start<=addr && addr<v->vm_end)return -1;
			if(v->vm_start<addr+page*4096 && addr+page*4096<=v->vm_end)return -1;
			v=v->vm_next;
		}
		v=create_vm_area(addr,addr+page*4096,prot,NORMAL_PAGE_MAPPING);
		sorted_Insert(current->vm_area,v);
		return addr;
	}

	if(!addr) 
	{
		struct vm_area *prev,*cur;
		prev=current->vm_area;
		cur=prev->vm_next;
		while (cur )
		{
			if(cur->vm_start-prev->vm_end>=page*4096)break;
			prev=cur;
			cur=cur->vm_next;
		}
		cur=create_vm_area(prev->vm_end,prev->vm_end+page*4096,prot,NORMAL_PAGE_MAPPING);
		sorted_Insert(current->vm_area,cur);
		return prev->vm_end;
	}
	else
	{
		addr=(addr+4096-1)/4096;
		addr=addr*4096;
		struct vm_area *v=current->vm_area->vm_next;
		int f=1;
		while (v)
		{
			if(v->vm_start<=addr && addr<v->vm_end)f=0;
			if(v->vm_start<addr +page*4096 && addr +page*4096 <=v->vm_end)f=0;
			v=v->vm_next;
		}
		if(f)
		{
			v=create_vm_area(addr,addr+page*4096,prot,NORMAL_PAGE_MAPPING);
			sorted_Insert(current->vm_area,v);
			return addr;
		}
		else
		{
			struct vm_area *prev,*cur;
			prev=current->vm_area;
			cur=prev->vm_next;
			while (!cur && cur->vm_start-prev->vm_end<page*4096)
			{
				prev=cur;
				cur=cur->vm_next;
			}
			cur=create_vm_area(prev->vm_end,prev->vm_end+page*4096,prot,NORMAL_PAGE_MAPPING);
			sorted_Insert(current->vm_area,cur);
			return prev->vm_end;
		}
	}

}


int vm_area_unmap(struct exec_context *current, u64 addr, int length)
{	
	if(length<=0)return -1;
	if(addr%4096)return -1;// not page aligned
	struct vm_area *cur=current->vm_area->vm_next,*prev=current->vm_area;
	int x;
	while (cur)
	{	
		if(cur->mapping_type==NORMAL_PAGE_MAPPING)
		{
			x=(length+4096-1)/4096;
			x=x*4096;
		}
		else
		{
			x=(length+(1<<21)-1)/(1<<21);
			x=x*(1<<21);
		}

		if(cur->vm_start>=addr && cur->vm_end<=addr+x)
		{	
			free_range(current,cur->vm_start,cur->vm_end,cur->mapping_type);	
			prev->vm_next=cur->vm_next;
			dealloc_vm_area(cur);
			cur=prev->vm_next;
			continue;
		}
		else if(addr>cur->vm_start && addr+x<cur->vm_end)
		{
			free_range(current,addr,addr+x,cur->mapping_type);
			struct vm_area *v=create_vm_area(addr+x,cur->vm_end,cur->access_flags,cur->mapping_type);
			cur->vm_end=addr;
			v->vm_next=cur->vm_next;
			cur->vm_next=v;
		}
		else if(cur->vm_end<=addr+x && cur->vm_end>addr)
		{
			free_range(current,addr,cur->vm_end,cur->mapping_type);
			cur->vm_end=addr;
		}
		else if(cur->vm_start>=addr && cur->vm_start<addr+x)
		{
			free_range(current,cur->vm_start,addr+x,cur->mapping_type);
			cur->vm_start=addr+x;
		}
		prev=cur;
		cur=cur->vm_next;
	}
	

	return 0;
}

int overlap(int start1 ,int end1, int start2,int end2)
{
	return (end1 >start2) && (end2>start1);
}

void copy_php(struct exec_context *current,u64 start ,u64 end,u64 prot)
{
	for(u64 i=start;i<end;i+=1<<21)
	{	
		present=0;
		u64 *entry=make_entry(current,i,prot,NORMAL_PAGE_MAPPING);
		u64	pfn=((*entry) >>PTE_SHIFT)& 0xFFFFFFFF;
		u64 *pte_entry=osmap(pfn);
		u64 hp_addr=(u64) os_hugepage_alloc();
		for(u64 j=0;j<512;j++)
		{
			if(*pte_entry & 1)
			{
				u64 page_pfn = (*pte_entry >> PTE_SHIFT) & 0xFFFFFFFF;
				u64 offset;
				u64 page_addr = i + (j << PAGE_SHIFT);
				memcpy((char *)(hp_addr + (j << PAGE_SHIFT)),(char *)page_addr,1<<12);
				os_pfn_free(USER_REG, page_pfn);
				free(current,page_addr,NORMAL_PAGE_MAPPING);
			}
			pte_entry++;
		}
        os_pfn_free(OS_PT_REG, pfn);
		u64 huge_pfn=get_hugepage_pfn((void *)hp_addr);
        *entry = (huge_pfn << HUGEPAGE_SHIFT) | (0x5 |(prot & 0x2)) | (1 << 7);
		if(!present)
		{
			free(current,i,HUGE_PAGE_MAPPING);
		}
	}




}


long vm_area_make_hugepage(struct exec_context *current, void *addr, u32 length, u32 prot, u32 force_prot)
{	
	if(!current || !addr || length<=0)return -EINVAL;
	struct vm_area *prev=current->vm_area,*cur=prev->vm_next;
	u64 x=(u64)addr;
	u64 l=length;
	while (cur)
	{
		if(cur->vm_start!=prev->vm_end && overlap(prev->vm_end,cur->vm_start,x,x+length))
		return -ENOMAPPING;
		prev=cur;
		cur=cur->vm_next;
	}
	cur=current->vm_area->vm_next;
	while (cur)
	{
		if(cur->mapping_type==HUGE_PAGE_MAPPING && overlap(cur->vm_start,cur->vm_end,x,x+length))
		return -EVMAOCCUPIED;
		cur=cur->vm_next;
	}

	if(!force_prot)
	{
		cur=current->vm_area->vm_next;
		while (cur)
		{
			if(overlap(cur->vm_start,cur->vm_end,x,x+length) && cur->access_flags!=prot)
			return -EDIFFPROT;
			cur=cur->vm_next;
		}
	}
	
	l=(x+l);
	l/=(1<<21);
	l*=(1<<21);
	x=(x+(1<<21)-1)/(1<<21);
	x=x*(1<<21);
	prev=current->vm_area;
	cur=prev->vm_next;
	while (cur)
	{
		if(cur->vm_start>=x && cur->vm_end<=l)
		{
			prev->vm_next=cur->vm_next; 
			dealloc_vm_area(cur);
			cur=prev->vm_next;
		}
		else  
		{	
			if(cur->vm_end>x && cur->vm_start<=x)
			{
				if(cur->vm_end>l)
				{
					struct vm_area *v=create_vm_area(l,cur->vm_end,cur->access_flags,cur->mapping_type);
					v->vm_next=cur->vm_next;
					cur->vm_next=v;
				}
				if(cur -> vm_start != x){
					cur->vm_end=x;
					prev=cur;
					cur=cur->vm_next;
				}
				else{
					prev -> vm_next = cur -> vm_next;
					dealloc_vm_area(cur);
					cur = prev -> vm_next;
				}
			}
			else{	
				prev=cur;
				cur=cur->vm_next;
			}
		}
	}
	copy_php(current,x,l,prot);
	cur=create_vm_area(x,l,prot,HUGE_PAGE_MAPPING);
	sorted_Insert(current->vm_area,cur);
	return x;
}


void copy_hpp(struct exec_context *current,u64 start ,u64 end,u64 prot)
{
	for(u64 i=start;i<end;i+=1<<21)
	{	
		present=0;
		u64 *entry=make_entry(current,i,prot,HUGE_PAGE_MAPPING);
		if((*entry & 0x1) && (*entry & (1<<7)))
		{
		    u64 ac_flags = 0x5 | (*entry & 0x2);
			u64 j;
			u64 hp_addr=(((*entry) >> HUGEPAGE_SHIFT) & 0xFFFFFFFF) << HUGEPAGE_SHIFT;
			u64 pfnd = os_pfn_alloc(OS_PT_REG);
            u64 *entry2 = osmap(pfnd);
        	char *pfn_os_addr;
			for(u64 j=0;j<512;j++)
			{
				u64 pfn = os_pfn_alloc(USER_REG);
				pfn_os_addr = osmap(pfn);
				memcpy(pfn_os_addr,(char *)(hp_addr + (j << PTE_SHIFT)),1<<12);
				*entry2 = (pfn << PTE_SHIFT) | ac_flags;
				entry2++;
			}
			os_hugepage_free((void *)hp_addr);
			asm volatile (
			"invlpg (%0);" 
			:: "r"(i) 
			: "memory"
			); 
			*entry = (pfnd << PTE_SHIFT) | ac_flags;

		}
		else
		{
			free(current,i,HUGE_PAGE_MAPPING);
		}
		
	}
	
}


int vm_area_break_hugepage(struct exec_context *current, void *addr, u32 length)
{	
	struct vm_area *prev=current->vm_area,*cur=prev->vm_next,*v,*v1;
	u64 x=(u64)addr;
	u64 l=x+length;
	if(!addr || length<=0 || (length%(1<<21)) ||(x%(1<<21)) )return -EINVAL;

	while (cur)
	{
		if(cur->mapping_type!=HUGE_PAGE_MAPPING)
		{
			cur=cur->vm_next;
			continue;
		}
		if(cur->vm_start>=x && cur->vm_end<=l )
		{
			prev->vm_next=cur->vm_next;
			v=create_vm_area(cur->vm_start,cur->vm_end,cur->access_flags,NORMAL_PAGE_MAPPING); 
			copy_hpp(current,cur->vm_start,cur->vm_end,cur->access_flags);
			dealloc_vm_area(cur);
			sorted_Insert(current->vm_area,v);
	
		}
		else if(cur->vm_start>=x && cur->vm_start<l)
		{
			v=create_vm_area(cur->vm_start,l,cur->access_flags,NORMAL_PAGE_MAPPING);
			copy_hpp(current,cur->vm_start,l,cur->access_flags);
			cur->vm_start=l;
			sorted_Insert(current->vm_area,v);
		}
		else if(cur->vm_end>x && cur->vm_end<l)
		{
			v=create_vm_area(cur->vm_end,l,cur->access_flags,NORMAL_PAGE_MAPPING);
			copy_hpp(current,cur->vm_end,l,cur->access_flags);

			cur->vm_end=x;
			sorted_Insert(current->vm_area,v);
		}
		else if(x>=cur->vm_start && l<cur->vm_end)
		{
			v=create_vm_area(x,l,cur->access_flags,NORMAL_PAGE_MAPPING);
			copy_hpp(current,x,l,cur->access_flags);
			v1=create_vm_area(l,cur->vm_end,cur->access_flags,HUGE_PAGE_MAPPING);
			cur->vm_end=x;
			v->vm_next=v1;
			v1->vm_next=cur->vm_next;
			cur->vm_next=v;
		}
		cur=cur->vm_next;
	}
	return 0;
}




                    
