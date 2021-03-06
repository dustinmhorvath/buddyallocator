/**
 * Buddy Allocator
 *
 * For the list library usage, see http://www.mcs.anl.gov/~kazutomo/list/
 */

/**************************************************************************
 * Conditional Compilation Options
 **************************************************************************/
#define USE_DEBUG 0

#define PRINT 0


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
  int split;

  int inUseOrder;

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
    g_pages[i].split = 0;
    g_pages[i].inUseOrder = 0;

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
  //printf("next_power2(unsigned int size) = %i \n" , size);
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

  if(PRINT){printf("ADDING BLOCK size is currently [%i] \n", size);}

  int blocksize = next_power2(size);
  unsigned int blockorder = 0 ;
  while( blocksize>>=1 ) blockorder++;

  //printf("next_power2 set blockorder = [%i] \n", blockorder);
  if(blockorder > MAX_ORDER){
    blockorder = MAX_ORDER;
  }
  else if(blockorder < MIN_ORDER){
    blockorder = MIN_ORDER;
  }

  // Look across free_list for smallest size free block that's big enough.
  // Starts at 'blockorder' because we don't want any size smaller than that.
  int freeorder = blockorder;
  //printf("blockorder is currently [%i] \n", blockorder);
  // TODO maybe problems here if they ask for more memory than is available or
  // more than the size of the memory. Will likely segfault if freeorder >
  // MAX_ORDER
  while(list_empty(&free_area[freeorder]) != 0){ //This returns zero when the list IS NOT empty
    //printf("free_area order [%i] has no blocks allotted.\n", freeorder);
    freeorder++;
  }

  // Stopgap helps, but doesn't catch memory in use segfault
  if(freeorder > MAX_ORDER){
    return NULL;
  }

  // Split blocks until small enough
  //printf("freeorder is currently [%i] \n", freeorder);
  page_t* page;
  page_t* front;

  // If no agreement, remove element and remember it as 'front'. Add the
  // second half onto the list of smaller order
  if(freeorder > blockorder){
    front = page = list_entry(free_area[freeorder].next, page_t, list);
    list_del_init(free_area[freeorder].next);
    freeorder--;
    //printf("Front has index %d \n",front -> index);
    //printf("Added index %d \n",page -> index + (1<<(freeorder))/PAGE_SIZE);
    list_add_tail(&g_pages[ page -> index + (1<<(freeorder))/PAGE_SIZE ].list, &free_area[freeorder]);
    if(PRINT){printf("Added page %d \n", front -> index + (1<<(freeorder))/PAGE_SIZE);}
  }

  // If still too big, keep adding the smaller block indices to order lists
  // until small enough. The very last leftmost element gets returned instead
  // of added.
  while(freeorder > blockorder){

    // Get frontmost item from free_area queue
    page = list_entry(free_area[freeorder].next, page_t, list);

    //printf("Checked page order [%i] in use ? %d \n", freeorder, page -> inuse);
    //printf("Checked page order [%i] has page index ? %d \n", freeorder, page -> index); 


    // now working in smaller order
    freeorder--;

    //   printf("Checked page order [%i] has buddy page index ? %d \n", freeorder, front -> index + (1<<(freeorder))/PAGE_SIZE);
    g_pages[ front -> index + (1<<(freeorder))/PAGE_SIZE ].split = 1;

    list_add_tail(&g_pages[ front -> index + (1<<(freeorder))/PAGE_SIZE ].list, &free_area[freeorder]);
    if(PRINT){printf("Added page %d \n", front -> index + (1<<(freeorder))/PAGE_SIZE);}
  }

  front -> inuse = 1;
  front -> split = 1;
  front -> inUseOrder = blockorder;

  //printf("Returning page address [%i] with pageindex %d \n", &free_area[blockorder].next, front -> index); 

  return front -> address;

}

void coalesce(){
  int temp_order = MIN_ORDER;
  while(list_empty(&free_area[temp_order]) == 0 || temp_order < MAX_ORDER){
    //while(1){
    struct list_head *ptr;
    page_t* page;
    page_t* buddy;
    //printf("While!\n");
    // Go down each list and look for a free buddy
    for (ptr = free_area[temp_order].next; ptr != &free_area[temp_order]; ptr = ptr->next) {
      page = list_entry(ptr, page_t, list);
      int buddyaddress = BUDDY_ADDR(page -> address, page -> inUseOrder);
      int buddyindex = ADDR_TO_PAGE(buddyaddress);
      buddy = &g_pages[buddyindex];
      int lowerindex;
      if(buddy -> index < page -> index){
        lowerindex = buddy -> index;
      }
      else{
        lowerindex = page -> index;
      }
      if(buddy -> inuse == 0 && page -> inUseOrder > 0){// && buddy -> inUseOrder == page -> inUseOrder  ){
        printf("page inuseorder %d\n", page -> inUseOrder);

        // THIS IS THE PART THAT DOESN'T WORK CORRECTLY
        list_del( free_area[temp_order].next );
        list_del( free_area[temp_order].next );

        buddy -> inUseOrder = 0;
        page -> inUseOrder = 0;

        list_add( &g_pages[ lowerindex ].list, &free_area[temp_order+1]  );



      }
      }
      temp_order++;

    }




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

    int pageindex = ADDR_TO_PAGE(addr);
    //int pageindex = ((page_t*)addr) -> index;
    if(PRINT){printf("\nREMOVING addr %d with pageindex %d \n", addr, pageindex);}

    g_pages[pageindex].inuse = 0;

    // Get the inUseOrder of the page at the address provided
    int temp_order = g_pages[pageindex].inUseOrder;
    if(PRINT){printf("remove block addr %d has inUseOrder %d \n", addr, temp_order);}

    page_t* page;


    // THESE AREN'T QUITE WORKING BUT ARE A BETTER IMPLEMENTATION
    //list_add(&g_pages[pageindex].list, &free_area[temp_order]);




    while(list_empty(&free_area[temp_order]) == 0){ // && temp_order < MAX_ORDER){
      struct list_head *ptr;

      // Go down each list and look for a free buddy
      list_for_each(ptr, &free_area[temp_order]) {
        page = list_entry(ptr, page_t, list);
        if(PRINT){printf("Free block in list has address %d and looking for %d. Block has pageindex %d \n", page, BUDDY_ADDR(addr, temp_order), page -> index);}

        if(page -> address == BUDDY_ADDR(addr, temp_order)){
          if(PRINT){printf("MERGING BUDDY with page index %d\n", page -> index);}

          // TODO Problems here. This is where it gets deleted from one list and
          // added to the next one
          list_del(&page -> list);
          // Exit as soon as we have the page we want
          break;
        }
      }
      temp_order++;

    }
    list_add(&g_pages[pageindex].list, &free_area[temp_order]);   

    coalesce();
  }


    /**
     * Print the buddy system status---order oriented
     *
     * print free pages in each order.
     */
    void buddy_dump(){
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
