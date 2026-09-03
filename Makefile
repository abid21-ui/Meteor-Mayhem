CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDLIBS += -lraylib -lm

SOURCES := $(wildcard src/*.c)
OBJECTS := $(SOURCES:.c=.o)
TARGET := meteor_mayhem

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(OBJECTS) $(TARGET)
