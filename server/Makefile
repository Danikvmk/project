TARGET = server
CC = gcc

SRC = $(wildcard *.c)
OBJ = $(patsubst %.c, %.o, $(SRC))

$(TARGET) : $(OBJ)
	$(CC) -g $(OBJ) -o $(TARGET)

%.o : %.c
	$(CC) -g -c $< -o $@

clean :
	rm $(TARGET) *.o
