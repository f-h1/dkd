# Compiler and compiler flags
CC = gcc
CFLAGS = -I/usr/include/libevdev-1.0 -Wall
LDFLAGS = -levdev

# Define the target executable
TARGET = bin/dkd

# Define the source file
SRC = src/dkd.c
OBJ = bin/dkd.o

# Define the installation directory
PREFIX = /usr
BINDIR = $(PREFIX)/bin

# Default target
all: $(TARGET)

# Link the object file into a binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $(TARGET)

# Compile the source file to an object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Clean target
clean:
	rm -f bin/* $(OBJ)

# Install target
install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/dkd

# Phony targets
.PHONY: all clean install

