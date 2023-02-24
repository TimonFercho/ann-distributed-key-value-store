#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#
# set flags for makefile like so:
# make USE_SIMD=TRUE USE_OMP=TRUE PMODE=1

# define the Cpp compiler to use
CXX = g++

# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra
USE_SIMD := TRUE
USE_OMP := TRUE

ifeq ($(USE_SIMD),TRUE)
CXXFLAGS += -mavx
else
$(info SIMD disabled)
endif

ifeq ($(USE_OMP),TRUE)
CXXFLAGS += -fopenmp
else
$(info OpenMP disabled)
endif

# defines the parallel mode
# (0: sequential mode, 1: parallelize over queries, 2: parallelize over queries and lists)
PMODE := 2
ifneq ($(PMODE),0)
ifneq ($(PMODE),1)
ifneq ($(PMODE),2)
$(error PMODE must be 0, 1 or 2)
endif
endif
ifeq ($(USE_OMP),FALSE)
override PMODE := 0
endif
endif
CXXFLAGS += -D PMODE=$(PMODE)

DEBUG := FALSE
ifeq ($(DEBUG),TRUE)
CXXFLAGS += -g
else
CXXFLAGS += -O3
endif

# define library paths in addition to /usr/lib
#   if I wanted to include libraries not in /usr/lib I'd specify
#   their path using -Lpath, something like:
# LFLAGS =

# define output directory
OUTPUT	:= out

# define source directory
SRC		:= src

# define include directory
INCLUDE	:= include

# define lib directory
LIB		:= lib

# define test directory
TEST	:= tests

# define benchmark directory
BENCH := evaluation/storage-lists/bulk-insertion

TMP := tests/tmp

ifeq ($(OS),Windows_NT)
MAIN	:= main.exe
SOURCEDIRS	:= $(SRC)
INCLUDEDIRS	:= $(INCLUDE)
LIBDIRS		:= $(LIB)
FIXPATH = $(subst /,\,$1)
RM			:= del /q /f
MD	:= mkdir
else
MAIN	:= main
TESTMAIN  := testmain
BENCHMAIN := benchmain
SOURCEDIRS	:= $(shell find $(SRC) -type d)
TESTDIRS	:= $(shell find $(TEST) -type d)
BENCHDIRS := $(shell find $(BENCH) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)
FIXPATH = $1
RM = rm -f
MD	:= mkdir -p
endif

# define any directories containing header files other than /usr/include
INCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))

# define the C libs
LIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

# define the C source files
SOURCES		:= $(wildcard $(patsubst %,%/*.cpp, $(SOURCEDIRS)))

# define test source files
TESTS_NO_SRC		  := $(wildcard $(patsubst %,%/*.cpp, $(TESTDIRS)))
TESTS             := $(TESTS_NO_SRC) $(SOURCES:$(SRC)/$(MAIN).cpp=)

# define benchmark source files
BENCHS_NO_SRC			:= $(wildcard $(patsubst %,%/*.cpp, $(BENCHDIRS)))
BENCHS            := $(BENCHS_NO_SRC) $(SOURCES:$(SRC)/$(MAIN).cpp=)

# define the C object files 
OBJECTS		:= $(SOURCES:.cpp=.o)

TESTOBJECTS	:= $(TESTS:.cpp=.o)
TESTOBJECTS_NO_TESTMAIN := $(TESTOBJECTS:$(TEST)/TestMain.o=)

BENCHOBJECTS := $(BENCHS:.cpp=.o)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

OUTPUTMAIN	:= $(call FIXPATH,$(OUTPUT)/$(MAIN))

OUTPUTTEST  := $(call FIXPATH,$(OUTPUT)/$(TESTMAIN))

OUTPUTBENCH := $(call FIXPATH,$(OUTPUT)/$(BENCHMAIN))

all: $(OUTPUT) $(MAIN)

test: $(OUTPUT) $(TESTMAIN)

bench: $(OUTPUT) $(BENCHMAIN)

$(OUTPUT):
	$(MD) $(OUTPUT)

$(MAIN): $(OBJECTS) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS) $(LFLAGS) $(LIBS)

$(TESTMAIN): $(TESTOBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTTEST) $(TESTOBJECTS) $(LFLAGS) $(LIBS)
	
$(BENCHMAIN): $(BENCHOBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTBENCH) $(BENCHOBJECTS) $(LFLAGS) $(LIBS)

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@

.PHONY: clean
clean:
	$(RM) $(OUTPUTMAIN)
	$(RM) $(call FIXPATH,$(OBJECTS))
	$(RM) $(OUTPUTTEST)
	$(RM) $(call FIXPATH,$(TESTOBJECTS_NO_TESTMAIN))
	$(RM) -r $(TMP)/*
	$(RM) $(OUTPUTBENCH)
	$(RM) $(call FIXPATH,$(BENCHOBJECTS))

run: all
	./$(OUTPUTMAIN)

runtest: test
	./$(OUTPUTTEST)

runtests: test
	./$(OUTPUTTEST) --success