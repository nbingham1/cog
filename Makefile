LLVMFLAGS    := $(shell llvm-config-5.0 --libs core native --cxxflags --ldflags)
CXXFLAGS      =  -g -O2 -Wall -fmessage-length=0 -Isrc $(LLVMFLAGS)
# -g -fprofile-arcs -ftest-coverage
LEXERS       := $(wildcard src/*.l)
PARSERS      := $(wildcard src/*.y)
SOURCES      := $(wildcard src/*.cpp) $(wildcard src/*.c) $(PARSERS:%.y=%.c) $(LEXERS:%.l=%.c)
TESTS        := $(wildcard test/*.cpp)
OBJECTS      := $(SOURCES:%.cpp=%.o)
TEST_OBJECTS := $(TESTS:.c*=.o)
DEPS         := $(OBJECTS:.o=.d)
TEST_DEPS    := $(TEST_OBJECTS:.o=.d)
GTEST        := ../googletest
GTEST_I      := -I$(GTEST)/include -I.
GTEST_L      := -L$(GTEST) -L.
TARGET        = cog
TEST_TARGET   = test_cog

-include $(DEPS)
-include $(TEST_DEPS)

all:
	#$(TARGET)
	echo $(SOURCES)

test: $(TARGET) $(TEST_TARGET)

check: test
	./$(TEST_TARGET)

$(TARGET): $(OBJECTS)
	g++ $(CXXFLAGS) $^ -o $@

src/%.c: src/%.l
	lex -o $@ $<

src/%.c: src/%.y
	yacc -d -o $@ $<

src/%.o: src/%.cpp 
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
	rm -f $(LEXERS:%.l=%.c) $(LEXERS:%.l=%.h)
	rm -f $(PARSERS:%.y=%.c) $(PARSERS:%.y=%.h)
	rm -f src/*.o test/*.o
	rm -f src/*.d test/*.d
	rm -f src/*.gcda test/*.gcda
	rm -f src/*.gcno test/*.gcno
	rm -f $(TARGET) $(TEST_TARGET)
