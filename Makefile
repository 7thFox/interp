

run: build
	bin/interp

build: bin
	gcc -o bin/interp main.c

bin:
	mkdir bin