#!/usr/bin/env bash

test () {
	# remove old output files if any
	[ -f output$1.txt ] && rm output$1.txt
	
	./executeCommand "$2" > output$1.txt 
	diff output$1.txt tests/expected_outputs/expected_output$1.txt > /dev/null
	
	if [[ $? -eq 0 ]]
	then
		echo "TEST $1 PASSED"
	else
		echo "TEST $1 FAILED"
	fi
	# [ -f output$1.txt ] && rm output$1.txt
}

# set the environment variable
export CS330_PATH="tests/bin:tests/bin/sbin:tests/bin/ubin"

# cleanup old executable 
[ -f executeCommand ] && rm executeCommand

# compile
gcc -o executeCommand executeCommand.c

test 1 "grep -a"
test 2 "grep"
test 3 "ls"
test 4 "unknwn"
