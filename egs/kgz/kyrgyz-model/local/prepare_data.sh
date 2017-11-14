#!/bin/bash

# USAGE:
#
# local/prepare_data.sh \
#    $audio_dir \
#    $input_dir \
#    $train_percentage \
#    $data_dir \
#    $data_type


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
# │       ├── audio.list
# │       ├── audio_shuffled.list
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
data_dir=$3
# data_type = train or test
data_type=$4 


# Creating ./${data_dir} directory
mkdir -p ${data_dir}/local
mkdir -p ${data_dir}/local/tmp
cd ${data_dir}/local



##
## Check if utt IDs in transcripts and audio dir match
##

ls -1 ../../${audio_dir} > tmp/audio.list

awk -F"." '{print $1}' tmp/audio.list > tmp/utt-ids-audio.txt
awk -F" " '{print $1}' ../../${input_dir}/transcripts > tmp/utt-ids-transcripts.txt

for fileName in tmp/utt-ids-audio.txt tmp/utt-ids-transcripts.txt; do
    LC_ALL=C sort -i $fileName -o $fileName;
done;

diff tmp/utt-ids-audio.txt tmp/utt-ids-transcripts.txt > tmp/diff-ids.txt

if [ -s tmp/diff-ids.txt ]; then
    printf "\n####\n#### ERROR: Audio files & transcripts mismatch \n####\n\n";
    exit 0;
fi



###                        ###
### Most Important section ###
###                        ###

# make two-column lists of utt IDs and path to audio
../../local/create_kgz_wav_scp.pl $audio_dir tmp/audio.list > tmp/${data_type}_wav.scp
# make two-column lists of utt IDs and transcripts
../../local/create_kgz_txt.pl ../../${input_dir}/transcripts tmp/audio.list > tmp/${data_type}.txt
# copy the language mode to the working dir (i.e. ${data_dir}/local )
cp ../../${input_dir}/task.arpabo lm.arpa
cd ../..


mkdir -p ${data_dir}/${data_type}
cp ${data_dir}/local/tmp/${data_type}_wav.scp ${data_dir}/${data_type}/wav.scp
cp ${data_dir}/local/tmp/${data_type}.txt ${data_dir}/${data_type}/text

cat ${data_dir}/${data_type}/text | \
    awk '{printf("%s %s\n", $1, $1);}' > ${data_dir}/${data_type}/utt2spk

utils/utt2spk_to_spk2utt.pl <${data_dir}/${data_type}/utt2spk \
                            >${data_dir}/${data_type}/spk2utt


#clean up temp files
rm -rf ${data_dir}/local/tmp




## get total number of seconds of WAVs in wav.scp
TOTAL_SECS=0
while IFS='' read -r line || [[ -n "$line" ]]; do
    line=( $line )
    file=${line[1]}
    SECS="$( soxi -D $file )"
    TOTAL_SECS=$( echo "$TOTAL_SECS + $SECS" | bc )
done < "${data_dir}/${data_type}/wav.scp"

# Calculate hours and print to screen
total_hours=$( echo "scale=2;$TOTAL_SECS / 60 / 60" | bc )
echo ""
echo " $total_hours hours of audio for training "
echo " in ${data_dir}/${data_type}/wav.scp"
echo ""
