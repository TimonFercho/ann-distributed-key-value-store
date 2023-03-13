#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#
# set flags for makefile like so:
# make USE_SIMD=1 USE_OMP=1 PMODE=1

# define the Cpp compiler to use
CXX = g++

# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra

# Toggle debug mode
DEBUG := 0
ifneq ($(DEBUG),0)
ifneq ($(DEBUG),1)
$(error DEBUG must be 0 or 1)
endif
endif
ifeq ($(DEBUG),1)
CXXFLAGS += -g
endif

# Set optimization level
O := 3
ifneq ($(O),0)
ifneq ($(O),1)
ifneq ($(O),2)
ifneq ($(O),3)
$(error O must be 0, 1, 2 or 3)
endif
endif
endif
endif
CXXFLAGS += -O$(O)


# Toggle SIMD
USE_SIMD := 1
ifneq ($(USE_SIMD),0)
ifneq ($(USE_SIMD),1)
$(error USE_SIMD must be 0 or 1)
endif
endif
ifeq ($(USE_SIMD),1)
CXXFLAGS += -mavx
else
$(info SIMD disabled)
endif

# Toggle OpenMP
USE_OMP := 1
ifneq ($(USE_OMP),0)
ifneq ($(USE_OMP),1)
$(error USE_OMP must be 0 or 1)
endif
endif
ifeq ($(USE_OMP),1)
CXXFLAGS += -fopenmp
else
$(info OpenMP disabled)
endif

# Set parallel mode
# (0: sequential mode, 1: parallelize over queries, 2: parallelize over queries and lists)
PMODE := 2
ifneq ($(PMODE),0)
ifneq ($(PMODE),1)
ifneq ($(PMODE),2)
$(error PMODE must be 0, 1 or 2)
endif
endif
endif
ifeq ($(USE_OMP),0)
override PMODE := 0
endif
CXXFLAGS += -D PMODE=$(PMODE)

# Toggle dynamic insertion
DYNAMIC_INSERTION := 1
ifneq ($(DYNAMIC_INSERTION),0)
ifneq ($(DYNAMIC_INSERTION),1)
$(error DYNAMIC_INSERTION must be 0 or 1)
endif
endif
CXXFLAGS += -D DYNAMIC_INSERTION=$(DYNAMIC_INSERTION)

# Other parameters
ifdef MIN_TOTAL_SIZE_BYTES
CXXFLAGS += -D MIN_TOTAL_SIZE_BYTES=$(MIN_TOTAL_SIZE_BYTES)
endif
ifdef MIN_N_ENTRIES_PER_LIST
CXXFLAGS += -D MIN_N_ENTRIES_PER_LIST=$(MIN_N_ENTRIES_PER_LIST)
endif
ifdef MAX_BUFFER_SIZE
CXXFLAGS += -D MAX_BUFFER_SIZE=$(MAX_BUFFER_SIZE)
endif

# Test parameters
ifdef TEST_N_SAMPLES
CXXFLAGS += -D TEST_N_SAMPLES=$(TEST_N_SAMPLES)
endif
ifdef TEST_N_LISTS
CXXFLAGS += -D TEST_N_LISTS=$(TEST_N_LISTS)
endif
ifdef TEST_N_PROBES
CXXFLAGS += -D TEST_N_PROBES=$(TEST_N_PROBES)
endif
ifdef TEST_N_RESULTS
CXXFLAGS += -D TEST_N_RESULTS=$(TEST_N_RESULTS)
endif
ifdef TEST_N_THREADS
CXXFLAGS += -D TEST_N_THREADS=$(TEST_N_THREADS)
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