# compiler
CC = gcc

# compiler flags    
CFLAGS  = -Wall -O3 -maes -ansi -pedantic

# target executables
TARGET = Permutations_Ref Permutations_128 Permutations_256 Permutations_384 Permutations_512 Permutations_768 Permutations_1024

all: $(TARGET)

$(TARGET): *.c
	$(CC) $(CFLAGS) -o $@ $@.c

clean:
	$(RM) $(TARGET)
