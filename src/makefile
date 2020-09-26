##===================================================================##
## File Name: makefile
## Create Author: Tom Hui
## Create Date: Tue Jun 19 1850 2018
## Description:
##		
##===================================================================##

include ../inc.mk

TARGET :=
TARGET_ME = ${LIB_PATH}/hsmiddle.a
TARGET += ${TARGET_ME}

SRC = $(wildcard *.cpp)
OBJS = ${patsubst %.cpp, %.o, ${SRC}}


CXX_INCLUDE += -I../../huibase/src -I../../huibase/db -I../../huibase/nosql -I../deps/libconfig/lib -I../client -I../deps/redisclient 
LINK_CXX_LIBS += -L../../huibase/lib -lmysqlpp ../deps/libconfig/lib/.libs/libconfig++.a -lnosql

CXX_CFLAGS += -fstack-protector-all
LINK_CXX_CFLAGS += -fstack-protector-all

#TTYPE = EFILE

all: def

def: ${TARGET}

test_me:
	@echo ${OBJS}


include ../route.mk
