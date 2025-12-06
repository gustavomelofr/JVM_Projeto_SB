# Makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = jvm
SRCS = jvm.cpp classfile.cpp disassembler.cpp interpreter.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)