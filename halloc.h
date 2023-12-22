#ifndef HALLOC_H
#define HALLOC_H

/*
    the heap consists of a page of memory (page A), divided in cells of arbitrary size (based on the user's needs)
    heap cells' metadata is stored in a linked list in another page of memory (page B)
*/
struct heap_cell
{
    void* start_addr;   //the address in page A where the heap cell starts at
    size_t size;        //the size of the heap cell
    int in_use;         //whether or not the cell is currently in use. becomes 0 upon freeing the cell
    int valid;          /*whether or not the cell is a valid cell; only valid cells are stored in the linked list.
                          when "deallocating" a node of the list to merge its cell with the previous node's one, 
                          its validity  bit gets set to zero and it gets removed from the list.
                          validity bit set at zero means that the node (struct heap_cell) allocator knows 
                          that chunk of memory in page B can be used to allocate a new struct heap_cell when needed*/
    struct heap_cell *next; //pointer to the next node in the linked list. it's an address in page B
};

struct heap
{
    struct heap_cell* head; //stores the page B address of the struct heap_cell at the head of the linked list
    size_t avail;           /*stores the total available space in the heap (note: it's not contiguous, so 8 bytes left
                              doesn't necessarily mean an 8 byte space can be reserved from the heap)*/
    //heap* next;
};


/**API**/

void *halloc(size_t size);  //the function to call instead of malloc() specifying the size to reserve

int hfree(void* ptr);       //the function to call instead of free() specifying the address to free
                            //size metadata is stored in the heap_cell

void printheap();           //prints out the heap_cell linked list

#endif
