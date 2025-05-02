# Compiler and flags
CC = gcc
CFLAGS = -Wall -g -I$(INCLUDE)

# Paths
MODULES = ./modules
INCLUDE = ./include
ADTS = ./adts

# Find all source files
MODULES_SRC = $(wildcard $(MODULES)/*.c)
ADTS_SRC = $(wildcard $(ADTS)/*.c)
SRC = $(MODULES_SRC) $(ADTS_SRC)

# Object files (replace .c with .o)
OBJS = $(SRC:.c=.o)

# Executable name
EXEC = fss_manager

# Default rule
all: $(EXEC)

# Link
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

# Pattern rule to compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run
run: all
	./$(EXEC)

# Clean
clean:
	rm -f $(OBJS) $(EXEC)
	clear
