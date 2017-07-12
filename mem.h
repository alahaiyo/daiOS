
#ifndef __MEM_H_
#define __MEM_H_

#include "list.h"

struct kmem_cache{
	unsigned int obj_size;
	unsigned int obj_nr;
	unsigned int page_order;
	unsigned int flags;
	struct page *head_page;
	struct page *end_page;
	void *nf_block;
};

struct page {
	unsigned int vaddr;
	unsigned int flags;
	int order;
	struct kmem_cache *cachep;
	struct list_head list;//to string the buddy member
};

#endif
