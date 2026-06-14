BUILD_DIR = ./build
TARGET_EXEC = 8086sim
SRC = ./8086sim.c
TEST_EXEC = test
TEST_SRC = ./src/tester.go
SUPRESSED_WARNING =
CFLAGS = -std=c89 -Wall -Wpedantic

all: release tester

release:
	@mkdir -p build
	$(CC) $(SRC) -o $(BUILD_DIR)/$(TARGET_EXEC) $(CFLAGS) $(SUPRESSED_WARNING) -O2

debug:
	@mkdir -p build
	$(CC) $(SRC) -o $(BUILD_DIR)/$(TARGET_EXEC) $(CFLAGS) $(SUPRESSED_WARNING) -O0 -g -D_DEBUG

debug_loud:
	@mkdir -p build
	$(CC) $(SRC) -o $(BUILD_DIR)/$(TARGET_EXEC) $(CFLAGS) -O0 -g -D_DEBUG

tester:
	@mkdir -p build
	go build -o build/test $(TEST_SRC)

clean:
	rm -f $(BUILD_DIR)/$(TARGET_EXEC) $(BUILD_DIR)/$(TEST_EXEC)
