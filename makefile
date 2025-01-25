CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lncurses -lcjson

SRC = main.c task_manager.c ui_controll.c
OBJ = $(SRC:.c=.o)
EXEC = todo

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
