CFLAGS = -g
INC= -I.
LIB= -L.

CFLAGS += $(INC) $(LIB)
CXX=g++ 
CC=gcc

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o, $(SRCS))
MEM_OBJ=TestMemory.o
TARGET=TestCgroup TestMemory


.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CXX) $(CFLAGS) -c $<

#$(TARGET):$(OBJS)
#	$(CXX) $(CLFAGS) $^ -o $@ -lpthread

build:$(TARGET)

TestCgroup:$(OBJS)
	$(CXX) $(CLFAGS) $^ -o $@ -lpthread

TestMemory:$(MEM_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ 

.PHONY:build
clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
