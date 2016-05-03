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

  int inuse;
  char owner;


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

static void *buddy_base_address = 0;



/**
 * Initialize the buddy system
 */
void buddy_init(){
  int i;
  int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;

  

  for (i = 0; i < n_pages; i++) {
    g_pages[i].inuse = 0;
    g_pages[i].owner = '\0';
    //INIT_LIST_HEAD(&(g_pages[i].list));

  }

  /* initialize freelist */
  for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
    INIT_LIST_HEAD(&free_area[i]);
  }

  /* add the entire memory as a freeblock */
  list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

unsigned int next_power2(unsigned int size){
  if (size < (1 << MIN_ORDER)) {
    return (1 << MIN_ORDER);
  }

  size--;
  // bitwise or AND equal bitshift
  // Increases size until condition satisfies
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size++;

  return size;
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
void *buddy_alloc(int size){
  //TODO

  if(size == 0) {
    size = 1;
  } 
  else {
    // Get the block size we actually need, rounded up
    size = (int)next_power2(size);
  }

  // these are causing a segfault, but I'm pretty sure something like this
  // will need done

  //list_del(&free_area[MAX_ORDER]);
  //list_add(&g_pages[1<<size].list, &free_area[size]);
  

  return &g_pages[0];

  //return NULL;
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
void buddy_free(void *addr){
  //TODO
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
