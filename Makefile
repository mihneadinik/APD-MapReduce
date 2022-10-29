# Mihnea Dinica 333CA

CC=g++
CFLAGS=-Wall -Werror -lpthread

TARGETS=homework

build: $(TARGETS)

homework: main.cpp
	$(CC) $(CFLAGS) main.cpp -o tema1

clean:
	rm -rf $(TARGETS)
