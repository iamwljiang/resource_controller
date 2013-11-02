CFLAGS = -g
INC= -I.
LIB= -L.

CFLAGS += $(INC) $(LIB)
CXX=g++

SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o, $(SRCS))
TARGET=TestCgroup

.cpp.o:
	$(CXX) $(CFLAGS) -c $<

$(TARGET):$(OBJS)
	$(CXX) $(CLFAGS) $^ -o $@ -lpthread

.PHONY:build
clean:
	rm -rf $(OBJS)
