#!/usr/bin/env bash


test () {
    ./umpire2 tests/test$1/players.txt > tests/test$1/output.txt 2> tests/test$1/debug.txt
    diff tests/test$1/output.txt tests/test$1/expected.txt
    if [[ $? -eq 0 ]]
    then
        echo "Test $1 success"
    else
        echo "Test $1 failed"
    fi
    #rm tests/test$1/output.txt
}

make
make test

test 1
test 2
test 3
test 4