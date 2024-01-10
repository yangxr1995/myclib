#!/bin/sh

if [ $# != 3 ]; then
	echo "Usage : addr2line.sh <exec-file-location> <addr-file> <function-file>"
	exit 
fi

rm $3 -f
touch $3

old_ratio=-1
function progress_bar()
{
	local ratio=$1
	local mark=""

	[ $ratio -gt 100 ] && ratio=100

	[ $old_ratio -ne $ratio ] && old_ratio=$ratio || return

	for ((i=0;${i}<=${ratio};i+=1))
	do
			mark="#${mark}"
	done

	printf "progress:[%-101s]%d%%\r" "${mark}" "${ratio}"
}

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

	call=$(addr2line -e $call_bin -f $call_addr -s -p | c++filt 2>/dev/null)
	this=$(addr2line -e $this_bin -f $this_addr -s -p | c++filt 2>/dev/null)
}

acct_sum=$(wc -l $2 | awk '{print $1}')
acct_cnt=0

echo "Please Wait.."

cnt=0
cat $2 | while read line
do
	acct_cnt=$((acct_cnt + 1))

	if [[ "$line" =~ "Enter" ]]; then

		read line1
		acct_cnt=$((acct_cnt + 1))
		read line2
		acct_cnt=$((acct_cnt + 1))

		parse_line $line1 $line2 $1

		print_tab $cnt $3
		printf "%s[%s] ===> %s[%s]\n" "$call" "$call_sym" "$this" "$this_sym" >> $3
		echo >> $3
		cnt=$((cnt + 1))
	elif [[ "$line" =~ "Exit" ]]; then
		cnt=$((cnt - 1))

		read line1
		acct_cnt=$((acct_cnt + 1))
		read line2
		acct_cnt=$((acct_cnt + 1))

		parse_line $line1 $line2 $1

		print_tab $cnt $3
		printf "%s[%s] <=== %s[%s]\n\n" "$call" "$call_sym" "$this" "$this_sym" >> $3
	fi
		
	acct_ra=$(($((acct_cnt * 100)) / acct_sum))
	progress_bar ${acct_ra}
done

echo ""
