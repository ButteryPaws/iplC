CXX   = g++-8
EXE = iplC
CXXDEBUG = -g -w
CXXSTD = -std=c++11


.PHONY: all parser lexer clean
all: parser lexer 	
	$(CXX) $(CXXDEBUG) -o $(EXE) driver.cpp parser.o scanner.o 

parser: parser.yy scanner.hh
	bison -d -v $<
	$(CXX) $(CXXDEBUG) -c parser.tab.cc -o parser.o 

lexer: scanner.l scanner.hh parser.tab.hh parser.tab.cc	
	flex++ --outfile=scanner.yy.cc  $<
	$(CXX)  $(CXXDEBUG) -c scanner.yy.cc -o scanner.o

clean:
	rm -f $(EXE) *.o location.hh parser.output parser.tab.cc parser.tab.hh position.hh scanner.yy.cc stack.hh
