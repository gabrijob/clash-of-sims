CC = g++ -std=c++14
CFLAGS = -Wall -g3
#CFLAGS = -Wall -lpthread -g3 -O3

INSTALL_PATH = /opt/simgrid
INCLUDES = -I../include -I$(INSTALL_PATH)/include
DEFS = -L$(INSTALL_PATH)/lib64
LDADD = -lm -lsimgrid

SRC=$(wildcard *.cpp)
BIN=$(SRC:.c=.bin)
OBJ = simgrid-simulation.o

#all: $(BIN)
all: simgrid-simulation.bin

#%.bin: %.cpp
#	$(CC) $(INCLUDES) $(DEFS) -o $@ $^ $(LDADD)

#%.o: %.cpp
#	$(CC) $(INCLUDES) $(DEFS) $(CFLAGS) $(LDADD) -c -o $@ $<

simgrid-simulation.bin: simgrid-simulation.cpp
	$(CC) $(INCLUDES) $(DEFS) $(LDADD) -o $@ $^ 

clean:
	rm -vf *.bin *.trace *.plist *.log *.csv

.PHONY: clean
