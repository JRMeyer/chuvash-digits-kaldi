#!/bin/bash

feats_ark=$1

. ./path.sh


copy-feats ark:$feats_ark ark,t:tmp_txt_feats

echo "### REFORMAT to get <LABEL> <DATA>\n ###"

utt_id=''
frame=''

while read line; do
    if `echo $line | grep -q "\["` ; then
        i=($line);
        utt_id=${i[0]};        
    elif `echo $line | grep -q "\]"`; then
        i=($line);
        unset "i[${#i[@]}-1]";
        frame="${i[@]}";
        echo "$utt_id $frame" >> reformatted_${feats_ark##*/};
    else
        frame=$line;
        echo "$utt_id $frame" >> reformatted_${feats_ark##*/};
    fi;
        
done<tmp_txt_feats
