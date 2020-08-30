# Makefile for tfstool

.PHONY: all clean

SRCS = configuration/configuration.cpp configuration/ini.cpp commands.cpp \
			 main.cpp services/http.cpp \
			 utils/filesys.cpp utils/logging.cpp

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

CC = gcc
CXX? = g++

# Dependencies
DEP_INCLUDES = $(shell pkg-config libcurl --cflags)
DEP_LFLAGS =
DEP_LIBS = $(shell pkg-config libcurl --libs)

CFLAGS = -Wall -O3 -I.

EXE = tf

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(CFLAGS) -o $(EXE) $(OBJS) $(DEP_LFLAGS) $(DEP_LIBS)

.cpp.o:
	$(CXX) $(CFLAGS) $(DEP_INCLUDES) -MMD -MP -MT $@ -o $@ -c $<

clean:
	rm -f $(OBJS) $(EXE) $(DEPS)

# Include automatically generated dependency files
-include $(DEPS)
