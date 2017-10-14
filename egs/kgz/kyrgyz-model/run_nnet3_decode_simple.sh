#!/bin/bash
set -e
set -o pipefail

stage=0
cmd="utils/run.pl"



########################
##
## nnet3 model decoding
##
########################

if [ $stage -le 1 ]; then
    
    data_dir=/home/josh/git/kaldi/egs/kgz/kyrgyz-model/data/test
    graph_dir=/home/josh/git/kaldi/egs/kgz/kyrgyz-model/exp/triphones/graph
    model=/home/josh/git/kaldi/egs/kgz/kyrgyz-model/exp/nnet3/multitask/foo/final_adj.mdl
    decode_dir=/home/josh/git/kaldi/egs/kgz/kyrgyz-model/exp/nnet3/multitask/decode
    mkdir -p $decode_dir

    # steps/nnet3/decode.sh \
    #     --nj 4 \
    #     --cmd $cmd \
    #     --online-ivector-dir $ivector_dir \
    #     $graph_dir \
    #     $data_dir\
    #     $decode_dir \
    #     $model \
    #     | tee $decode_dir/decode.log
    
    steps/nnet3/decode.sh \
        --nj 16 \
        --cmd $cmd \
        $graph_dir \
        $data_dir\
        $decode_dir \
        $model \
        | tee $decode_dir/decode.log
    
fi



