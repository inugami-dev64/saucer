CC = gcc
OBJ = saucer.c.o \
	  yaml_parser.c.o \
	  str_ext.c.o \
	  build_maker.c.o \
	  make_writer.c.o 

TARGET = saucer
FLAGS = -g -Wall

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

saucer.c.o: saucer.c
	$(CC) -c saucer.c -o saucer.c.o $(FLAGS)

yaml_parser.c.o: yaml_parser.c
	$(CC) -c yaml_parser.c -o yaml_parser.c.o $(FLAGS)
str_ext.c.o: str_ext.c
	$(CC) -c str_ext.c -o str_ext.c.o $(FLAGS)
build_maker.c.o: build_maker.c
	$(CC) -c build_maker.c -o build_maker.c.o $(FLAGS)
make_writer.c.o: make_writer.c
	$(CC) -c make_writer.c -o make_writer.c.o $(FLAGS)

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf saucer
