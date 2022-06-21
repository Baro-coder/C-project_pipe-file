#!/bin/bash

clear

echo "-######################################-"
echo "#-------------- BUILD -----------------#"
echo "-______________________________________-"
echo

gcc -o project.out project.c -pthread

if [ $? -eq 0 ]; then
    echo "Compiled."
    echo
else
    echo
    echo
    kill $$ SIGTERM
fi

echo "-######################################-"
echo "#--------------- RUN ------------------#"
echo "-______________________________________-"
echo

./project.out
