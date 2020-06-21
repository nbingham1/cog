LLVMFLAGS    := $(shell llvm-config-5.0 --cxxflags --ldflags)
LLVMLIBS     := $(shell llvm-config-5.0 --libs core mcjit native) 
CXXFLAGS      =  -g -O2 -Wall -fmessage-length=0 -Isrc -I../pgen/ -L../pgen/ $(LLVMFLAGS)
# -g -fprofile-arcs -ftest-coverage
LEX          := $(wildcard src/*.l)
YACC         := $(wildcard src/*.y)
PSOURCES     := $(LEX:=.cpp) $(YACC:=.cpp)
POBJECTS     := $(LEX:=.o) $(YACC:=.o)
CSOURCES     := $(wildcard src/*.cpp)
COBJECTS     := $(CSOURCES:%.cpp=%.o)
SOURCES      := $(PSOURCES) $(CSOURCES)
OBJECTS      := $(POBJECTS) $(COBJECTS)
DEPS         := $(OBJECTS:%.o=%.d)
TARGET        = cog

TSOURCES     := $(wildcard test/*.cpp)
TOBJECTS     := $(TSOURCES:%.cpp=%.o)
TDEPS        := $(TOBJECTS:%.o=%.d)
GTEST        := ../googletest
GTEST_I      := -I$(GTEST)/include -I. 
GTEST_L      := -L$(GTEST) -L.
TTARGET       = test_cog

all: $(PSOURCES) $(TARGET)

test: $(TARGET) $(TTARGET)

check: test
	./$(TTARGET)

-include $(DEPS)
-include $(TDEPS)

$(TARGET): $(OBJECTS)
	g++ $(CXXFLAGS) $^ $(LLVMLIBS) -lparse -o $@

src/%.l.c: src/%.l
	lex -o $@ $<

src/%.y.c: src/%.y
	bison -y -d -o $@ $<

src/%.cpp: src/%.c
	mv $< $@

src/%.o: src/%.cpp 
	$(CXX) $(CXXFLAGS) -MM -MF $(patsubst %.o,%.d,$@) -MT $@ -c $<
	$(CXX) $(CXXFLAGS) -c -o $@ $<

src/%.o: src/%.c 
	$(CXX) $(CXXFLAGS) -MM -MF $(patsubst %.o,%.d,$@) -MT $@ -c $<
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(TEST_TARGET): $(TEST_OBJECTS) test/gtest_main.o
	$(CXX) $(CXXFLAGS) $(GTEST_L) $^ -o $(TEST_TARGET)

test/%.o: test/%.cpp
	$(CXX) $(CXXFLAGS) $(GTEST_I) -MM -MF $(patsubst %.o,%.d,$@) -MT $@ -c $<
	$(CXX) $(CXXFLAGS) $(GTEST_I) $< -c -o $@

test/gtest_main.o: $(GTEST)/src/gtest_main.cc
	$(CXX) $(CXXFLAGS) $(GTEST_I) $< -c -o $@

clean:
	rm -f src/*.y.* src/*.l.*
	rm -f src/*.o test/*.o
	rm -f src/*.d test/*.d
	rm -f src/*.gcda test/*.gcda
	rm -f src/*.gcno test/*.gcno
	rm -f $(TARGET) $(TEST_TARGET)
