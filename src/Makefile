CC := gcc
CFLAGS_DEBUG = -g -O0 -Wall -Wextra -DDEBUG -DHTTP_USE_MEMMEM
CFLAGS_RELEASE = -O2 -Wall -Wextra -Werror -DNDEBUG -DHTTP_USE_MEMMEM

TARGET := server

SRC := main.c server.c utils.c epoll_utils.c connection.c http_parser.c http_response.c timer.c shutdown.c

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
