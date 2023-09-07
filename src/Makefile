CXX = g++

CFLAGS = -std=c++17 -Wall

DIR = ../build
SRCS = main.cpp correlator.cpp
EXE = $(DIR)/CorrelatorIO
LDFLAGS = -O3 -g
#LDFLAGS = -O0 -g -DDEBUG

.PHONY: all clean prep remake

all: prep $(EXE)

$(EXE): $(SRCS)
	$(CXX) $(CFLAGS) $(LDFLAGS) -o $@ $^

# helper rules
prep:
	@mkdir -p $(DIR)

remake: clean all

clean: 
	rm -f $(EXE) $(OBJS)


