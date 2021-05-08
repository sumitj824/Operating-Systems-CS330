#!/usr/bin/env bash


test () {
    ./umpire tests/test$1/player1 tests/test$1/player2 > tests/test$1/output.txt
    diff tests/test$1/output.txt tests/test$1/expected.txt
    if [[ $? -eq 0 ]]
    then
        echo "Test $1 success"
    else
        echo "Test $1 failed"
    fi
    rm tests/test$1/output.txt
}

make
make test

test 1
test 2
test 3
test 4