SERVER_DIR  := ../server
INCLUDE_DIR := ../include 
CC := gcc
CFLAGS_DEBUG = -g -O0 -Wall -Wextra -DDEBUG -DHTTP_USE_MEMMEM -I$(INCLUDE_DIR)
CFLAGS_RELEASE = -O2 -Wall -Wextra -Werror -DNDEBUG -DHTTP_USE_MEMMEM -I$(INCLUDE_DIR)

TARGET := server

SRC := test.c $(wildcard $(SERVER_DIR)/*.c)

BUILD ?= release

ifeq ($(BUILD),debug)
	CFLAGS = $(CFLAGS_DEBUG)
else
	CFLAGS = $(CFLAGS_RELEASE)
endif

all: $(TARGET)

debug: 
	$(MAKE) BUILD=debug

release: 
	$(MAKE) BUILD=release

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $(TARGET)
	@echo "Compiling $(TARGET) complete" 

.PHONY: clean
clean:  
	rm -rf $(TARGET)
	@echo "Cleaning complete"
