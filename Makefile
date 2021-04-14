.PHONY: all depend
CXX := g++
CXXFLAGS := -std=c++20 -g -Wall -Wextra -Wpedantic -Og 
LDLIBS := -lncurses -lpthread

SRCS := $(filter-out tkw.cpp tkw_sen.cpp Expression.cpp, $(wildcard *.cpp))
BINS := $(SRCS:%.cpp=%.o)

all: tkw tkw_sen

depend: .depend
.depend: $(SRCS) tkw.cpp tkw_sen.cpp
	$(CXX) $(CXXFLAGS) -MM $^ > "$@"
include .depend

%.o: %.cpp 
	${CXX} ${CXXFLAGS} $< -c -o $@

tkw: depend $(BINS) tkw.o
	$(CXX) ${LDLIBS} tkw.o $(BINS) -o $@

tkw_sen: depend $(BINS) tkw_sen.o
	$(CXX) ${LDLIBS} tkw_sen.o $(BINS) -o $@


clean:
	rm -f tkw
	rm -f $(BINS)
