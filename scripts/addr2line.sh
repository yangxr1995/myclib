#!/bin/sh

if [ $# != 3 ]; then
	echo "Usage : addr2line.sh <exec-file-location> <addr-file> <function-file>"
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

function parse_line()
{
	call_sym=$(echo $1 | awk -F: '{print $1}')
	call_bin="${3}/${call_sym}"
	call_addr=$(echo $1 | awk -F: '{print $2}')

	this_sym=$(echo $2 | awk -F: '{print $1}')
	this_bin="${3}/${this_sym}"
	this_addr=$(echo $2 | awk -F: '{print $2}')

	call=$(addr2line -e $call_bin -f $call_addr -s -p)
	this=$(addr2line -e $this_bin -f $this_addr -s -p)
}

cnt=0
cat $2 | while read line
do
	if [[ "$line" =~ "Enter" ]]; then
		read line1
		read line2

		parse_line $line1 $line2 $1

		print_tab $cnt $3
		printf "%s[%s] ===> %s[%s]\n" "$call" "$call_sym" "$this" "$this_sym" >> $3
		echo >> $3
		cnt=$((cnt + 1))
	elif [[ "$line" =~ "Exit" ]]; then
		cnt=$((cnt - 1))
		read line1
		read line2

		parse_line $line1 $line2 $1

		print_tab $cnt $3
		printf "%s[%s] <=== %s[%s]\n\n" "$call" "$call_sym" "$this" "$this_sym" >> $3
	fi
done
