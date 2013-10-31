CFLAGS = -Wall -g
INC= -I.
LIB = -L.

CFLAGS += $(INC) $(LIB)
CXX=g++

SRCS=$(wildcard %.cpp)
OBJS=$(patsubst %.cpp,%.o, $(SRCS))
TARGET=TestCgroup
.cpp.o:
    $(CXX) $(CFLAGS) -c $<

$(TARGET):$(OBJS)
    $(CXX) $(CLFAGS) -o $@ $< -lcgconfig -lpthread

.PHONY:build