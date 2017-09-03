ROOT  = ../../..
PLATF = unix

VPATH = ../ $(ROOT)/usb/ $(ROOT)/usb/class

IPATH = $(VPATH)
IPATH+= $(ROOT)/lib/$(PLATF)/inc
IPATH+= $(ROOT)/inc
VPATH+= $(ROOT)/lib/$(PLATF)/src

PRJ= dscbld
CC = gcc
CXX= g++
CFLAGS = -Wall -Os
CFLAGS+= -DDSCBLD=1
CFLAGS+= $(addprefix -I,$(IPATH))
#CFLAGS+= -ggdb
MFLAGS = -o $(PRJ)
LFLAGS = -lpthread

all: $(PRJ)

OBJS = usbuser.o

OBJS+= usbclass.o
OBJS+= usbdevice.o
OBJS+= usbhw.o
OBJS+= usbinterface.o
OBJS+= usbendpoint.o

OBJS+= msclass.o
#OBJS+= cdclass.o

$(PRJ): $(OBJS)
	$(CXX) $(MFLAGS) $(OBJS) $(LFLAGS)
clean:
	rm -f *.o *~ $(PRJ)

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

