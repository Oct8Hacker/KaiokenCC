# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include

# Directories
SRC_DIR = src
INC_DIR = include

# Common source files used by both client and server
COMMON_SRCS = $(SRC_DIR)/helper.c $(SRC_DIR)/log_helper.c $(SRC_DIR)/error_logger.c

# Executable names
CLIENT_BIN = client
SERVER_BIN = server

# Client and Server specific sources
CLIENT_SRCS = $(SRC_DIR)/client.c $(SRC_DIR)/login.c $(COMMON_SRCS)
SERVER_SRCS = $(SRC_DIR)/server.c $(COMMON_SRCS)

# Default target
all: $(CLIENT_BIN) $(SERVER_BIN)

# Build the client executable
$(CLIENT_BIN): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Build the server executable
$(SERVER_BIN): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean up build artifacts
clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -f *.o $(SRC_DIR)/*.o
	rm -f *.log

.PHONY: all clean