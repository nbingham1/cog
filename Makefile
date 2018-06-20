CXXFLAGS	   =  -g -O2 -Wall -fmessage-length=0 -I.
# -g -fprofile-arcs -ftest-coverage
SOURCES		   := $(wildcard src/*.cpp)
TESTS        := $(wildcard test/*.cpp)
OBJECTS		   := $(SOURCES:%.cpp=%.o)
TEST_OBJECTS := $(TESTS:.cpp=.o)
DEPS         := $(OBJECTS:.o=.d)
TEST_DEPS    := $(TEST_OBJECTS:.o=.d)
GTEST        := ../googletest
GTEST_I      := -I$(GTEST)/include -I.
GTEST_L      := -L$(GTEST) -L.
TARGET		   = llcog
TEST_TARGET  = test_llcog

-include $(DEPS)
-include $(TEST_DEPS)

all: $(TARGET)

test: $(TARGET) $(TEST_TARGET)

parser:
	yacc -d *.y
	lex *.l
	g++ lex.yy.c y.tab.c -lstdcore -Istdcore/ -Lstdcore/ -o bas

check: test
	./$(TEST_TARGET)

$(TARGET): $(OBJECTS)
	g++ `llvm-config-5.0 --libs core native --cxxflags --ldflags` $^ -o $<

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
	rm -f src/*.o test/*.o
	rm -f src/*.d test/*.d
	rm -f src/*.gcda test/*.gcda
	rm -f src/*.gcno test/*.gcno
	rm -f $(TARGET) $(TEST_TARGET)
