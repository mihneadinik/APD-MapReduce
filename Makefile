# Mihnea Dinica 333CA

CC=g++
CFLAGS=-Wall -Werror -lpthread

TARGETS=homework

build: $(TARGETS)

homework: main.cpp
	$(CC) main.cpp -o tema1 $(CFLAGS)

clean:
	rm -rf $(TARGETS)
