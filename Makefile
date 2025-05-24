CC = gcc
CFLAGS = -Wall -g
INCLUDE = -Iinclude
SRC_DIR = src
BUILD_DIR = build

# Main targets
all: $(BUILD_DIR)/download

$(BUILD_DIR)/download: $(BUILD_DIR)/download.o
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/download.o: $(SRC_DIR)/download.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

run: $(BUILD_DIR)/download
	./$< ftp://ftp.netlab.fe.up.pt/pub/README

.PHONY: all clean run