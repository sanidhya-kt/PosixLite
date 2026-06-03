# Works on macOS (clang++) and Linux (g++)
CXX      = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET   = msh

$(TARGET): shell.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) shell.cpp

clean:
	rm -f $(TARGET)

.PHONY: clean
