CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11 -pthread -lcurl

SRC = elevator_scheduler.cpp

TARGET = scheduler_os

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ 

clean:
	rm -f $(TARGET)
