VPATH = . ../../src
PR = test
CC = gcc
CXX= g++
AS = as
CFLAGS = -Wall -Os -I../../inc
MFLAGS = -o $(PR)
LFLAGS = 

all : $(PR)

OBJECTS = main.o
OBJECTS+= contextserver.o
OBJECTS+= keys.o tea.o
OBJECTS+= fwcheck.o

$(PR) : $(OBJECTS)
	$(CXX) $(MFLAGS) $(OBJECTS) $(LFLAGS)
clean:
	rm -f *.o *~ $(PR)
	rm -rf ./html
%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<
%.o : %.cpp
	$(CXX) -c $(CFLAGS) -o $@ $<
%.o : %.s
	$(AS) -o $@ $<

.PHONY : all clean
