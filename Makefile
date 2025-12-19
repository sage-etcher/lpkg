

build: build/src/lpkg/lpkg

setup: build/.gitignore

clean:
	rm -rf build

run: build/src/lpkg/lpkg
	./build/src/lpkg/lpkg

build/.gitignore:
	meson setup build

build/src/lpkg/lpkg: build/.gitignore src/lpkg/**
	ninja -C build

.PHONEY: build setup clean run

# end of file
