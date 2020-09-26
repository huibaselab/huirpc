##===================================================================##
## File Name: makefile
## Create Author: Tom Hui
## Create Date: Tue Jun 19 1850 2018
## Description:
##		
##===================================================================##

include ../inc.mk

TARGET :=
TARGET_ME = ${LIB_PATH}/libmidclient.a
TARGET += ${TARGET_ME}

SRC = $(wildcard *.cpp)
OBJS = ${patsubst %.cpp, %.o, ${SRC}}


CXX_INCLUDE += -I../../huibase/src -I../deps/libconfig/lib
LINK_CXX_LIBS += -L../../huibase/lib ../deps/libconfig/lib/.libs/libconfig++.a

CXX_CFLAGS += -fstack-protector-all
LINK_CXX_CFLAGS += -fstack-protector-all

#TTYPE = EFILE

all: def

def: ${TARGET}

test_me:
	@echo ${OBJS}


include ../route.mk
