CXX ?= g++
CXXFLAGS = -O2 -std=c++17
LDFLAGS = -lrt

all: publisher subscriber

publisher: publisher.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

subscriber: subscriber.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f publisher subscriber

.PHONY: all clean
