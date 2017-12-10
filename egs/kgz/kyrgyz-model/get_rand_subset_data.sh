#!/bin/bash

# this script takes in a file of labeled data where the first
# column is the LABEL and the following cols are data.
# one row per data point. space delimited

all_frames_labels=$1
num_sub=$2

if [ "$#" -ne "2" ]; then
    echo "$0: <labeled_data_file> <num_subset>"
    exit
fi

echo "$0: taking a random subset of ${num_sub} labels from $all_frames_labels"

# isolate labels and take random subset
cut -d " " -f 1 $all_frames_labels | \
    sort -u | \
    shuf | \
    head -${num_sub} \
         > sub_rand_labels

# save random subset of labels and data
while read frame_id; do
    grep "^${frame_id} " $all_frames_labels >> sub_rand_frames_labels;
done<sub_rand_labels
rm sub_rand_labels

echo ""
echo "$0: saving random subset of data to (1) sub_${num_sub}_labels.txt"
echo "$0:                                 (2) sub_${num_sub}_data.txt"
echo ""
cut -d" " -f1 sub_rand_frames_labels > sub_${num_sub}_labels.txt
cut -d" " -f2- sub_rand_frames_labels > sub_${num_sub}_data.txt
rm sub_rand_frames_labels

echo "$0: DONE"
