#!/bin/bash

# USAGE:
#
# local/prepare_data.sh \
#    $audio_dir \
#    $input_dir \
#    $train_percentage \
#    $data_dir \
#    $train_dir \
#    $test_dir;

# INPUT:
#
#    audio_dir/
#
#    input_dir/
#      task.arpabo
#      transcripts
#
#    train_percentage

# OUTPUT:
# data_org/
# ├── local
# │   ├── lm.arpa
# │   └── tmp
# │       ├── audio_all.list
# │       ├── audio_all_shuffled.list
# │       ├── audio.test
# │       ├── audio.train
# │       ├── diff-ids.txt
# │       ├── test.txt
# │       ├── test_wav.scp
# │       ├── train.txt
# │       ├── train_wav.scp
# │       ├── utt-ids-audio.txt
# │       └── utt-ids-transcripts.txt
# ├── test
# │   ├── spk2utt
# │   ├── text
# │   ├── utt2spk
# │   └── wav.scp
# └── train
#     ├── spk2utt
#     ├── text
#     ├── utt2spk
#     └── wav.scp

# 4 directories, 20 files



audio_dir=$1
input_dir=$2
trainPercentage=$3
data_dir=$4
train_dir=$5
test_dir=$6

# Creating ./${data_dir} directory
mkdir -p ${data_dir}/local
# create temporary dir for files we need along the way
mkdir -p ${data_dir}/local/tmp

cd ${data_dir}/local

# print all the filenames from the model/audio_dir to the text file:
# model/data_dir/local/audio_all.list
ls -1 ../../${audio_dir} > tmp/audio_all.list

# compare the files we actually find in the audio_dir with
# the utterance-ids we have in the input/transcripts file
# throw an error if they don't match
awk -F"." '{print $1}' tmp/audio_all.list > tmp/utt-ids-audio.txt
awk -F" " '{print $1}' ../../${input_dir}/transcripts > tmp/utt-ids-transcripts.txt

for fileName in tmp/utt-ids-audio.txt tmp/utt-ids-transcripts.txt; do
    LC_ALL=C sort -i $fileName -o $fileName;
done;

diff tmp/utt-ids-audio.txt tmp/utt-ids-transcripts.txt > tmp/diff-ids.txt

if [ -s tmp/diff-ids.txt ]; then
    printf "\n####\n#### ERROR: Audio files & transcripts mismatch \n####\n\n";
    exit 0;
fi

# shuffle audio ids so we get diff train and test sets each time
cat tmp/audio_all.list | shuf >> tmp/audio_all_shuffled.list


# split the complete list of wave files from audio_all.list into a train and
# test set, and print two new text files of the filenames for test and training
../../local/split_datasets_test_train.pl \
    tmp/audio_all_shuffled.list \
    tmp/audio.test \
    tmp/audio.train \
    $trainPercentage \
    || printf "\n####\n#### ERROR: $0 \n####\n\n";

# sort input files by bytes (kaldi-style) and re-save them with orginal filename
for fileName in tmp/audio_all.list tmp/audio.test tmp/audio.train; do
    LC_ALL=C sort -i $fileName -o $fileName;
done;

# make two-column lists of test and train utterance ids and their paths
../../local/create_kgz_wav_scp.pl $audio_dir tmp/audio.test > tmp/${test_dir}_wav.scp
../../local/create_kgz_wav_scp.pl $audio_dir tmp/audio.train > tmp/${train_dir}_wav.scp

# make two-column lists of test and train utterance ids and transcripts
../../local/create_kgz_txt.pl ../../${input_dir}/transcripts tmp/audio.train > tmp/${train_dir}.txt
../../local/create_kgz_txt.pl ../../${input_dir}/transcripts tmp/audio.test > tmp/${test_dir}.txt

# copy the language mode to the working dir (i.e. ${data_dir}/local )
cp ../../${input_dir}/task.arpabo lm.arpa

cd ../..

for dir in $train_dir $test_dir; do

    mkdir -p ${data_dir}/${dir}
    cp ${data_dir}/local/tmp/${dir}_wav.scp ${data_dir}/${dir}/wav.scp
    cp ${data_dir}/local/tmp/${dir}.txt ${data_dir}/${dir}/text

    cat ${data_dir}/${dir}/text | \
        awk '{printf("%s %s\n", $1, $1);}' > ${data_dir}/${dir}/utt2spk

    utils/utt2spk_to_spk2utt.pl <${data_dir}/${dir}/utt2spk \
        >${data_dir}/${dir}/spk2utt
done

#clean up temp files
rm -rf ${data_dir}/local/tmp


## get total number of seconds of WAVs in wav.scp
TOTAL_SECS=0
while IFS='' read -r line || [[ -n "$line" ]]; do
    line=( $line )
    file=${line[1]}
    SECS="$( soxi -D $file )"
    TOTAL_SECS=$( echo "$TOTAL_SECS + $SECS" | bc )
done < "${data_dir}/train/wav.scp"

# Calculate hours and print to screen
total_hours=$( echo "scale=2;$TOTAL_SECS / 60 / 60" | bc )
echo ""
echo " $total_hours hours of audio for training "
echo " in ${data_dir}/train/wav.scp"
echo ""
