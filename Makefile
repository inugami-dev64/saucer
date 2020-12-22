CC = gcc
OBJ = saucer.c.o \
	  yml_parser.c.o \
	  str_ext.c.o 

TARGET = saucer
FLAGS = -g -Wall

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

saucer.c.o: saucer.c
	$(CC) -c saucer.c -o saucer.c.o $(FLAGS)

yml_parser.c.o: yml_parser.c
	$(CC) -c yml_parser.c -o yml_parser.c.o $(FLAGS)

str_ext.c.o: str_ext.c
	$(CC) -c str_ext.c -o str_ext.c.o $(FLAGS)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf saucer
