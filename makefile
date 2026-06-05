suppressed_warning = -Wno-unused-label -Wno-unused-variable

release: decoder.c
	@mkdir -p build
	cc decoder.c -o build/decode -std=c89 -Wall -Wpedantic -O2 $(suppressed_warning)

debug: decoder.c
	@mkdir -p build
	cc decoder.c -g -o build/decode -Wall -Wpedantic -O0 $(suppressed_warning)

tester: tester.go
	@mkdir -p build
	go build -o build/test tester.go

clean:
	@rm build/decode build/test
