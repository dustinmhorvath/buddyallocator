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

  void* address;

  int inuse;
  int index;

} page_t;

/**************************************************************************
 * Global Variables
 **************************************************************************/

/* free lists*/
//21
struct list_head free_area[MAX_ORDER+1];

/* memory area */
//2^20 = 1048576
char g_memory[1<<MAX_ORDER];

/* page structures */
// [256]
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
  
  //n_pages = 256
  int n_pages = (1<<MAX_ORDER) / PAGE_SIZE;

  buddy_base_address = &g_pages[0];

  //for(0 < 256)
  for (i = 0; i < n_pages; i++) {
    g_pages[i].inuse = 0;
    // Correct? Seems to output an address if I printf here
    g_pages[i].address = PAGE_TO_ADDR(i);
    g_pages[i].index = i;
    
    //printf("Inuse:[%i]\n", g_pages[i].inuse);
    //printf("Address:[%i]\n", g_pages[i].address);
    //printf("Index:[%i]\n", g_pages[i].index);
    //printf("\n");
  }
 

  /* initialize freelist */
//for(12 < = 20)
  for (i = MIN_ORDER; i <= MAX_ORDER; i++) {
      /*INIT_LIST_HEAD(ptr) do { (ptr)->next = (ptr); (ptr)->prev = (ptr); } while (0) 
       this sets free_area[i]'s linked list head pointers next and prev to both point at the head*/
    INIT_LIST_HEAD(&free_area[i]);
  }

  /* add the entire memory as a freeblock */
  list_add(&g_pages[0].list, &free_area[MAX_ORDER]);
}

unsigned int next_power2(unsigned int size){
    printf("next_power2(unsigned int size) = %i \n" , size);
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

  //printf("size is currently [%i] \n", size);

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

  // Look across free_list for smallest size free block that's big enough.
  // Starts at 'blockorder' because we don't want any size smaller than that.
  int freeorder = blockorder;
  printf("blockorder is currently [%i] \n", blockorder);
  // TODO maybe problems here if they ask for more memory than is available or
  // more than the size of the memory. Will likely segfault if freeorder >
  // MAX_ORDER
  while(list_empty(&free_area[freeorder]) != 0){ //This returns zero when the list IS NOT empty
    printf("free_area order [%i] has no blocks allotted.\n", freeorder);
    freeorder++;
  }

  // Stopgap helps, but doesn't catch memory in use segfault
  if(freeorder > MAX_ORDER){
    return NULL;
  }

  // Split blocks until small enough
  printf("freeorder is currently [%i] \n", freeorder);
  page_t* page;
  while(freeorder > blockorder){

    // Get frontmost item from free_area queue
    page = list_entry(free_area[freeorder].next, page_t, list);
    
    // We have a pointer, so dequeue from list
    list_del_init(free_area[freeorder].next);

    printf("Checked page order [%i] in use ? %d \n", freeorder, page -> inuse);
    printf("Checked page order [%i] has page index ? %d \n", freeorder, page -> index); 


    // now working in smaller order
    freeorder--;

    printf("Checked page order [%i] has second page index ? %d \n", freeorder, (page -> index) + (1<<(freeorder-1))/PAGE_SIZE);

    // Add the two smaller sizes in order. ORDER MATTERS because when we get
    // the front from the queue, we want to get the earlier entry's index
    list_add_tail(&g_pages[ page -> index ].list, &free_area[freeorder]);
    
    list_add_tail(&g_pages[ page -> index + (1<<(freeorder-1))/PAGE_SIZE ].list, &free_area[freeorder]);

  }

  
  page = list_entry(free_area[freeorder].next, page_t, list);
  list_del_init(free_area[freeorder].next);

  
  return PAGE_TO_ADDR(page -> index);

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
