.PHONY: all depend coverage
CXX := g++
CXXFLAGS := -std=c++20 -g -Wall -Wextra -Wpedantic -Og # --coverage 
LDLIBS := -lncurses -lpthread # --coverage

SRCS := $(filter-out tkw.cpp tkw_sen.cpp, $(wildcard *.cpp))
BINS := $(SRCS:%.cpp=%.o)
GCDA := $(wildcard .gcda)

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
	rm -f tkw tkw.o
	rm -f tkw_sen tkw_sen.o
	rm -f $(BINS)

coverage: $(GCDA)
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory out

