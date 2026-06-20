CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread -MMD -MP
TARGET = csopesy_scheduler

SRCS = main.cpp Process.cpp Scheduler.cpp ConsoleManager.cpp Utils.cpp
SRCS = src/main.cpp src/Process.cpp src/Scheduler.cpp src/ConsoleManager.cpp src/Utils.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp:.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -Iinclude -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)
	rm -rf process_logs

-include $(DEPS)

.PHONY: all run clean
