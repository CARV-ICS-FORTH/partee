/*
 * Copyright (c) 1999-2001
 *      The Regents of the University of California.  All rights reserved.
 * Edits for 64bit port: Copyright (c) 2010
 *      FORTH-ICS. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
#include <limits.h>
#include <pthread.h>
#include <atomic.h>

typedef __rcintptr pageid;

static size_t total_page_count; /* total pages allocated */

#define PAGE_GROUP_SIZE ( 1 + (total_page_count >> 7) )

#if 0
#define FREEPAGE ( (region_t)-1 ) /* Id of a free page */
#else /* if 0 */
#define FREEPAGE (&zeroregion)
#endif /* if 0 */
#ifdef NMEMDEBUG
#define ASSERT_FREE(p)
#define ASSERT_INUSE(p, r)
#else /* ifdef NMEMDEBUG */
#define ASSERT_FREE(p) assert(regionof(p) == FREEPAGE)
#ifdef DUPLICATES
#define ASSERT_INUSE(p, r) assert(regionof(p) == r->base)
#else /* ifdef DUPLICATES */
#define ASSERT_INUSE(p, r) assert(regionof(p) == r)
#endif /* ifdef DUPLICATES */
#endif /* ifdef NMEMDEBUG */

/* Page allocator for region-based memory management */
/* TBD: special free list for size == K ?? */

#define PAGECOUNTBITS (CHAR_BIT * sizeof(pageid) - 1)

volatile unsigned int freepages_lock = 0;

#define get_next_random_list(total_lists) (rand_r(&myseed) % total_lists)

__thread unsigned int list_id;
struct page {
	unsigned int list_id;

	/* Next page in region or in free list */
	struct page *next;

	/* number of pages in this allocation unit. Negative for free pages. */
	pageid pagecount: PAGECOUNTBITS;

	unsigned int free : 1;
	char         *available; /* next available location on the page */

	/* Only in free pages not in the single_pages list */
	struct page *previous;
};
struct list_pages {
	struct page           *pages;
	volatile unsigned int page_count;
	volatile int          lock;
};
struct page* alloc_single_page(struct page *next);

void free_single_page(region_t r, struct page *p);
void add_single_pages(struct page *base);

struct page* alloc_pages(int n, struct page *next);

void free_pages(region_t r, struct page *p);

#define MAXLISTS 16

/* lists of free individual pages */
struct list_pages single_pages[MAXLISTS];

/* free pages (not including those in single_pages) */
/*struct page *unused_pages;*/

#ifdef __i386__

/* Thomas Wang's 32 Bit Mix Function:
 * http://www.cris.com/~Ttwang/tech/inthash.htm */
inline unsigned int Hash(unsigned int key)
{
	key += ~(key << 15);
	key ^= (key >> 10);
	key += (key << 3);
	key ^= (key >> 6);
	key += ~(key << 11);
	key ^= (key >> 16);
	return key;
}

#endif /* ifdef __i386__ */

#ifdef __x86_64__
/* Thomas Wang's 64 bit Mix Function:
 * http://www.cris.com/~Ttwang/tech/inthash.htm */
inline unsigned int Hash(unsigned long key)
{
	key += ~(key << 32);
	key ^= (key >> 22);
	key += ~(key << 13);
	key ^= (key >> 8);
	key += (key << 3);
	key ^= (key >> 15);
	key += ~(key << 27);
	key ^= (key >> 31);
	return key;
}

#endif /* ifdef __x86_64__ */

region_t *__rcregionmap[RMAXMAP];
static region_t* get_or_alloc_regionmap(pageid map, pageid page)
{
	while (page >= RMAXPAGE) {
		map++;
		page -= RMAXPAGE;
	}
	if (__rcregionmap[map] == 0)
		__rcregionmap[map] = malloc( RMAXPAGE * sizeof(region_t) );
	assert(__rcregionmap[map]);
	return &__rcregionmap[map][page];
}

static void init_pages(void)
{
	int i;

	bzero( __rcregionmap, sizeof(__rcregionmap) );
	for (i = 0; i < MAXLISTS; i++) {
		bzero( &single_pages[i], sizeof(struct list_pages) );
	}
}

#undef USE_MMAP
#ifdef USE_MMAP
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#if !defined (MAP_ANONYMOUS) && defined (MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif /* if !defined (MAP_ANONYMOUS) && defined (MAP_ANON) */

#undef MAP_ANONYMOUS

#ifndef MAP_ANONYMOUS

static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */

#define MMAP(addr, size, prot, flags)                                      \
	( (dev_zero_fd < 0 ? dev_zero_fd = open("/dev/zero", O_RDWR) : 0), \
	  mmap( (addr), (size), (prot), (flags), dev_zero_fd, 0 ) )

#else /* ifndef MAP_ANONYMOUS */

#define MMAP(addr, size, prot, flags) \
	( mmap( (addr), (size), (prot), (flags) | MAP_ANONYMOUS, -1, 0 ) )

#endif /* ifndef MAP_ANONYMOUS */

struct page* region_get_mem(size_t s)
{
	struct page *newp, *tmp;

#if 0
	s = ALIGN(s, 65536);
#endif /* if 0 */

	newp = (struct page*)MMAP(0, s, PROT_READ | PROT_WRITE, MAP_PRIVATE);

	if (!newp)
		return NULL;

	if (PALIGN(newp, RPAGESIZE) != newp)
		abort();

	/* Add the new memory to unused_pages */
#ifndef NMEMDEBUG
	set_region_range(newp, (char*)newp + s, FREEPAGE);
#endif /* ifndef NMEMDEBUG */
	total_page_count += s >> RPAGELOG; /* This should be atomic */
	newp->pagecount   = s >> RPAGELOG;
	newp->free        = 1;

	return newp;
}

#else /* ifdef USE_MMAP */

/* region_get_mem will never allocate less than MINIMUM_MEM_REQUEST bytes.
 *        It wastes RPAGESIZE bytes, so there is an overhead of
 *        RPAGESIZE / MINIMUM_MEM_REQUEST
 */
#define MINIMUM_MEM_REQUEST (PAGE_GROUP_SIZE * K * RPAGESIZE)

struct page* region_get_mem(size_t s)
{
	size_t      request_bytes;
	void        *mem;
	struct page *newp;

	/* Don't get less than K * RPAGESIZE extra memory (K * RPAGESIZE
	 *        is the minimum useful size for something on unused_pages) */
	if ( (unsigned long)s + (unsigned long)K * RPAGESIZE <
	     (unsigned long)MINIMUM_MEM_REQUEST )
		request_bytes = MINIMUM_MEM_REQUEST;
	else
		request_bytes = s;

	mem = malloc(request_bytes + RPAGESIZE);
	if (!mem)
		return NULL;
	newp = PALIGN(mem, RPAGESIZE);
	if (mem == newp) /* Maybe we were lucky! */
		request_bytes += RPAGESIZE;

#ifndef NMEMDEBUG
	set_region_range(newp, (char*)newp + request_bytes, FREEPAGE);
#endif /* ifndef NMEMDEBUG */
	total_page_count += request_bytes >> RPAGELOG; /* this should be atomic
	                                                * */
	newp->pagecount   = request_bytes >> RPAGELOG;
	newp->free        = 1;

	return newp;
}

#endif /* ifdef USE_MMAP */

/* Page to region map management */
/* ----------------------------- */

static void set_page_region(pageid mapnb, pageid pagenb, region_t r)
{
	region_t *rp = get_or_alloc_regionmap(mapnb, pagenb);

	assert(rp);
	*rp = r;
}

#define page_region(mapnb, \
	            pagenb) ( *( get_or_alloc_regionmap( (mapnb), (pagenb) ) ) )

void set_region(struct page *p, int npages, region_t r)
{
	pageid pnb = PAGENB(p);
	pageid mnb = MAPNB(p);

	while (npages-- > 0)
		set_page_region(mnb, pnb++, r);
}

/* Mark the memory range from 'from' (inclusive) to 'to' (exclusive)
 *        as belonging to region with id 'rid' */
void set_region_range(void *from, void *to, region_t r)
{
	pageid firstp = PAGENB( (pageid)from ), firstm = MAPNB( (pageid)from ),
	       lastp  = PAGENB( (pageid)to - 1 ),
	       lastm  = MAPNB( (pageid)to - 1 );

	while (firstm < lastm || firstp <= lastp)
		if (firstp >= RMAXPAGE) {
			firstm++;
			firstp -= RMAXPAGE;
		}

	set_page_region(firstm, firstp++, r);
}

/* Multi-page allocation management */
/* -------------------------------- */

struct page* alloc_split(struct page *split, int n, struct page *next)
{
	/* Assumes freepages_lock held */
#ifndef NMEMDEBUG

	/* These pages had better be free */
	pageid i, pnb = PAGENB(split);
	pageid mnb = MAPNB(split);

	assert(split->pagecount >= n);
	for (i = pnb; i < pnb + split->pagecount; i++)
		assert(page_region(mnb, i) == FREEPAGE);
#endif /* ifndef NMEMDEBUG */
	if (split->pagecount > n) {
		/* Keep first part of block */
		split->pagecount -= n;
		/* Return latter part of block */
		split =
		        (struct page*)( (char*)split +
		                        (split->pagecount << RPAGELOG) );

		/* Update the by address list */
	}
	split->next      = next;
	split->pagecount = n;
	split->free      = 0;
	/*split->list_id = Hash(pthread_self())%MAXLISTS;*/
	split->list_id = list_id % MAXLISTS;

	return split;
}

struct page* alloc_new(int n, struct page *next)
{
	/* Assumes freepages_lock held */
	struct page *newp = region_get_mem( (INT_PTR)n << RPAGELOG );

	if (!newp) {
		if (nomem_h)
			nomem_h();
		abort();
	}
	assert( !( (long)newp & (RPAGESIZE - 1) ) );
	/*newp->list_id = Hash(pthread_self())%MAXLISTS;*/
	newp->list_id = list_id % MAXLISTS;
	/* region_get_mem may get us more memory than we asked for */
	return alloc_split(newp, n, next);
}

struct page* alloc_pages(int n, struct page *next)
{
	/* pthread_t pt = pthread_self(); */
	struct page *ret_val, *p = NULL;

	assert(n >= K);
	list_id = get_next_random_list(MAXLISTS);
	while (try_lock(&single_pages[list_id % MAXLISTS].lock) == 1)
		list_id = get_next_random_list(MAXLISTS);
	/*if( n > single_pages[Hash(pt)%MAXLISTS].page_count ){*/
	if (n > single_pages[list_id % MAXLISTS].page_count)
		p = alloc_new(n + PAGE_GROUP_SIZE, NULL);
	add_single_pages(p);
	/*ret_val = single_pages[Hash(pt)%MAXLISTS].pages;*/
	/*single_pages[Hash(pt)%MAXLISTS].pages =*/
	/*single_pages[Hash(pt)%MAXLISTS].pages->next;*/
	ret_val =
	        single_pages[list_id % MAXLISTS].pages;
	single_pages[list_id %
	             MAXLISTS].pages =
	        single_pages[list_id % MAXLISTS].pages->next;
	ret_val->next = next;
	/*single_pages[Hash(pt)%MAXLISTS].page_count -= n;*/
	single_pages[list_id % MAXLISTS].page_count -= n;
	/*release_spinlock( &single_pages[Hash(pt)%MAXLISTS].lock );*/
	release_spinlock(&single_pages[list_id % MAXLISTS].lock);
	/*list_id++;*/

	return ret_val;
}

void free_pages(region_t r, struct page *p)
/* Assumes freepages_lock held */
{
#ifndef NMEMDEBUG

	pageid i;
	pageid pnb = PAGENB(p);
	pageid mnb = MAPNB(p);

	for (i = pnb; i < pnb + p->pagecount; i++) {
		assert(page_region(mnb, i) == r);
		set_page_region(mnb, i, FREEPAGE);
	}
#endif /* ifndef NMEMDEBUG */

	list_id = get_next_random_list(MAXLISTS);
	while (try_lock(&single_pages[list_id % MAXLISTS].lock) == 1)
		list_id = get_next_random_list(MAXLISTS);
	p->next                           = single_pages[list_id].pages;
	single_pages[list_id].pages       = p;
	single_pages[list_id].page_count += p->pagecount;
	release_spinlock(&single_pages[list_id].lock);

	/*acquire_spinlock2( &single_pages[p->list_id].lock );*/
	/*p->next = single_pages[p->list_id].pages;*/
	/*single_pages[p->list_id].pages = p;*/
	/*single_pages[p->list_id].page_count += p->pagecount;*/
	/*release_spinlock( &single_pages[p->list_id].lock );*/

	/*p->next = single_pages[pt%MAXLISTS].pages;*/
	/*single_pages[pt%MAXLISTS].pages = p;*/
	/*single_pages[pt%MAXLISTS].page_count++;*/
	/*for( i = 0; i < p->pagecount; i++){*/
	/*add_single_pages(p,Hash(pt)%MAXLISTS);*/
	/*p=p->next;*/
	/*}*/
}

/* Single page management */
/* ---------------------- */


void add_single_pages(struct page *base)
/* Effects: Adds pages at base to the single_pages list */
{
	pageid            n;
	struct page       *next;
	unsigned long int index;

	if (base == NULL)
		return;
	n     = base->pagecount;
	index = base->list_id;

	single_pages[index].page_count += n;

	for ( ; ; ) {
		ASSERT_FREE(base);
		base->free                = 0; /* Not free so that coalesce
		                                * won't steal these back */
		base->next                = single_pages[index].pages;
		single_pages[index].pages = base;
		if (--n == 0)
			break;
		next = (struct page*)( (char*)base + RPAGESIZE );
		base = next;
	}
}

struct page* alloc_single_page(struct page *next)
{
	struct page *p = NULL;

	/* pthread_t pt = pthread_self(); */

	list_id = get_next_random_list(MAXLISTS);
	while (try_lock(&single_pages[list_id % MAXLISTS].lock) == 1)
		list_id = get_next_random_list(MAXLISTS);
	/*if( single_pages[Hash(pt)%MAXLISTS].page_count == 0 ){*/
	if (single_pages[list_id % MAXLISTS].page_count == 0)
		p = alloc_new(PAGE_GROUP_SIZE, NULL);
	add_single_pages(p);
	/*p = single_pages[Hash(pt)%MAXLISTS].pages;*/
	p = single_pages[list_id % MAXLISTS].pages;
	/*single_pages[Hash(pt)%MAXLISTS].pages = p->next;*/
	single_pages[list_id % MAXLISTS].pages = p->next;
	p->next                                = next;
	/*single_pages[Hash(pt)%MAXLISTS].page_count--;*/
	single_pages[list_id % MAXLISTS].page_count--;
	/*release_spinlock( &single_pages[Hash(pt)%MAXLISTS].lock );*/
	release_spinlock(&single_pages[list_id % MAXLISTS].lock);
	/*list_id++;*/

	return p;
}

void free_single_page(region_t r, struct page *p)
/* Assumes freepages_lock held */
{
	/* pthread_t pt = pthread_self(); */
#ifndef NMEMDEBUG
	ASSERT_INUSE(p, r);
	set_page_region(MAPNB(p), PAGENB(p), FREEPAGE);
#endif /* ifndef NMEMDEBUG */
	list_id = get_next_random_list(MAXLISTS);
	while (try_lock(&single_pages[list_id % MAXLISTS].lock) == 1)
		list_id = get_next_random_list(MAXLISTS);
	p->next                     = single_pages[list_id].pages;
	single_pages[list_id].pages = p;
	single_pages[list_id].page_count++;
	release_spinlock(&single_pages[list_id].lock);
	/*acquire_spinlock1( &single_pages[p->list_id].lock );*/
	/*p->next = single_pages[p->list_id].pages;*/
	/*single_pages[p->list_id].pages = p;*/
	/*single_pages[p->list_id].page_count++;*/
	/*release_spinlock( &single_pages[p->list_id].lock );*/

	/*p->next = single_pages[Hash(pt)%MAXLISTS].pages;*/
	/*single_pages[Hash(pt)%MAXLISTS].pages = p;*/
	/*single_pages[Hash(pt)%MAXLISTS].page_count++;*/
}
