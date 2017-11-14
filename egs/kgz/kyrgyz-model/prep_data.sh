#!/bin/bash

# Joshua Meyer (2017)


# USAGE:
#
# 
# INPUT:
#
# 

input_dir=$1
audio_dir=$2
data_dir=$3

if [ "$#" -ne 3 ]; then
    echo "ERROR: $0"
    echo "USAGE: $0 <input_dir> <audio_dir>"
    exit 1
fi


check_input=1
prep_data=1

if [ "$check_input" -eq "1" ]; then
    
    printf "\n####=================####\n";
    printf "#### BEGIN CHECK INPUT ####\n";
    printf "####=================####\n\n";

    echo "$0: looking for audio data in $audio_dir"
    
    # Make sure we have the audio data (WAV file utterances)
    if [ ! -d $audio_dir ]; then
        printf '\n####\n#### ERROR: '"${audio_dir}"' not found \n####\n\n';
        exit 1;
    fi

    # sort input files by bytes (C-style) and re-save them with orginal filename
    for fileName in \
        lexicon.txt \
            lexicon_nosil.txt \
            phones.txt \
            transcripts; do
        LC_ALL=C sort -i ${input_dir}/${fileName} -o ${input_dir}/${fileName};
    done;
    
fi




if [ "$prep_data" -eq "1" ]; then
    
    printf "\n####=================####\n";
    printf "#### BEGIN DATA PREP ####\n";
    printf "####=================####\n\n";

    # Given dir of WAV files, create dir for train, create 'wav.scp',
    # create 'text', create 'utt2spk' and 'spk2utt', and copy the language model
    # from elsewhere (ARPA format)

    local/prepare_data.sh \
        $audio_dir \
        $input_dir \
        $data_dir \
        train \
        || printf "\n####\n#### ERROR: prepare_data.sh \n####\n\n" \
        || exit 1;

    # Copy and paste existing phonetic dictionary, language model, and phone list

    local/prepare_dict.sh \
        $data_dir \
        $input_dir \
        "SIL" \
        || printf "\n####\n#### ERROR: prepare_dict.sh\n####\n\n" \
        || exit 1;

    # Prepare a dir such as data/lang/
    # This script can add word-position-dependent phones, and constructs a host of
    # other derived files, that go in data/lang/.
    # This creates the FST for the lexicon.

    local/prepare_lang.sh \
        --position-dependent-phones false \
        $data_dir/local/dict \
        $data_dir/local/lang \
        $data_dir/lang \
        "<unk>" \
        || printf "\n####\n#### ERROR: prepare_lang.sh\n####\n\n" \
        || exit 1;

    # Create the FST (G.fst) for the grammar

    local/prepare_lm.sh \
        $data_dir \
        || printf "\n####\n#### ERROR: prepare_lm.sh\n####\n\n" \
        || exit 1;

    printf "\n####===============####\n";
    printf "#### END DATA PREP ####\n";
    printf "####===============####\n\n";

fi



exit;


