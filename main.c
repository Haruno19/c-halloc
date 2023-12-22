#include <stdio.h>
#include "halloc.h"

int main()
{
    int *p = (int*) halloc(sizeof(int));        //allocates an int pointer
    int *p1 = (int*) halloc(sizeof(int));       //allocates another int pointer
    hfree(p);                                   //frees the first int pointer
    char *c = (char*) halloc(sizeof(char));     //allocates a char pointer 
    printheap();                                //prints the current state of the heap

    return 0;
}