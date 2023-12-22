main.out: main.o halloc.o
	gcc main.o halloc.o -o main.out

main.o: main.c
	gcc -c main.c -o main.o

halloc.o: halloc.c
	gcc -c halloc.c -o halloc.o

clean: 
	rm *.o

.PHONY: clean