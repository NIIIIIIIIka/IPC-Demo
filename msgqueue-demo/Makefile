CXX ?= g++
CXXFLAGS = -O2 -std=c++17

all: publisher subscriber

publisher: publisher.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

subscriber: subscriber.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f publisher subscriber

.PHONY: all clean
