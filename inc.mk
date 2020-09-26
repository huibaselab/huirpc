##===================================================================##
## File Name: inc.mk
## Create Author: Tom Hui tomhui1009@yahoo.com 8613760232170
## Create Date: Tue Sep 12 1359 2015
## Description:
##		Makefile header. This include some declaration for using in 
##	makefile. I create this file for reduce duplication.
## DONOT MODIFY THIS FILE UNTIL YOU UNDERSTAND THE DETAIL.
##===================================================================##

CC = g++
AR_CMD = ar cr

CFLAGS = -g

HUI_ROOT = /hui
HUI_DEPS_PATH = ${HUI_ROOT}/deps
HUIBASE_ROOT = ${HUI_ROOT}/huibase
HUIBASE_INCLUDE_PATH = ${HUIBASE_ROOT}/include
HUIBASE_LIB_PATH = ${HUIBASE_ROOT}/lib

BIN_PATH = ../bin
LIB_PATH = ../lib

#CXX_CFLAGS = -fPIC -O2 -g -Wall -std=c++11
CXX_CFLAGS = -fPIC -O2 -g -Wall -Wextra -Werror -Wno-unknown-pragmas -fstack-protector-strong -fstack-protector -fPIC -std=c++11
CXX_DEFINES :=
CXX_INCLUDE = -I../src -I${HUIBASE_INCLUDE_PATH} 
CXX_INCLUDE += -I${HUIBASE_INCLUDE_PATH}/nosql 
CXX_INCLUDE += -I${HUIBASE_INCLUDE_PATH}/db 
CXX_INCLUDE += -I${HUIBASE_INCLUDE_PATH}/crypto 
CXX_INCLUDE += -I${HUI_DEPS_PATH}/hiredis
CXX_INCLUDE += -I${HUI_DEPS_PATH}/libconfig/lib

CC_CFLAGS :=
CC_DEFINES :=
CC_INCLUDE :=

LINK_CXX_CFLAGS :=
LINK_CXX_LIBS := -L../lib -lhuibase -lhuidb

LINK_CC_CFLAGS :=
LINK_CC_LIBS :=


SO_CXX_CFLAGS = -shared -fPIC
AR_CC_CFLAGS :=

.PHONY: all clean install test type_test

OS_NAME = $(shell uname)


CXX_INCLUDE += -I../../huibase/src -I../../huibase/db -I../deps/mysql++/lib -I/usr/include/mysql
LINK_CXX_LIBS += -L../../huibase/lib -pthread


TTYPE = AFILE

TARGET :=


