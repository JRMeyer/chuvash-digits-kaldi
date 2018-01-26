#!/bin/bash

feats_ark=$1

. ./path.sh


copy-feats ark:$feats_ark ark,t:tmp_txt_feats

echo "### REFORMAT to get <LABEL> <DATA>\n ###"
output=reformatted_${feats_ark##*/}

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
        echo "$utt_id $frame" >> $output;
    else
        frame=$line;
        echo "$utt_id $frame" >> $output;
    fi;
        
done<tmp_txt_feats
rm tmp_txt_feats

echo "$0: new feats in $output... use these to be predicted by new classifier"
