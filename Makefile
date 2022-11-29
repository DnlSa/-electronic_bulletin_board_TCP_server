CC = gcc
CFLAGS = -Wall

all:
	@echo "use either 'IT' or 'ENG' depending on the language you want to use"

IT:
	@gcc server.c -o server -pthread -DIT
	@gcc client.c -o client -DIT


ENG:
	@gcc server.c -o server -pthread
	@gcc client.c -o client
