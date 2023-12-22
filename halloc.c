#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "halloc.h"

struct heap hp = { (void *)-1, 0 };     //heap metadata

// func decl

/*** INNER LOGIC FUNCTIONS ***/

void *new_cell(size_t size);    //creates a new heap_cell if contiguous space of size size is available in page A

void hdefrag();                 //if two unused cells (in_use bit = 0) are next to each other, merges them into one cell

struct heap_cell * hfindptr(void* ptr);     //finds the heap_cell that starts at ptr address in page A

struct heap_cell * allocate_cell_meta();    /*allocates a new struct heap_cell in page B in the first sizeof(struct heap_cell)
                                              free chunk it finds*/

/*** UTILITIES ***/

void printstruct(struct heap_cell*s);   //prints out the fields of the struct heap_cell s


// func def

/*** PUBLIC API ***/

void *halloc(size_t size)
{
    size_t ps = getpagesize();  //the heap is limited to the size of a memory page
    if(size > ps) return NULL;  //if the user asks for more memory than the memory page contains, return NULL
    if(hp.head == (void*) -1)   //heap initialization: the first halloc() call initializes the heap
    {
        //instantiates page B, assigns the start address of page B as the head of the heap
        hp.head = (struct heap_cell*) mmap(NULL,ps,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,0,0);
        if(hp.head == (void*) -1)   //if mmap() fails, return NULL
        {
            printf("mmap err\n");
            return NULL;
        }
        //instantiates page A, assigns the start address of page A to the start address of the head heap_cell
        //when the heap initalizes, it's composed of one single heap_cell of size of the whole memory page A
        hp.head->start_addr = mmap(NULL,ps,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,0,0);
        if(hp.head->start_addr == (void*) -1)   //if mmap() fails, return NULL
        {
            printf("mmap err\n");
            return NULL;
        }
        hp.avail = ps;          //at the start, the whole page is available
        hp.head->size=ps;       //at the start, the head heap_cell is the whole memory page A
        hp.head->in_use=0;      //at the start, the head heap_cell is not in use
        hp.head->valid=1;       //at the start, the head heap_cell is valid
                                /*the head heap_cell validity bit will never be set to 0; the heap_cell linked
                                  list will always have at least one cell; head will never be merged by hdefrag()
                                  since the mergee node gets merged into its predecessor and not vice-versa*/
        hp.head->next=NULL;     //at the start, the linked list is composed of only one cell; hp.head is both head and tail
    }else if(hp.avail <= size){ //if he user asks for more space than the available one in the heap, return NULL
        return NULL;
    }

    return new_cell(size);  //create a new heap_cell of size size and give its start address back to the user
}

void *new_cell(size_t size)
{
    /* 
        a new heap_cell needs to be created, space from page A of size size needs to be reserved.
        free space in page A might not be contiguous since heap_cells can be freed from the user;
        the space in page A gets reserved according to a best-fit-rightmost search:
            e.g.: if size is equal to 8 bytes and space in page A is distributed this way:
                page A: [|USED| FREE 40bytes |USED| FREE 10bytes |USED| FREE till the end]
            the new heap_cell will reserve 8 bytes from the 10 bytes FREE chunk (best-fit)
    */
    struct heap_cell *s = hp.head, *q = NULL;
    size_t minsize = hp.avail;
    while(s!=NULL)  //goes through all the valid heap_cells
    {
        if(!s->in_use && s->size>=size)     //if the cell is free and can fit the desired size
            if(s->size-size <= minsize)     //updates the best-fit cell
            {
                q = s;
                minsize = s->size-size;
            }
        s=s->next;
    }

    if(q==NULL) return NULL;    //if no suitable heap_cell was found, return NULL
    /* 
        visualization of the new_cell() over multiple calls:

        1.
            [h|........]
            [     h    ]

        2.
            q=h
            [h|n|......]
            [ h |   n  ]

        3.
            n => m
            q=m
            [h|m|n|....]
            [ h | m | n]

        4.
            n => k
            free(m)
            q=m
            [h|m|k|n|..]
            [ h |m|n| k]
    */
    if(q->size - size > 0)  //if the desired chunk (of size size) does not take up the entire selected cell
    {                       //a new free heap_cell of the remaining size must be created to keep track of that free space
        struct heap_cell *n = allocate_cell_meta(); //allocates a new struct heap_cell in page B
        if(n == NULL) return NULL;          //if the allocation fails, return NULL
        n->start_addr=q->start_addr+size;   //set the attributes of the new heap_cell accordingly
        n->size=q->size-size;
        n->valid=1;
        n->in_use=0;
        n->next=q->next;
        q->next=n;
    }
    q->size=size;           //sets the attributes of the cell accordingly to the user's needs
    q->in_use=1;
    hp.avail-=size;         //updates the available space in the heap
    return q->start_addr;   //returns to the user the address in page A where the cell starts
}

int hfree(void* ptr)
{
    struct heap_cell *s = hfindptr(ptr);    //the users provieds a pointer to free, that pointer then used to
                                            //find the cell that starts at that address
    if(s==NULL) return -1;                  //if no cell is found, return -1 (meaning: bad address)

    s->in_use=0;                            //to effecitvely free a heap_cell, set the in_use bit to 0
    memset(s->start_addr,0,s->size);        //and set to 0 the whole chunk of page A the heap_cell pointed at
    hp.avail+=s->size;                      //update the available space space in the heap metadata

    hdefrag();                              //call to defrag contiguous unused heap_cells
    return 0;
}



/*** INNER LOGIC FUNCTIONS ***/

void hdefrag()
{
    struct heap_cell *s = hp.head;
    while(s!=NULL && s->next!=NULL) //scans through all the valid cells in the linked list
    {
        if(!s->in_use && !s->next->in_use)  //if two consecutive cells are unused, they got merged
        {
            s->size+=s->next->size;         //s->next gets merged into s
            s->next->valid=0;               //to "deallocate" s->next from page B, its validity bid gets set to 0
            s->next=s->next->next;
        }
        s=s->next;
    }
}

struct heap_cell * hfindptr(void* ptr)
{
    struct heap_cell *s = hp.head;
    while(s!=NULL)
    {
        if(s->start_addr==ptr)
            return s;
        s=s->next;
    }

    return NULL;
}

struct heap_cell * allocate_cell_meta()
{
    struct heap_cell *s = hp.head + sizeof(struct heap_cell);   //starting from the heap_cell next to the head
                                                                //(since the head is never deallocated)
    while(s<hp.head+getpagesize())      //scans page B in chunks of sizeof(struct heap_cell)
    {
        if(!s->valid)                   //when an invalid chunk (struct) is found, it's page B address gets
            return s;                   //returned to allocate a new struct heap_cell
        s+=sizeof(struct heap_cell);
    }

    return NULL;    //if no invalid (aka free) chunk is found in page B, meaning page B is full, return NULL
}


/*** UTILITIES ***/

void printstruct(struct heap_cell*s)
{
    printf("-%p:\n%p\n%zu\n%d\n%d\n%p\n\n",s, s->start_addr, s->size, s->in_use, s->valid, s->next);
}

void printheap()
{
    struct heap_cell *s = hp.head;
    printf("--heap cells metadata--\n\n");
    while(s!=NULL)
    {
        printstruct(s);
        s=s->next;
    }
    printf("-----------------------\n\n");
}