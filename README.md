# halloc
Custom `heap` written in C; re-implementing the `malloc()` functionalities _for fun and learning_ :)

# How it works
In this paragraph, I'm going to describe how `halloc` works, what's the general idea behind it, where does it get memory from and how it manages it.

## The General Idea
`halloc()` needs to get memory from _somewhere_, and give partitions of it to the user when they ask for a place to allocate some data.  

We'll call this "zone of memory" the `heap`, and store its metadata (its size and the address where it starts) in some structure.  

The user will receive partitions of the `heap`, we will call them `heap_cell`, and store them in a (linked) list, ordered accordingly to where in the heap the point to. So, for example, we could have a list like this:

```
[heap_cell 0]     [heap_cell 1]     [heap_cell 2]
[addr: 0x000] ->  [addr: 0x010] ->  [addr: 0x018] -> ...
[size:   10B]     [size:    8B]     [size:    2B]
```
So that where `heap_cell x` ends in the `heap`, `heap_cell x+1` starts.

When the user asks for a zone of memory of a certain size using the `halloc()` function, upon making sure there's enough space in the heap, a new `heap_cell` of the desired size will be created, and its start address will be given back to the user as the return. 

The user might also want to free some of the memory they allocated. The `hfree()` function serves this exact purpose; given a pointer, an address in memory, the `heap_cell` that starts at that provided address will be freed. 

To make the `heap` more space-efficient, every time the user frees a cell, a heap-defragmentation procedure is called; if two consecutive `heap_cell` are freed, they are merged into a single one with size equal to the sum on the single `heap_cell`'s sizes. Using the same sample list as before, if the user frees `heap_cell 0` and `heap_cell 1`, the `hdefrag()` call will result in the following list:

```
[heap_cell 0]     [heap_cell 1]
[addr: 0x000] ->  [addr: 0x018] -> ...
[size:   18B]     [size:    2B]
```

The head cannot possibly be deallocated, otherwise we'll lose the memory address where the `heap` starts. Hence, an allocated `heap_cell` gets deallocated only if it's not in use anymore (meaning it was freed by the user), and it's predecessor `heap_cell` in the list is also not in use. The two of them will be merged during defragmentation, creating a contiguous zone of free memory. 

## Memory

To "get" the memory for the `heap`, `halloc` uses the `mmap()` function, requesting for a whole page of memory (this means the `heap` is limited to a predetermined size, that by default is the size of a page of memory). 
  
`halloc` uses _two_ pages of memory, we'll call them __[Page A]__ and __[Page B]__.   
  
Why two?   
`halloc()` needs access to two distinct zones of memory: 
- One to utilize as the actual `heap`, to get addresses from to give back to the user. We'll call this one __[Page A]__. 
- A second one to dinamically store `heap_cell` metadata in a linked list. We'll call this one __[Page B]__.

# `halloc` in Detail
In this paragraph, I'm going to cover the data structes and all the specific functions in `halloc` (except for the printing functions and the general linked list research funcion), how they're implemented and their inner workings.   

The source code is already extensively commented, this documentation aims to be a general but in-detail description of what the functions do and how they do it, for a more general undersating of how `halloc` works.

## Structures

### struct heap  
The data structure that stores the metadata of the entire heap, is `struct heap`. 

### struct heap_cell  

## Functions 

### `void *halloc(size_t size)`

### `void *new_cell(size_t size)`

### `struct heap_cell *allocate_cell_meta()`

### `int hfree(void *ptr)`

### `void hdefrag()`
