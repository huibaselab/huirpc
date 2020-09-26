#!/bin/bash

#apt-get install perl -y
#apt-get install -y libmysqlclient-dev


if [ ! -d deps ]
then 
    mkdir deps;
fi

cd deps;
if [ ! -f libconfig.tar.gz ]
then 
    cp ../../deps/libconfig.tar.gz ./
fi

if [ ! -f hiredis.tar.gz ]
then 
    cp ../../deps/hiredis.tar.gz ./
fi

tar xvfz libconfig.tar.gz
rm -fr libconfig.tar.gz

tar xvfz hiredis.tar.gz
rm -fr hiredis.tar.gz


# install libconfig
cd libconfig
./configure
make -j8
cd ..


# install redisclient
cd hiredis
make -j8

# return to root
cd ../..
make

exit 0