CC=g++
CCFLAGS=-std=c++11 -g -O3 -Wall -D_PTHREADS -finput-charset=UTF-8 -fexec-charset=UTF-8 -static
CCLINK=-lpthread

MAIN_SOURCE:=Main.cpp 
SOURCE:=$(wildcard *.cpp ./base/*.cpp) 
override SOURCE:=$(filter-out $(MAIN_SOURCE), $(SOURCE)) 
OBJECTS:=$(patsubst %.cpp, %.o, $(SOURCE)) 

MAIN_TARGET:=./bin/HttpServer 

all:$(MAIN_TARGET)  

%.o:%.cpp 
	$(CC) $(CCFLAGS) -c -o $@ $<

$(MAIN_TARGET):$(OBJECTS) Main.o
	$(CC) $(CCFLAGS) -o $@ $^ $(CCLINK) 

Main.o: Main.cpp 
	$(CC) $(CCFLAGS) -c -o $@ $^

.PHONY:
clean:
	rm $(OBJECTS) $(all) Main.o *.log log/* bin/* core* -rf
time:
	date
