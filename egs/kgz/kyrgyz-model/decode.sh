#!/bin/bash

#decode new audio files with nnet2 model

exp_dir=exp_org
data_dir=new_data

# this script assumes the final model is 1 dir up from
# the decoding dir: aka the last dir of the arguments

steps/decode.sh \
    --cmd "utils/run.pl" \
    --nj `nproc` \
    --beam 13 \
    --lattice-beam 7 \
    --max-active 700 \
    --skip-scoring "true" \
    exp_org/triphones/graph \
    new_data \
    exp_org/triphones/decode_new_data \
    "SPOKEN_NOISE" \
    "SIL" \
    || printf "\n####\n#### ERROR: decode.sh \n####\n\n" \
    || exit 1;
