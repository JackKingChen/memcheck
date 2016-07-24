#/*******************************************************************
#*
#*    DESCRIPTION:
#*
#*    AUTHOR:
#*
#*    HISTORY:
#*
#*    DATE:2011-08-02
#*
#*******************************************************************/

################################################################################
################################################################################
CC_TARGET = demo

CC_FLAGS  =#-Wall -Wstrict-prototypes
CC_FLAGS +=-DDBG_MEM_CHECK

LD_FLAGS  =-lpthread -dl

#file need to be CC
CS_FILES = $(wildcard *.c)
CC_FILES	= $(CS_FILES)
CO_FILES	= $(CC_FILES:.c=.o)

PS_FILES = $(wildcard *.cpp)
PC_FILES	= $(PS_FILES)
PO_FILES	= $(PC_FILES:.cpp=.o)


################################################################################
################################################################################
#platform option
#if comples as pc linux platform 
CC_PREFIX  ?= #mipsel-linux-uclibc-

#setup the comples tools
CC		= $(CC_PREFIX)g++
CPP		= $(CC_PREFIX)g++
LD		= $(CC_PREFIX)ld

################################################################################
################################################################################
all:$(CO_FILES) $(PO_FILES) $(CC_TARGET)

clean:
	rm -rf *.o
	rm -rf $(CC_TARGET)

#
# objs
#	
$(CO_FILES):
	$(CC) -c $(CC_FLAGS) $(@:.o=.c) -o $@ 

$(PO_FILES):
	$(CC) -c $(CC_FLAGS) $(@:.o=.cpp) -o $@ 	
#
# target
#		
$(CC_TARGET):
	$(CC) $(LD_FLAGS) -o $@ $(CO_FILES) $(PO_FILES)