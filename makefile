default:
	g++ -std=c++11 -lboost_regex -o simi.bin simi.cpp
debug:
	g++ -g -std=c++11 -lboost_regex -o simi.bin simi.cpp
clean:
	rm *.bin
