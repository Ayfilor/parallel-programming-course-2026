CC = clang++
CFLAGS = -std=c++20 -O2

.PHONY: default run test stress clean

default:
	$(CC) cmd/main.cpp $(CFLAGS) -o build/exe

run:
	$(CC) cmd/main.cpp $(CFLAGS) -o build/exe && build/exe

test:
	$(CC) cmd/test.cpp $(CFLAGS) -o build/test_exe && build/test_exe $(TEST)

stress:
	$(CC) cmd/stress.cpp $(CFLAGS) -o build/stress_exe && build/stress_exe

clean:
	rm -r build/*
