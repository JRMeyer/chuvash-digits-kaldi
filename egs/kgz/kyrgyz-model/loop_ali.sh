#!/bin/bash

while read line; do

    index=-1
    cur_id=''
    old_id=''
    frame_i=-1
    frame_j=-1
    
    for i in $line; do

        cur_id=$i

        if [ "$index" -eq "0" ]; then
            # utt-id
            utt_id=$i
        elif [ "$index" -eq "1" ]; then
            # first frame of alignment
            old_id=$i
            frame_i=0
        elif [ "$cur_id" -eq "$old_id" ]; then
             # repeat trans-id
            :
        else
            # cur_id is not the same as old_id
            frame_j=$index
            echo "$utt_id $old_id $frame_i $frame_j"

            # reset for next id
            old_id=$cur_id
            frame_i=$index
        fi

        ((index++))

    done;

done<exp_org/triphones_aligned/1.ali
