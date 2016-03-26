#This variable stores the name/path of the directory with all source and header files is (relative to the make file)
SRC_DIR=src
#This variable stores the name/path of the directory with all the object files will be (relative to the make file)
OBJ_DIR=build
#This variable stores the names/paths of all the header files relative to the SRC_DIR
# a command like "find . -name \*.h | sed 's/^..//'" is good for this
HEADERS=DAQLib/DigiController.h DAQLib/DigiReader.h HVLib/MPODController.h HVLib/MPODReader.h\
Threads/DigitizerThread.h Threads/EventThread.h Threads/EventThreadPool.h Threads/MPODThread.h\
Threads/UserThread.h InputLib/InputParser.h InputLib/Blocks/DigitizerBlock.h\
InputLib/Blocks/GeneralBlock.h InputLib/Blocks/PowerBlock.h InputLib/Blocks/InputParams.h\
InputLib/Parsers/Parser.h InputLib/Parsers/DigitizerBlockGrammar.h InputLib/Parsers/FileParser.h\
InputLib/Parsers/GeneralBlockGrammar.h InputLib/Parsers/PowerBlockGrammar.h InputLib/Parsers/utility.h 
#This variable stores the names/paths of all the source files relative to the SRC_DIR 
# a command like "find . -name \*.cpp | sed 's/^..//'" is good for this
SOURCES=main.cpp DAQLib/DigiController.cpp DAQLib/DigiReader.cpp HVLib/MPODController.cpp\
HVLib/MPODReader.cpp Threads/DigitizerThread.cpp Threads/EventThread.cpp Threads/EventThreadPool.cpp\
Threads/MPODThread.cpp Threads/UserThread.cpp InputLib/Blocks/DigitizerBlock.cpp\
InputLib/Blocks/GeneralBlock.cpp InputLib/Blocks/PowerBlock.cpp InputLib/Blocks/InputParams.cpp\
InputLib/Parsers/Parser.cpp
#This variable stores the name/path of the executable file this executable will appear in the same folder as the make file
EXECUTABLE=orchid

#This flag contains special include directories in case the user needs to include headers from non standard directories
#The user need not modify this for their own header files as the paths are calculated and included elsewhere in this program
#here I also use this to include a compiler option needed to make the blue libraries compatible with a c++ program
#CLANG_INCLUDE_LOCATIONS=-I/usr/include/c++/5.1.1/x86_64-redhat-linux/
USER_INCLUDE_LOCATIONS=$(CLANG_INCLUDE_LOCATIONS)
#This flag contains special library location directories and library link flags
#CLANG_LIBRARY_FLAGS=-L/usr/lib/gcc/x86_64-redhat-linux/5.1.1 -L/usr/lib64 -L/usr/lib/
USER_LIBRARY_FLAGS=$(CLANG_LIBRARY_FLAGS)
#This variable contains the compiler command set it to g++ for C++ programs and gcc for C programs
COMP=g++
#COMP=clang++
#This variable contains the extentions for the source files set it to .cpp or .C for C++ and .c for C files
EXT=.cpp


#here are the variables that users do not need to modify unless they want to modify compiler flags and the like
#This variable stores the names of all the object files
TEMP_DIR=$(MAKECMDGOALS)
OBJECTS=$(patsubst %$(EXT),$(OBJ_DIR)/$(TEMP_DIR)/%.o,$(SOURCES))

# here we generate and include the list of make files that contain prerequisites
# but we only want to include them if we are not doing a cleaning
MAKE_INCLUDES=$(patsubst %$(EXT),$(SRC_DIR)/%.mk,$(SOURCES))
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
#include the files, suppress the errors since if they are non existant
#they will be rebuilt
-include $(MAKE_INCLUDES)
endif

OUTPUT_FLAG=-o
NO_LINK_FLAG=-c
MAKE_FLAG=-MM -std=c++11
NOWARN_FLAG=-w
VERSION_FLAG=-v
DEBUG_FLAG=-g
OPT_FLAGS=-O2
#OPT_FLAGS=-O3 -fstrict-aliasing
CODE_GEN_FLAG=-m64
#CODE_GEN_FLAG=
STD_FLAG=-std=c++11
BIG_FILE_FLAG=-D_FILE_OFFSET_BITS=64
INCLUDE_LOC_FLAGS=$(USER_INCLUDE_LOCATIONS) -I./$(SRC_DIR)
LIB_FLAGS=$(USER_LIBRARY_FLAGS)

# big initial list of warning flags
WARN_FLAGS=-Wall -Wextra -Wpedantic -Weffc++ -Wdouble-promotion -Wformat \
-Wswitch-default -Wswitch-enum -Wsync-nand -Wsuggest-attribute=pure \
-Wsuggest-attribute=const -Wsuggest-attribute=noreturn \
-Wsuggest-attribute=format -Wsuggest-final-types -Wsuggest-final-methods \
-Wsuggest-override -Wtrampolines -Wfloat-equal -Wshadow \
-Wunsafe-loop-optimizations -Wpointer-arith -Wtype-limits -Wcast-qual \
-Wcast-align -Wwrite-strings -Wconditionally-supported -Wconversion \
-Wzero-as-null-pointer-constant -Wdate-time -Wuseless-cast -Wenum-compare \
-Wlogical-op -Wmissing-declarations -Wmissing-field-initializers -Wnormalized \
-Wopenmp-simd -Wpacked -Wpadded -Wredundant-decls -Winline -Winvalid-pch \
-Wvector-operation-performance -Wvla -Wdisabled-optimization -Wopenmp-simd
# big list of warning flags that get activated with optimization
WARN_OPT_FLAGS=-fstrict-aliasing -Wstrict-aliasing -fstrict-overflow \
-Wstrict-overflow=5 -ftree-vrp -Warray-bounds=2 -fsized-deallocation \
-Wsized-deallocation -flto-odr-type-merging -Wno-odr  -fstack-protector \
-Wstack-protector

#here are the targets that the user should access to build the project they define the 
#this builds the project with debug flags making it easier to attach a debugger and see what is happening
.PHONY: debug
debug: FLAGS=$(DEBUG_FLAG) $(CODE_GEN_FLAG) $(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(BIG_FILE_FLAG) $(LIB_FLAGS)
debug:$(EXECUTABLE)

#this builds the project with no flags but those absolutely necessary
.PHONY: plain
plain: FLAGS=$(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(STD_FLAG) $(LIB_FLAGS)
plain:$(EXECUTABLE)

#this builds the project with debug flags and with optimization flags allowing you to attach a debugger
#to see if the optimization is screwing something up
.PHONY: opt_debug
opt_debug: FLAGS=$(OPT_FLAGS) $(DEBUG_FLAG) $(CODE_GEN_FLAG) $(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(BIG_FILE_FLAG) $(LIB_FLAGS)
opt_debug:$(EXECUTABLE)
	
#this builds the project with no debug flags and with optimization flags making it run faster
.PHONY: release
release: FLAGS=$(OPT_FLAGS) $(CODE_GEN_FLAG) $(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(BIG_FILE_FLAG) $(LIB_FLAGS)
release:$(EXECUTABLE)

#this builds the project with the generic warning flags enabled to see where you might want to make changed
.PHONY: warn
warn: FLAGS=$(WARN_FLAGS) $(OPT_FLAGS)  $(CODE_GEN_FLAG) $(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(BIG_FILE_FLAG) $(LIB_FLAGS)
warn:$(EXECUTABLE)

#this builds the project with the generic warning flags enabled to see where you might want to make changed
.PHONY: warn_opt
warn_opt: FLAGS=$(WARN_FLAGS) $(WARN_OPT_FLAGS) $(CODE_GEN_FLAG) $(STD_FLAG) $(INCLUDE_LOC_FLAGS) $(BIG_FILE_FLAG) $(LIB_FLAGS)
warn_opt:$(EXECUTABLE)

#this builds the automatic documentation
.PHONY: docs
docs: $(patsubst %,$(SRC_DIR)/%,$(SOURCES)) $(patsubst %,$(SRC_DIR)/%,$(HEADERS))
	doxygen ./docs/doxygen/Doxyfile

#cleaning targets to remove various things generated by this make file
#this removes the object directory / files
.PHONY: clean
clean:
	-rm -rf $(OBJ_DIR)

#this removes the obj directory and the prereqs file
.PHONY: veryclean
veryclean: clean
	-rm -f $(MAKE_INCLUDES)

#this removes the obj directory, the prereqs file, and the executable file
.PHONY: cleanall
cleanall: veryclean
	-rm -f $(EXECUTABLE)


#This is the section of the file that is the boiler room it constructs the prerequisites file and the object files
#this constructs the executable
$(EXECUTABLE):$(OBJECTS)
	$(COMP) $(FLAGS) $(LIB_FLAG) $(OBJECTS) $(OUTPUT_FLAG) $(EXECUTABLE)

#all object files are built here, some moderate magic in making the prerequisites work right
.SECONDEXPANSION:
$(OBJECTS): $$($$(*F))
	-@mkdir -p $(@D)
	$(COMP) $(FLAGS) $(NO_LINK_FLAG) $(OUTPUT_FLAG) $@ $(patsubst $(OBJ_DIR)/$(TEMP_DIR)/%.o,$(SRC_DIR)/%$(EXT),$@) #| gSTLFilt.pl
	@echo ""

#The prereqs.mk target, here the dependencies for the various object files 
#	are calculated and placed in a seperate file for each object
%.mk: %$(EXT)
	$(COMP) $(DIR_FLAGS) $(MAKE_FLAG) $(INCLUDE_LOC_FLAGS) $< | sed 's,\([abcdefghijklmnopqrstuvwxyz_-]*\).o:,\1= \\\n,' > $@
	@echo ""

