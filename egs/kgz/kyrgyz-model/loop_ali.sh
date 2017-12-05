#!/bin/bash

# given an alignment ark file, return
# <utt-id> <trans-id> <start_frame> <end_frame>


ali_ark_file=$1

echo "$0: assuming $1 is a compressed alignments file"
gunzip -c $ali_ark_file > tmp.txt


while read line; do

    # get length of alignment
    myarr=($line)
    len=${#myarr[@]}

    # set some counters
    index=0
    cur_id=''
    old_id=''
    frame_i=0
    frame_j=0

    # looping over the transitionID-to-frame level alignments
    for i in $line; do

        cur_id=$i

        if [ "$index" -eq "0" ]; then
            # utt-id
            utt_id=$i
            ((index++))
        elif [ "$index" -eq "1" ]; then
            old_id=$i
            ((index++))
        elif [ "$cur_id" -eq "$old_id" ]; then
             # repeat trans-id
            ((index++))
        else
            # cur_id is not the same as old_id
            frame_j=$((index-1))
            echo "$utt_id $old_id $frame_i $frame_j"
            # reset for next id
            old_id=$cur_id
            frame_i=$((index-1))
            ((index++))
        fi
        

        if [ "$index" -eq "$len" ] ; then
            echo "$utt_id $old_id $frame_i $frame_j"
        fi
        
    done;

done<tmp.txt

rm tmp.txt
