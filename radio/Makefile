TARGET = output
LIBS = -lm
CC = g++
CFLAGS = -std=c++11 -pthread

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(addprefix build/,$(patsubst %.cpp,%.o,$(wildcard *.cpp)))
HEADERS = $(wildcard *.h)

build/%.o: %.cpp $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
