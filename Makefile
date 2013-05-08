
CC=gcc
CFLAGS= -Os -flto -Wall -Wno-unused-variable -DFM_BUILDING_DLL  -fPIC -fvisibility=hidden
LIBS=-lm

#CFLAGS+=-DFM_DEBUG_LOGGING

OBJECTS=\
	flowmaster.o\
	flowmaster_linux.o\
	flash.o

LIBFLOW=libflowmaster.so

.SUFFIXES: .o .c
.PHONY: clean

all: $(LIBFLOW) monitor setspeed

$(LIBFLOW): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(LIBFLOW) -shared -Wl,-soname,$(LIBFLOW) $(OBJECTS) $(LIBS)

monitor: $(LIBFLOW) monitor.o
	$(CC) -Wall -g -o $@ monitor.o -L. -lflowmaster

testflash: $(LIBFLOW) testflash.o
	$(CC) -Wall -g -o $@ testflash.o -L. -lflowmaster

setspeed: $(LIBFLOW) speed.o
	$(CC) -Wall -g -o $@ speed.o -L. -lflowmaster

clean:
	rm -f $(OBJECTS) $(LIBFLOW) monitor monitor.o testflash testflash.o setspeed speed.o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
