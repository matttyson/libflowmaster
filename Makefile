
CC=gcc
CFLAGS= -Wall -DFM_BUILDING_DLL  -fPIC -fvisibility=hidden
LIBS=-lm

CFLAGS+=-g --std=gnu89 -fdiagnostics-color=always
#-Wno-unused-variable 
OPTFLAGS=-Os -flto

#CFLAGS+=-DFM_DEBUG_LOGGING

OBJECTS=\
	flowmaster.o\
	flowmaster_linux.o\
	flash.o

LIBFLOW=libflowmaster.so

.SUFFIXES: .o .c
.PHONY: clean

all: $(LIBFLOW) static monitor setspeed testflash

$(LIBFLOW): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(LIBFLOW) -shared -Wl,-soname,$(LIBFLOW) $(OBJECTS) $(LIBS)

static: $(OBJECTS)
	ar rcs libflowmaster_static.a $(OBJECTS)

monitor: $(LIBFLOW) monitor.o
	$(CC) -Wall -g -o $@ monitor.o -L. -lflowmaster_static -lm

testflash: $(LIBFLOW) testflash.o
	$(CC) -Wall -g -o $@ testflash.o -L. -lflowmaster

setspeed: $(LIBFLOW) speed.o
	$(CC) -Wall -g -o $@ speed.o -L. -lflowmaster

clean:
	rm -f $(OBJECTS) $(LIBFLOW) monitor monitor.o testflash testflash.o setspeed speed.o

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
