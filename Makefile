CXX=g++-5
CXXFLAGS=-std=c++14 -fomit-frame-pointer	\
		-save-temps							\
		-fnon-call-exceptions				\
		-g -Wall -O3 -march=native			\
		-DNDEBUG

all: delimited benchmark

delimited: delimited_test.cc delimited.hpp  test_utils.hpp
	$(CXX)  $(CXXFLAGS) delimited_test.cc -o delimited

benchmark: benchmark.cc delimited.hpp  test_utils.hpp
	$(CXX)  $(CXXFLAGS) benchmark.cc -o benchmark

.PHONY:
test: run-delimited run-benchmark

.PHONY:
run-delimited: delimited
	./delimited

.PHONY:
run-benchmark: benchmark
	for x in $$(seq 1 10); do ./benchmark ; done

