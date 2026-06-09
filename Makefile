BUILD_DIR = ./build
TARGET_EXEC = decode8086
SRC = ./decoder8086.c
TEST_EXEC = test
TEST_SRC = ./tester.go
SUPRESSED_WARNING = -Wno-unused-variable -Wno-unused-but-set-variable -Wno-maybe-uninitialized
CFLAGS = -std=c89 -Wall -Wpedantic

all: release tester

release: decoder8086.c
	@mkdir -p build
	$(CC) $(SRC) -o $(BUILD_DIR)/$(TARGET_EXEC) $(CFLAGS) $(SUPRESSED_WARNING) -O2

debug: decoder8086.c
	@mkdir -p build
	$(CC) $(SRC) -o $(BUILD_DIR)/$(TARGET_EXEC) $(CFLAGS) $(SUPRESSED_WARNING) -O0 -g -D_DEBUG

tester: tester.go
	@mkdir -p build
	go build -o build/test $(TEST_SRC)

clean:
	rm -f $(BUILD_DIR)/$(TARGET_EXEC) $(BUILD_DIR)/$(TEST_EXEC)
