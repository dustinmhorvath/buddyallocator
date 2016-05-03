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

#include "math.h"

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

  char* address;

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
    // Correct?
    g_pages[i].address = PAGE_TO_ADDR(i);

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

  printf("size is currently [%i] \n", size);

  // TODO TODO TODO the size argument is in BYTES, NOT ORDER. NEEDS FIX.
  // not sure if this works, untested

  

  int blocksize = next_power2(size);
  unsigned int blockorder = 0 ;
  while( blocksize>>=1 ) blockorder++;

  printf("next_power2 set blockorder = [%i] \n", blockorder);
  if(blockorder > MAX_ORDER){
    blockorder = MAX_ORDER;
  }
  else if(blockorder < MIN_ORDER){
    blockorder = MIN_ORDER;
  }

  // Look across free_list for smallest size free block that's big enough
  int freeorder = blockorder;
  printf("blockorder is currently [%i] \n", blockorder);
  while(list_empty(&free_area[freeorder]) != 0 && freeorder <= MAX_ORDER){
    printf("free_area order [%i] is not available\n", freeorder);
    freeorder++;
  }
  printf("freeorder is currently [%i] \n", freeorder);
  while(freeorder > blockorder){
    page_t* page;
    page = list_entry(&free_area[freeorder], page_t, list);
    //list_del_init(&free_area[freeorder]);
    //printf("Checked page order [%i] in use ? %i", freeorder, page -> inuse);
    freeorder--;
  }


  //list_del_init(&free_area[MAX_ORDER]);
  // this line executes without segfault
  //
  //list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
  
  // this is segfaulting for some reason
  //

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
