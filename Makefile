# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g -I$(INCLUDE)

# Paths
MODULES = ./modules
INCLUDE = ./include

# Automatically find all .c files and convert them to .o
MODULES_SRC = $(wildcard $(MODULES)/*.c)

# Executable program
EXEC = out

# Default rule
all: $(EXEC)

# Link object files to create the executable
$(EXEC): $(MODULES_SRC)
	$(CC) -o $@ $(MODULES_SRC) -lm  # Add -lm here

# Compile .c files from MODULES and ADTs directories into .o files
$(MODULES)/%.o: $(MODULES)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the executable
run: all
	./$(EXEC) -i init.txt

# Clean the build directory
clean:
	rm -f $(MODULES_SRC) $(EXEC)
	clear
