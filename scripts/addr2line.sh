#!/bin/sh

if [ $# != 3 ]; then
	echo "Usage : addr2line.sh <exec-file> <addr-file> <function-file>"
	exit 
fi

rm $3 -f
touch $3

function print_tab()
{
	_cnt=$1
	_file=$2
	for ((i=0; i < $cnt; i++))
	do
		printf "\t" >> $_file
	done
}

cnt=0
cat $2 | while read line
do
	if [[ "$line" =~ "Enter" ]]; then
		read line1
		read line2
		call=$(addr2line -e $1 -f $line1 -s -p)
		this=$(addr2line -e $1 -f $line2 -s -p)
		print_tab $cnt $3
		printf "%s ===> %s\n" "$call" "$this" >> $3
		echo >> $3
		cnt=$((cnt + 1))
	elif [[ "$line" =~ "Exit" ]]; then
		cnt=$((cnt - 1))
		read line1
		read line2
		call=$(addr2line -e $1 -f $line1 -s -p)
		this=$(addr2line -e $1 -f $line2 -s -p)
		print_tab $cnt $3
		printf "%s <=== %s\n\n" "$call" "$this" >> $3
	fi
done
