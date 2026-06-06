suppressed_warning = -Wno-unused-variable -Wno-maybe-uninitialized

all: release tester

release: decoder8086.c
	@mkdir -p build
	cc decoder8086.c -o build/decode8086 -std=c89 -Wall -Wpedantic -O2 $(suppressed_warning)

debug: decoder8086.c
	@mkdir -p build
	cc decoder8086.c -g -o build/decode8086 -Wall -Wpedantic -O0 $(suppressed_warning)

tester: tester.go
	@mkdir -p build
	go build -o build/test tester.go

clean:
	@rm build/decode build/test
