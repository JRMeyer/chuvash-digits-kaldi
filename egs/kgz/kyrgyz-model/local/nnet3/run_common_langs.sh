#!/bin/bash

# Copyright 2016 Pegah Ghahremani

# This script used to generate MFCC+pitch features for input language lang.

set -e
stage=1
train_stage=-10
cmd="utils/run.pl"
mfcc_config=""

. ./utils/parse_options.sh

lang=$1

if [ $# -ne 1 ]; then
  echo "Usage:$0 [options] <language-id>"
  echo "e.g. $0 102-assamese"
  exit 1;
fi



mfcc_dir=mfcc/$lang

if [ $stage -le 3 ]; then

    data_dir=data/$lang/train
    log_dir=exp/$lang/make_mfcc/train

    utils/copy_data_dir.sh \
        data/$lang/train \
        $data_dir \
        || exit 1;

    # scale the waveforms, this is useful as we don't use CMVN
    utils/data/perturb_data_dir_volume.sh \
        $data_dir \
        || exit 1;

    steps/make_mfcc.sh \
        --nj 70 \
        --mfcc-config $mfcc_config \
        --cmd "$cmd" \
        $data_dir \
        $log_dir \
        $mfcc_dir;

    steps/compute_cmvn_stats.sh \
        $data_dir \
        $log_dir \
        $mfcc_dir;

    # Remove the small number of utterances that couldn't be extracted for some
    # reason (e.g. too short; no such file).
    utils/fix_data_dir.sh ${data_dir};

fi
exit 0;
