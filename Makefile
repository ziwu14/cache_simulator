CC=g++
CFLAGS=-std=gnu++98 -pedantic -Wall -Werror  -O3
DBGFLAGS=-std=gnu++98 -pedantic -Wall -Werror -ggdb3 -DDEBUG

SRCS=$(wildcard *.cpp)
$(info    SRCS is $(SRCS))
OBJS=$(patsubst %.cpp,%.o,$(SRCS))
$(info    OBJS is $(OBJS))
DBGOBJS=$(patsubst %.cpp,%.dbg.o,$(SRCS))
$(info    DBGOBJS is $(DBGOBJS))

.PHONY: clean depend all

all: myProgram myProgram-debug
myProgram: $(OBJS)
	g++ -o $@ -O3 $(OBJS)
myProgram-debug: $(DBGOBJS)
	g++ -o $@ -ggdb3 $(DBGOBJS)

%.o:%.cpp
	g++ $(CFLAGS) -c -o $@ $<
%.dbg.o:%.cpp
	g++ $(DBGFLAGS) -c -o $@ $<

clean:
	rm -f *.o  *~ myProgram myProgram-debug obj/*.o  
depend:
	makedepend $(SRCS)
	makedepend -a -o .dgb.o $(SRCS)
# DO NOT DELETE

