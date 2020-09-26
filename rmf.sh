#!/bin/bash

for ff in $(ls)
do
    if [ -d $ff ]; then
        cd $ff;
        rm -f *.cpp;
        cd ..;
    fi

done
