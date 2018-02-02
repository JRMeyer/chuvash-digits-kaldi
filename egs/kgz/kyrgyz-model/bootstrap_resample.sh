#!/bin/bash

infile=$1
outfile=$2

num_egs=(`wc -l $infile`)

iter=0
while [ $iter -lt $num_egs ]; do
    ((iter++))
    shuf $infile -n 1 >> $outfile
done

LC_ALL=C sort $outfile -o $outfile
