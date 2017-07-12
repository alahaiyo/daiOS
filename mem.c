/*
mem.c:
Copyright (C) 2009  david leels <davidontech@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see http://www.gnu.org/licenses/.
*/

#include "list.h"
#include "mem.h"

#define	NULL	((void *)0)

/* 内存管理大小 250MB */
#define _MEM_END	0x8fc00000
#define _MEM_START	0x80200000


#define PAGE_SHIFT	(12)  //page size = 4KB  12bit
#define PAGE_SIZE	(1<<PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1)) //0xffff f000 

#define KERNEL_MEM_END	(_MEM_END)
/*the bigin and end of the kernel mem which is needed to be paged.*/
#define KERNEL_PAGING_START	((_MEM_START+(~PAGE_MASK))&((PAGE_MASK)))
#define	KERNEL_PAGING_END	(((KERNEL_MEM_END-KERNEL_PAGING_START)/(PAGE_SIZE+sizeof(struct page)))*(PAGE_SIZE)+KERNEL_PAGING_START)

/*page number in need */
#define KERNEL_PAGE_NUM	((KERNEL_PAGING_END-KERNEL_PAGING_START)/PAGE_SIZE)//一共需要多少个page结构体来表示
/*the start and end of the page structure should be storaged in.*/
#define KERNEL_PAGE_END	_MEM_END
#define KERNEL_PAGE_START	(KERNEL_PAGE_END-KERNEL_PAGE_NUM*sizeof(struct page))//page结构体的起始地址


/*page flags*/
#define PAGE_AVAILABLE		0x00
#define PAGE_DIRTY			0x01
#define PAGE_PROTECT		0x02
#define PAGE_BUDDY_BUSY		0x04
#define PAGE_IN_CACHE		0x08


/*这里我设置的最大分配的budy大小是2^12 = 4096PAGE = 16MB*/
#define MAX_BUDDY_PAGE_NUM	(12)	//finally I desided to choose a fixed value,which means users could alloc 1M space at most,those who need more than 1M should change the macro to reserve enough space for private use.

#define AVERAGE_PAGE_NUM_PER_BUDDY	(KERNEL_PAGE_NUM/MAX_BUDDY_PAGE_NUM) //最少的budy数量
#define PAGE_NUM_FOR_EACH_BUDDY(j) ((AVERAGE_PAGE_NUM_PER_BUDDY>>(j))*(1<<(j)))
#define PAGE_NUM_FOR_MAX_BUDDY	((1<<MAX_BUDDY_PAGE_NUM)-1) //0x0000 0fff

struct list_head page_buddy[MAX_BUDDY_PAGE_NUM];//定义链表头，最大是12阶，所以就12个链表就够了


struct page *virt_to_page(unsigned int addr)
{
	unsigned int i;
	i=((addr)-KERNEL_PAGING_START)>>PAGE_SHIFT;
	if(i>KERNEL_PAGE_NUM)
		return NULL;
	return (struct page *)KERNEL_PAGE_START+i;
}

/*初始化各个链表头*/
void init_page_buddy(void)
{
	int i;
	for(i=0;i<MAX_BUDDY_PAGE_NUM;i++){
		INIT_LIST_HEAD(&page_buddy[i]);
	}
}


/*对各个buddy进行初始化*/
void init_page_map(void)
{
	int i;
	struct page *pg=(struct page *)KERNEL_PAGE_START;
	init_page_buddy();
	for(i=0;i<(KERNEL_PAGE_NUM);pg++,i++){
/*fill struct page first*/
		pg->vaddr=KERNEL_PAGING_START+i*PAGE_SIZE;	//初始化每一个page的起始地址
		pg->flags=PAGE_AVAILABLE;  //初始化flag为可用状态
		INIT_LIST_HEAD(&(pg->list));

	
/*make the memory max buddy as possible*/
		if(i<(KERNEL_PAGE_NUM&(~PAGE_NUM_FOR_MAX_BUDDY))){	//i < 最大的buddy数量，初始化时，只有最大buddy和最小buddy
/*the following code should be dealt carefully,we would change the order field of a head struct page to the corresponding order,and change others to -1*/
			if((i&PAGE_NUM_FOR_MAX_BUDDY)==0){
				pg->order=MAX_BUDDY_PAGE_NUM-1; //设置每一个最大buddy的开始的哪个page的order
			}else{
				pg->order=-1;//剩下的page的oder设置为-1
			}
			list_add_tail(&(pg->list),&page_buddy[MAX_BUDDY_PAGE_NUM-1]);
/*the remainder not enough to merge into a max buddy is done as min buddy*/
		}else{
			pg->order=0; //剩下的都是最小的buddy，也就是一个page，order都设置为0
			list_add_tail(&(pg->list),&page_buddy[0]);
		}

	}
	
}

/*we can do these all because the page structure that represents one page aera is continuous*/
#define BUDDY_END(x,order)	((x)+(1<<(order))-1)
#define NEXT_BUDDY_START(x,order)	((x)+(1<<(order)))
#define PREV_BUDDY_START(x,order)	((x)-(1<<(order)))


/*the logic of this function seems good,no bug reported yet*/
struct page *get_pages_from_list(int order){
	unsigned int vaddr;
	int neworder=order;
	struct page *pg,*ret;
	struct list_head *tlst,*tlst1;
	for(;neworder<MAX_BUDDY_PAGE_NUM;neworder++){//先从所要的大小的order开始找，一直到系统最大的order
		if(list_empty(&page_buddy[neworder])){
			continue; //如果需要的order链表没有空闲的buddy，那么搜索大一号的
		}else{
			pg=list_entry(page_buddy[neworder].next,struct page,list);
			tlst=&(BUDDY_END(pg,neworder)->list);
			tlst->next->prev=&page_buddy[neworder];
			page_buddy[neworder].next=tlst->next;  //取出链表内的内容，链表重新连接好
			goto OUT_OK;
		}
	}
	printk("Error: Can't get pages from list!\r\n");
	return NULL;
//如果是从大号的链表中取出来的，那么就要进行善后处理	
OUT_OK:
	for(neworder--;neworder>=order;neworder--){
		
		tlst1=&(BUDDY_END(pg,neworder)->list);
		tlst=&(pg->list);

		pg=NEXT_BUDDY_START(pg,neworder);
		list_entry(tlst,struct page,list)->order=neworder;

		list_add_chain_tail(tlst,tlst1,&page_buddy[neworder]);
	}
	pg->flags|=PAGE_BUDDY_BUSY;
	pg->order=order;
	return pg;
}



void put_pages_to_list(struct page *pg,int order){
	struct page *tprev,*tnext;
	if(!(pg->flags&PAGE_BUDDY_BUSY)){
		printk("something must be wrong when you see this message,that probably means you are forcing to release a page that was not alloc at all\n");
		return;
	}
	pg->flags&=~(PAGE_BUDDY_BUSY);

	for(;order<MAX_BUDDY_PAGE_NUM;order++){
		tnext=NEXT_BUDDY_START(pg,order);
		tprev=PREV_BUDDY_START(pg,order);
		if((!(tnext->flags&PAGE_BUDDY_BUSY))&&(tnext->order==order)){
			pg->order++;
			tnext->order=-1;
			list_remove_chain(&(tnext->list),&(BUDDY_END(tnext,order)->list));
			BUDDY_END(pg,order)->list.next=&(tnext->list);
			tnext->list.prev=&(BUDDY_END(pg,order)->list);
			continue;
		}else if((!(tprev->flags&PAGE_BUDDY_BUSY))&&(tprev->order==order)){
			pg->order=-1;
			
			list_remove_chain(&(pg->list),&(BUDDY_END(pg,order)->list));
			BUDDY_END(tprev,order)->list.next=&(pg->list);
			pg->list.prev=&(BUDDY_END(tprev,order)->list);
			
			pg=tprev;
			pg->order++;
			continue;
		}else{
			break;
		}
	}
	
	list_add_chain(&(pg->list),&((tnext-1)->list),&page_buddy[order]);
}


void *page_address(struct page *pg)
{
	return (void *)(pg->vaddr);
}

struct page *alloc_pages(unsigned int flag,int order)
{
	struct page *pg;
	int i;
	pg = get_pages_from_list(order); //从链表中取出需要的页，参数是order
	if(pg == NULL) {
		printk("alloc pages failed!\r\n");
		return NULL;
	}
	for(i = 0; i < (1 << order); i++) {
		(pg+i)->flags |= PAGE_DIRTY; //获取出来的每个页都标记为脏
	}
	return pg;
}

void free_pages(struct page *pg,int order)
{
	int i;
	for(i=0;i<(1<<order);i++){
		(pg+i)->flags&=~PAGE_DIRTY;
	}
	put_pages_to_list(pg,order);
}

/*
* 获得（2^order）个page的内存空间
*/
void *get_free_pages(unsigned int flag,int order){
	struct page * page;
	page = alloc_pages(flag, order);//分配页
	if (!page)
		return NULL;
	return	page_address(page);
}


void put_free_pages(void *addr,int order)
{
	free_pages(virt_to_page((unsigned int)addr),order);
}



#define KMEM_CACHE_DEFAULT_ORDER	(0)
#define KMEM_CACHE_MAX_ORDER		(5)			//cache can deal with the memory no less than 32*PAGE_SIZE
#define KMEM_CACHE_SAVE_RATE		(0x5a)
#define KMEM_CACHE_PERCENT			(0x64)
#define KMEM_CACHE_MAX_WAST			(PAGE_SIZE-KMEM_CACHE_SAVE_RATE*PAGE_SIZE/KMEM_CACHE_PERCENT)//0.1*4KB


int find_right_order(unsigned int size){
	int order;
	
	for(order = 0; order <= KMEM_CACHE_MAX_ORDER; order++){
		
		if(size <= (KMEM_CACHE_MAX_WAST)*(1<<order)){//size大小是否在这个buddy大小的一个百分比之内。  <10%>
			
			return order;
		}
	}
	
	if(size > (1<<order)) //如果size 大于最大可分配的大小，返回错误。最大是 2^5 = 32 pages
		return -1;
	return order;
}


int kmem_cache_line_object(void *head,unsigned int size,int order)
{
	void **pl;
	char *p;
	int i,s;
	
	pl = (void **)head;
	p = (char *)head+size;
	s = PAGE_SIZE*(1<<order);
	
	for(i = 0; s > size; i++,s -= size){
		*pl = (void *)p;//只指向下一个内存块
		pl = (void **)p;
		p = p+size;
	}
	
	if(s == size)
		i++;
	return i;
}

struct kmem_cache *kmem_cache_create(struct kmem_cache *cache,unsigned int size,unsigned int flags)
{
	void **nf_block = &(cache->nf_block);

	int order = find_right_order(size);
	
	if(order == -1)
		return NULL;
	if((cache->head_page = alloc_pages(0,order)) == NULL) {//ok,从buddy重分配处内存，并获得起始page的地址
		return NULL;
	}
	*nf_block=page_address(cache->head_page); //获得第一个page的地址

	cache->obj_nr=kmem_cache_line_object(*nf_block,size,order);//初始化一件分配的内存。返回内存块的数量
	cache->obj_size=size;
	cache->page_order = order;
	cache->flags=flags;
	cache->end_page=BUDDY_END(cache->head_page,order);
	cache->end_page->list.next=NULL;

	return cache;
}

/*FIXME:I dont understand it now*/
void kmem_cache_destroy(struct kmem_cache *cache){
	int order=cache->page_order;
	struct page *pg=cache->head_page;
	struct list_head *list;
	while(1){
		list=BUDDY_END(pg,order)->list.next;
		free_pages(pg,order);
		if(list){
			pg=list_entry(list,struct page,list);
		}else{
			return;
		}
	}
}

void kmem_cache_free(struct kmem_cache *cache,void *objp){
	*(void **)objp=cache->nf_block;
	cache->nf_block=objp;
	cache->obj_nr++;
}


void *kmem_cache_alloc(struct kmem_cache *cache,unsigned int flag){
	void *p;
	struct page *pg;
	if(cache==NULL)
		return NULL;
	void **nf_block=&(cache->nf_block);
	unsigned int *nr=&(cache->obj_nr);
	int order=cache->page_order;

	if(!*nr){
		if((pg=alloc_pages(0,order))==NULL)
			return NULL;
		*nf_block=page_address(pg);
		cache->end_page->list.next=&pg->list;
		cache->end_page=BUDDY_END(pg,order);
		cache->end_page->list.next=NULL;
		*nr+=kmem_cache_line_object(*nf_block,cache->obj_size,order);
	}

	(*nr)--;
	p=*nf_block;
	*nf_block=*(void **)p;
	pg=virt_to_page((unsigned int)p);
	pg->cachep=cache;		//doubt it??? 
	return p;
}



#define KMALLOC_BIAS_SHIFT			(5)				//32byte minimal
#define KMALLOC_MAX_SIZE			(4096)
#define KMALLOC_MINIMAL_SIZE_BIAS	(1<<(KMALLOC_BIAS_SHIFT))//32
#define KMALLOC_CACHE_SIZE			(KMALLOC_MAX_SIZE/KMALLOC_MINIMAL_SIZE_BIAS)//128
struct kmem_cache kmalloc_cache[KMALLOC_CACHE_SIZE] = {{0,0,0,0,NULL,NULL,NULL},};
#define kmalloc_cache_size_to_index(size)	((((size))>>(KMALLOC_BIAS_SHIFT)))  //32字节为单位，看需要什么样的cache


int kmalloc_init(void)
{
	int i=0;
	
	init_page_map();
	
	for(i = 0; i < KMALLOC_CACHE_SIZE; i++){   //  最大到128*32 bytes = 4KB
		if(kmem_cache_create(&kmalloc_cache[i],(i+1)*KMALLOC_MINIMAL_SIZE_BIAS,0) == NULL) {
			return -1;
		}
	}
	return 0;
}
//kmalloc 用于分配任意大小的内存，要到slab内存池中去分配
void *kmalloc(unsigned int size)
{
	int index=kmalloc_cache_size_to_index(size);
	if(index>=KMALLOC_CACHE_SIZE)
		return NULL;
	return kmem_cache_alloc(&kmalloc_cache[index],0);
}

void kfree(void *addr)
{
	struct page *pg;
	pg=virt_to_page((unsigned int)addr);
	kmem_cache_free(pg->cachep,addr);
}
