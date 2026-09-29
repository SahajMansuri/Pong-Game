CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra

pong: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o pong $$(sdl2-config --cflags --libs)

clean:
	rm -f pong

.PHONY: clean
