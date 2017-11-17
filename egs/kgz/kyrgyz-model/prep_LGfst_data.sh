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


prep_data=1


if [ "$prep_data" -eq "1" ]; then
    
    echo "### Setup L.fst ###"
    
    for i in lexicon.txt lexicon_nosil.txt phones.txt; do
        LC_ALL=C sort -i $input_dir/$i -o $input_dir/$i;
    done;
    
    # move lexicon files
    local/prepare_dict.sh \
        $data_dir \
        $input_dir \
        "SIL" \
        || printf "\n####\n#### ERROR: prepare_dict.sh\n####\n\n" \
        || exit 1;

    # create L.fst
    local/prepare_lang.sh \
        --position-dependent-phones false \
        $data_dir/local/dict \
        $data_dir/local/lang \
        $data_dir/lang \
        "<unk>" \
        || printf "\n####\n#### ERROR: prepare_lang.sh\n####\n\n" \
        || exit 1;

    

    echo "### Setup G.fst ###"

    # move language model
    cp $input_dir/task.arpabo $data_dir/local/lm.arpa

    # create G.fst
    local/prepare_lm.sh \
        $data_dir \
        || printf "\n####\n#### ERROR: prepare_lm.sh\n####\n\n" \
        || exit 1;

    
    printf "\n####===============####\n";
    printf "#### END DATA PREP ####\n";
    printf "####===============####\n\n";

fi



exit;


