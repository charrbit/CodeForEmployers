/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "buddy.h"
#include "list.h"

/**************************************************************************
 * Public Definitions
 **************************************************************************/
#define MIN_ORDER 12
#define MAX_ORDER 20

#define PAGE_SIZE (1<<MIN_ORDER)
/* page index to address */
#define PAGE_TO_ADDR(page_idx) (void *)((page_idx*PAGE_SIZE) + g_memory)

/* address to page index */
#define ADDR_TO_PAGE(addr) ((unsigned long)((void *)addr - (void *)g_memory) / PAGE_SIZE)

/* find buddy address */
#define BUDDY_ADDR(addr, o) (void *)((((unsigned long)addr - (unsigned long)g_memory) ^ (1<<o)) \
									 + (unsigned long)g_memory)

#if USE_DEBUG == 1
#  define PDEBUG(fmt, ...) \
	fprintf(stderr, "%s(), %s:%d: " fmt,			\
		__func__, __FILE__, __LINE__, ##__VA_ARGS__)
#  define IFDEBUG(x) x
#else
#  define PDEBUG(fmt, ...)
#  define IFDEBUG(x)
#endif

/**************************************************************************
 * Public Types
 **************************************************************************/
typedef struct {
	struct list_head list;
	/* TODO: DECLARE NECESSARY MEMBER VARIABLES */
	void* addr;
	int order;
	int index;
} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/
/* free lists*/
struct list_head free_area[MAX_ORDER+1];

/* memory area */
char g_memory[1<<MAX_ORDER];

/* page structures */
page_t g_pages[(1<<MAX_ORDER)/PAGE_SIZE];

/**************************************************************************
 * Public Function Prototypes
 **************************************************************************/

/**************************************************************************
 * Local Functions
 **************************************************************************/

int getOrder(int size) {
	int order = MIN_ORDER;
	while ( ((1<<order) / size) < 1 ) {
		order++;
	}
	if (order > MAX_ORDER) {
		return -1;
	}
	return order;
}

void* splitBlock(struct list_head* head, int currentOrder, int endOrder, int blockStartIndex) {
	// No further splitting is needed
	if (currentOrder == endOrder) {
		// Remove the block from the free list, i.e. "allocate" it
		list_del(&g_pages[blockStartIndex].list);
		return g_pages[blockStartIndex].addr;
	}
	else { // Need to split block
		int newOrder = currentOrder - 1;
		// Build left block
		page_t* leftBlockPage = &g_pages[blockStartIndex];
		leftBlockPage->order = newOrder;
		
		// Build right block
		int rightBlockIndex = ADDR_TO_PAGE(BUDDY_ADDR(leftBlockPage->addr, newOrder));
		page_t* rightBlockPage = &g_pages[rightBlockIndex];
		rightBlockPage->order = newOrder;

		// Remove larger block from free list of currentOrder
		list_del(&leftBlockPage->list);
		// Add new split blocks to free list of newOrder
		list_add(&rightBlockPage->list, &free_area[newOrder]);
		list_add(&leftBlockPage->list, &free_area[newOrder]);
		// Either split the left block again or allocate to it
		return splitBlock(&free_area[newOrder], newOrder, endOrder, blockStartIndex);
	}
}


/**
 * Initialize the buddy system
 */
void buddy_init()
{
	int i;
	int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;
	for (i = 0; i < n_pages; i++) {
		/* TODO: INITIALIZE PAGE STRUCTURES */
		INIT_LIST_HEAD(&g_pages[i].list);
		g_pages[i].order = -1;
		g_pages[i].addr = PAGE_TO_ADDR(i);
		g_pages[i].index = i;
		
	}

	/* initialize freelist */
	for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
		INIT_LIST_HEAD(&free_area[i]);
	}

	/* add the entire memory as a freeblock */
	list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
	g_pages[0].order = MAX_ORDER;
}

/**
 * Allocate a memory block.
 *
 * On a memory request, the allocator returns the head of a free-list of the
 * matching size (i.e., smallest block that satisfies the request). If the
 * free-list of the matching block size is empty, then a larger block size will
 * be selected. The selected (large) block is then splitted into two smaller
 * blocks. Among the two blocks, left block will be used for allocation or be
 * further splitted while the right block will be added to the appropriate
 * free-list.
 *
 * @param size size in bytes
 * @return memory block address
 */
void *buddy_alloc(int size)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	int orderOfRequest = getOrder(size);
	// Size of request is larger than MAX_ORDER
	if (orderOfRequest < 0) {
		fprintf(stderr, "Allocation size is larger than largest available memory region!\nAllocation Denied . . .\n");
		return NULL;
	}
	int freeOrder;
	for (freeOrder = orderOfRequest; freeOrder <= MAX_ORDER; freeOrder++) {
		if (!list_empty(&free_area[freeOrder])) {
			break;			
		}
		else if (freeOrder == MAX_ORDER) {
			freeOrder = -1;
		}
	}
	// Size of request is larger than any available memory block
	if (freeOrder < 0) {
		fprintf(stderr, "Allocation size is larger than largest available memory region!\nAllocation Denied . . .\n");
		return NULL;
	}
	int blockStartIndex = ADDR_TO_PAGE(list_entry(free_area[freeOrder].next, page_t, list)->addr);
	return splitBlock(&free_area[freeOrder], freeOrder, orderOfRequest, blockStartIndex);
}

/**
 * Free an allocated memory block.
 *
 * Whenever a block is freed, the allocator checks its buddy. If the buddy is
 * free as well, then the two buddies are combined to form a bigger block. This
 * process continues until one of the buddies is not free.
 *
 * @param addr memory block address to be freed
 */
void buddy_free(void *addr)
{
	/* TODO: IMPLEMENT THIS FUNCTION */
	if (addr != NULL) {
		page_t* pageToFree = &g_pages[ADDR_TO_PAGE(addr)];
		if (list_empty(&free_area[pageToFree->order])) {
			// Buddy is not free
			list_add(&pageToFree->list, &free_area[pageToFree->order]);
		}
		// Segfault in this else statement
		else {
			void* buddyAddr = BUDDY_ADDR(pageToFree->addr, pageToFree->order);
			// Find the buddy
			page_t* tempPage = NULL;
			int foundBuddy = 0;
			list_for_each_entry(tempPage, &free_area[pageToFree->order], list) {
				if (tempPage->addr == buddyAddr) {
					// Buddy is free
					foundBuddy = 1;
					page_t* leftBlockPage = pageToFree;
					page_t* rightBlockPage = tempPage;
					if (rightBlockPage->index < leftBlockPage->index) {
						leftBlockPage = rightBlockPage;
						rightBlockPage = pageToFree;
					}
					// Prep right block to be merged with the left block
					rightBlockPage->order = -1;
					// Merge it with left block
					leftBlockPage->order++;
					// Remove buddy from free list
					list_del(&tempPage->list);
					if (leftBlockPage->order <= MAX_ORDER) {
						// See if the new larger block has its buddy free
						buddy_free(leftBlockPage->addr);
					}
					break;
				}
			}
			if (!foundBuddy) {
				// Buddy is not free
				list_add(&pageToFree->list, &free_area[pageToFree->order]);
			}
		}
	}
}

/**
 * Print the buddy system status---order oriented
 *
 * print free pages in each order.
 */
void buddy_dump()
{
	int o;
	for (o = MIN_ORDER; o <= MAX_ORDER; o++) {
		struct list_head *pos;
		int cnt = 0;
		list_for_each(pos, &free_area[o]) {
			cnt++;
		}
		printf("%d:%dK ", cnt, (1<<o)/1024);
	}
	printf("\n");
}
