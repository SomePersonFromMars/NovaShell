#!/bin/sh

. ./setup
. ./common/common

trap '' 2

if [ $# != 3 ]; then 
	echo Syntax: $0 shell_path suite_number test_number;
	exit 1;
fi

BASE_DIR=$PWD

TESTED_SHELL=$(readlink -f $1)
if [ -z  $TESTED_SHELL ]; then TESTED_SHELL=$(which $1); fi 
if [ -z  $TESTED_SHELL ]; then 
	echo shell file \" $1 \" not found
	exit 1
fi 

SUITE_NUMBER=$2
TEST_NUMBER=$3

SUITE_DIR=$BASE_DIR/$SUITES_DIR/$SUITE_NUMBER

cd $TEST_DIR

echo -n "Test " $TEST_NUMBER ": " 
head -1 $SUITE_DIR/$INPUT_DIR/$TEST_NUMBER.in
echo -n "Command:"

inf=$SUITE_DIR/$INPUT_DIR/$TEST_NUMBER.in
outf=$SUITE_DIR/$OUTPUT_DIR/$TEST_NUMBER.out
errf=$SUITE_DIR/$OUTPUT_DIR/$TEST_NUMBER.err

if [ -f $SUITE_DIR/$PRE_DIR/$TEST_NUMBER.pre ]; then
	. $SUITE_DIR/$PRE_DIR/$TEST_NUMBER.pre 	
fi

if [ -f $SUITE_DIR/$EXEC_DIR/$TEST_NUMBER.exec ]; then
	echo "specific see " $SUITE_DIR/$EXEC_DIR/$TEST_NUMBER.exec
	. $SUITE_DIR/$EXEC_DIR/$TEST_NUMBER.exec
else
	$TESTED_SHELL < $inf > $outf 2> $errf
	echo $TESTED_SHELL "<" $inf ">" $outf "2>"$errf
fi

if [ -f $SUITE_DIR/$EXPECTED_DIR/$TEST_NUMBER.gen ]; then
	. $SUITE_DIR/$EXPECTED_DIR/$TEST_NUMBER.gen 
fi


diff $outf $SUITE_DIR/$EXPECTED_DIR/$TEST_NUMBER.out > /dev/null
result1=$?

result2=0
if [ -f $SUITE_DIR/$EXPECTED_DIR/$TEST_NUMBER.err ]; then
	diff $errf $SUITE_DIR/$EXPECTED_DIR/$TEST_NUMBER.err > /dev/null
	result2=$?
fi

GREEN='\033[32m'
RED='\033[31m'
RESET='\033[0m'

if [ $result1 -eq 0 ] && [ $result2 -eq 0 ]
then
    printf '%bOK%b\n' "$GREEN" "$RESET"
else
    printf '%bFAIL%b\n' "$RED" "$RESET"
fi


if [ -f $SUITE_DIR/$POST_DIR/$TEST_NUMBER.post ]; then
	. $SUITE_DIR/$POST_DIR/$TEST_NUMBER.post 	
fi
