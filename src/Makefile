CC := gcc
CFLAGS := -Wall -Wextra -O2

TARGET := server

SRC := main.c server.c utils.c epoll_utils.c connection.c http_parser.c http_response.c timer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $(TARGET)
	@echo "Compiling $(TARGET) complete" 

.PHONY: clean
clean:  
	rm -rf $(TARGET)
	@echo "Cleaning complete"
