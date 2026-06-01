suppressed_warning = -Wno-unused-label -Wno-unused-variable

release : main.c
	cc main.c -o decode -std=c89 -Wall -Wpedantic -O2 $(suppressed_warning)
debug : main.c
	cc main.c -g -o decode -std=c89 -Wall -Wpedantic -O0 $(suppressed_warning)
