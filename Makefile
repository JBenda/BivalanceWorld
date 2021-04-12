.PHONY: all depend
CXX := g++
CXXFLAGS := -std=c++20 -g -Wall -Wextra -Wpedantic -O2 
LDLIBS := -lncurses -lpthread

SRCS := $(wildcard *.cpp)
BINS := $(SRCS:%.cpp=%.o)

all: tkw

depend: .depend
.depend: $(SRCS)
	$(CXX) $(CXXFLAGS) -MM $^ > "$@"
include .depend

%.o: %.cpp 
	${CXX} ${CXXFLAGS} $< -c -o $@

tkw: depend $(BINS)
	$(CXX)  ${LDLIBS} $(BINS) -o $@


clean:
	rm -f tkw
	rm -f $(BINS)
