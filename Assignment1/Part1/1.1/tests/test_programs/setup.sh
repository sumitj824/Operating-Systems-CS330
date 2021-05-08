#!/usr/bin/env bash

# remove old executables
[ -f ls ] && rm ls
[ -f grep ] && rm grep
[ -f ps ] && rm ps

# compile 
gcc -o ls ls.c
gcc -o grep grep.c
gcc -o ps ps.c

# clean the old directory structure
[ -f ../bin/ps ] && rm ../bin/ps
[ -f ../bin/ls ] && rm ../bin/ls
[ -f ../bin/sbin/ps ] && rm ../bin/sbin/ps
[ -f ../bin/sbin/grep ] && rm ../bin/sbin/grep
[ -f ../bin/ubin/grep ] && rm ../bin/ubin/grep

# copy to the correct directory structure
cp ls ../bin/ls
touch ../bin/ps
cp ps ../bin/sbin/ps
touch ../bin/sbin/grep
cp grep ../bin/ubin/grep
