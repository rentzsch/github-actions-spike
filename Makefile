BINNAME = native-instruction-support
CCFLAGS = -std=c17 -Wall -Wextra -pedantic

all: fmt build test clean

fmt:
	clang-format -i -style='{BasedOnStyle: LLVM, IndentPPDirectives: BeforeHash}' *.c

ifeq ($(shell uname -s),Darwin)
#
# Apple Darwin (macOS): Utilize Rosetta 2 to test x64 in addition to arm64.
#
build:
	cc $(CCFLAGS) -o $(BINNAME)-arm64 *.c
	cc $(CCFLAGS) -arch x86_64 -mmacosx-version-min=10.5 -o $(BINNAME)-x64 *.c

test:
	./$(BINNAME)-arm64
	./$(BINNAME)-x64

clean:
	rm -f $(BINNAME)-arm64 $(BINNAME)-x64
else
#
# Other OS: Just build and test the native architecture.
#
build:
	cc $(CCFLAGS) -o $(BINNAME) *.c

test:
	./$(BINNAME)

clean:
	rm -f $(BINNAME)
endif
