
ROOTPATH=/hui
PRONAME=huirpc
LIB_PATH=${ROOTPATH}/${PRONAME}/lib
INCLUDE_PATH=${ROOTPATH}/${PRONAME}/include
ITEMS = client
DIRS = src ${ITEMS}

all:
	@for dir in ${DIRS}; do make -C $$dir -j8; echo ; done


clean:
	@for dir in ${DIRS}; do make -C $$dir clean; echo ; done


install:
	mkdir -p ${LIB_PATH}
	cd lib; cp *.a ${LIB_PATH}/;
	mkdir -p ${INCLUDE_PATH}/;
	@for dir in ${ITEMS}; do mkdir -p ${INCLUDE_PATH}/$$dir; done
	cp src/*.h ${INCLUDE_PATH}/;
	@for dir in ${ITEMS}; do cp $$dir/*.h ${INCLUDE_PATH}/$$dir/; done


type_test:
	@echo ${OS_NAME};


