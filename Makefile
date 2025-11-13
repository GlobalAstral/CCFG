INCLUDES = -I src
FLAGS = -g3 -std=c17
TARGET = CCFG.exe

SRC := $(wildcard src/*.c) $(wildcard test/*.c)
OBJ := $(SRC:.c=.o)

default: buildNcleanup

build:
	gcc $(INCLUDES) $(FLAGS) -c $(SRC)
	gcc "*.o" -o $(TARGET) $(LBFLAGS)

clean:
	del "*.o"

buildNcleanup: build clean

buildNrun: buildNcleanup
	$(TARGET)
