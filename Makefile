CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -pthread -MMD -MP

ifeq ($(OS),Windows_NT)
TARGET = csopesy_scheduler.exe
CLEAN = cmd /C "del /F /Q $(TARGET) src\*.o src\*.d 2>NUL & if exist process_logs rmdir /S /Q process_logs"
else
TARGET = csopesy_scheduler
CLEAN = rm -f $(TARGET) $(OBJS) $(DEPS) && rm -rf process_logs
endif

SRCS = src/main.cpp \
       src/ConfigParser.cpp \
       src/ConsoleManager.cpp \
       src/Instruction.cpp \
       src/Process.cpp \
       src/ProcessTable.cpp \
       src/Scheduler.cpp \
       src/Utils.cpp
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -Iinclude -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -Iinclude -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	$(CLEAN)

-include $(DEPS)

.PHONY: all run clean
