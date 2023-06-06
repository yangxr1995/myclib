#!/bin/bash

MUNIQ=./muniq

[ $# -ne 1 ] && echo "Usage : $0 <input-file>" && exit 1

INPUT=$1
OUTPUT=${1}.norepeat

TMP=./muniq.tmp
INPUT_TMP=./muniq.input

cp $INPUT $INPUT_TMP

while true
do
	wc -l $INPUT_TMP
	$MUNIQ -m 2000 -n 2 -o $TMP $INPUT_TMP  
	[ $? -eq 0 ] && break
	mv $TMP $INPUT_TMP	
done

mv $TMP $OUTPUT

rm -f $TMP $INPUT_TMP
