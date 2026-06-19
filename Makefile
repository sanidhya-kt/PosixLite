# Works on macOS (clang++) and Linux (g++)
CXX      = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = msh
SRCS     = tokenizer.cpp parser.cpp jobs.cpp builtins.cpp executor.cpp main.cpp
OBJS     = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp shell.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: clean
