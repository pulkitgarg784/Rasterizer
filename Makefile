.PHONY: all build clean run

all: build

build:
	@cmake -B build
	@cmake --build build

clean:
	@rm -rf build
