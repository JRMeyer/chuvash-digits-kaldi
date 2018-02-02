#!/bin/bash
# Joshua Meyer 2018

# input: egs.scp file output by prepare_multilingual_egs.sh (just get_egs.sh wrapper)
# output: scp file of same length, but resampled with replacement
#
#
#

infile=$1
outfile=$2

num_egs=(`wc -l $infile`)

proc_ids=()
iter=0
while [ $iter -lt $num_egs ]; do
    ((iter++));
    shuf $infile -n 1 >> $outfile &
    proc_ids+=($!)
done
for proc_id in ${proc_ids[*]}; do wait $proc_id; done;


LC_ALL=C sort $outfile -o $outfile

