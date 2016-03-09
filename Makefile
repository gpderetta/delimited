CXX=g++-5
CXXFLAGS=-std=c++14 -fomit-frame-pointer	\
		-save-temps							\
		-fnon-call-exceptions				\
		-g -Wall -O3 -march=native			\
		-DNDEBUG

run-test: delimited_test
	./delimited

delimited_test: delimited_test.cc delimited.hpp  test_utils.hpp
	$(CXX)  $(CXXFLAGS) delimited_test.cc -o delimited
