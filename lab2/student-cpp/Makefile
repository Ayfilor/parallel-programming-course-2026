default:
	clang++ -O2 -std=c++20 -pthread src/*.cpp -o o/lab2

tsan:
	clang++ -fsanitize=thread -g -O1 -std=c++20 -pthread src/*.cpp -o o/lab2_tsan

stress:
	clang++ -O2 -std=c++20 -pthread extra/stress.cpp -o o/stress

ministress:
	clang++ -O2 -std=c++20 -pthread extra/ministress.cpp -o o/ministress
