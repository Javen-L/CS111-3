#!/bin/bash

# NAME: Anh Mac
# EMAIL: anhmvc@gmail.com
# UID: 905111606

echo | ./lab4b --bogus &> /dev/null
if [[ $? -ne 1 ]]
then
    echo "Test for bogus argument failed"
else
    echo "Test for bogus argument passed"
fi 

echo | ./lab4b --period=bad &> /dev/null
if [[ $? -ne 1 ]]
then
    echo "Test for bad period argument failed"
else
    echo "Test for bad period argument passed"
fi 

echo | ./lab4b --scale=bad &> /dev/null
if [[ $? -ne 1 ]]
then
    echo "Test for bad scale argument failed"
else
    echo "Test for bad scale argument passed"
fi 

echo | ./lab4b --period=3 --scale=F --log=test.txt <<-EOF
PERIOD=2
SCALE=C
SCALE=F
STOP
START
LOG MESSAGE
OFF
EOF
if [[ $? -ne 0 ]]
then
    echo "Test failed with bad exit code for correct input test"
else
    echo "Test with correct input passed"
fi 

if [ ! -s test.txt ]
then
	echo "Log file was not created"
else
	echo "Log file was created"
fi

grep "PERIOD=2" test.txt &> /dev/null; \
if [[ $? -ne 0 ]]
then
        echo "PERIOD was not logged"
else
        echo "PERIOD was logged"
fi

grep "SCALE=C" test.txt &> /dev/null; \
if [[ $? -ne 0 ]]
then
	echo "SCALE=C was not logged"
else
	echo "SCALE=C was logged"
fi

grep "SCALE=F" test.txt &> /dev/null; \
if [[ $? -ne 0 ]]
then
	echo "SCALE=F was not logged"
else
	echo "SCALE=F was logged"
fi

grep "LOG MESSAGE" test.txt &> /dev/null; \
if [[ $? -ne 0 ]]
then
        echo "LOG MESSAGE was not logged"
else
	echo "LOG MESSAGE was logged"
fi

grep "SHUTDOWN" test.txt &> /dev/null; \
if [[ $? -ne 0 ]]
then
        echo "SHUTDOWN was not logged"
else
	echo "SHUTDOWN was logged"
fi

rm -f test.txt