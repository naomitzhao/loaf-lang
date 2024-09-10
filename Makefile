loaf: main.cpp
	g++ -std=c++20 -Wall -o loaf main.cpp
clean:
	rm -f loaf *.s *.o