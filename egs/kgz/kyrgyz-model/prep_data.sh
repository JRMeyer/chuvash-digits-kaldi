#!/bin/bash

# Joshua Meyer (2017)


# USAGE:
#
# 
# INPUT:
#
# 

input_dir=$1
data_dir=$2


if [ "$#" -ne 2 ]; then
    echo "ERROR: $0"
    echo "USAGE: $0 <input_dir> <data_dir>"
    exit 1
fi


check_input=1
prep_data=1


if [ "$check_input" -eq "1" ]; then
    
    printf "\n####=================####\n";
    printf "#### BEGIN CHECK INPUT ####\n";
    printf "####=================####\n\n";

    # sort input files by bytes (C-style) and re-save them with orginal filename
    for fileName in \
        lexicon.txt \
            lexicon_nosil.txt \
            phones.txt; do
        LC_ALL=C sort -i $input_dir/$fileName -o $input_dir/$fileName;
    done;
    
fi




if [ "$prep_data" -eq "1" ]; then
    
    printf "\n####=================####\n";
    printf "#### BEGIN DATA PREP ####\n";
    printf "####=================####\n\n";


    # Copy and paste existing phonetic dictionary, and phone list

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

    # copy the language mode to the working dir (i.e. ${data_dir}/local )
    cp $input_dir/task.arpabo $data_dir/local/lm.arpa

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


