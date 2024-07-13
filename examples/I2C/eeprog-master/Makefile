CFLAGS=-g -I. -Wall -O2 -fno-stack-protector  

all: eeprog 

clean: 
	rm -f eeprog *.o

eeprog: eeprog.o 24cXX.o

eeprog-static: eeprog.o 24cXX.o
	$(CC) -static -o $@ $?
