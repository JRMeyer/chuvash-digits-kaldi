#!/bin/bash

while read line; do

    myarr=($line)
    len=${#myarr[@]}
    
    index=0
    cur_id=''
    old_id=''
    frame_i=0
    frame_j=0
    
    for i in $line; do

        cur_id=$i

        if [ "$index" -eq "0" ]; then
            # utt-id
            utt_id=$i
            ((index++))
        elif [ "$index" -eq "1" ]; then
            echo "FIRST FRAME"
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
            echo "LAST FRAME"
            echo "$utt_id $old_id $frame_i $frame_j"
        fi
        
    done;

done<exp_org/triphones_aligned/1.ali
