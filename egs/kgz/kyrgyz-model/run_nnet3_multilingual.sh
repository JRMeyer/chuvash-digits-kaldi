#!/bin/bash

# Copyright 2016 Pegah Ghahremani

# REQUIRED:
#
#    (1) baseline PLP features, and models (tri or mono) alignment (e.g. tri_ali or mono_ali)
#        for all languages
#    (2) input data (audio, lm, etc)
#
# This script can be used for training multilingual setup using different
# languages (specifically babel languages) with no shared phones.
#
# It will generate separate egs directory for each dataset and combine them
# during training.
#
# In the new multilingual training setup, mini-batches of data corresponding to
# different languages are randomly combined to generate egs.*.scp files
# using steps/nnet3/multilingual/combine_egs.sh and generated egs.*.scp files used
# for multilingual training.
#
# For all languages, we share all except last hidden layer and there is separate final
# layer per language.
#
# I removed ivectors and bottleneck feats
#
#


set -e

. ./path.sh
. ./utils/parse_options.sh

stage=0

lang_list=($1)
# this list of names 'mono' or 'ali' will
# be used for both finding the alignment dir
# and ...
typo_list=($2)
lang2weight=$3
run=$4

cmd="utils/run.pl"

exp_dir=exp/nnet3/multitask
master_egs_dir=$exp_dir/egs
num_langs=${#lang_list[@]}





if [ 1 ]; then

    #########################################
    ### SET VARIABLE NAMES AND PRINT INFO ###
    #########################################
    
    # Check data files from each lang
    # using ${typo_list[$i]}_ali for alignment dir
    for i in `seq 0 $[$num_langs-1]`; do
        for f in data/${lang_list[$i]}/train/{feats.scp,text} \
                      exp/${lang_list[$i]}/${typo_list[$i]}_ali/ali.1.gz \
                      exp/${lang_list[$i]}/${typo_list[$i]}_ali/tree; do
            [ ! -f $f ] && echo "$0: no such file $f" && exit 1;
        done
    done
    
    # Make lists of dirs for languages
    for i in `seq 0 $[$num_langs-1]`; do
        multi_data_dirs[$i]=data/${lang_list[$i]}/train
        multi_egs_dirs[$i]=exp/${lang_list[$i]}/nnet3/egs
        multi_ali_dirs[$i]=exp/${lang_list[$i]}/${typo_list[$i]}_ali
    done
    
    for i in `seq 0 $[$num_langs-1]`;do
        num_targets=`tree-info ${multi_ali_dirs[$i]}/tree 2>/dev/null | grep num-pdfs | awk '{print $2}'` || exit 1;
        
        echo ""
        echo "###### BEGIN LANGUAGE INFO ######"
        echo "lang= ${lang_list[$i]}"
        echo "num_targets= $num_targets"
        echo "data_dir= ${multi_data_dirs[$i]}"
        echo "ali_dir= ${multi_ali_dirs[$i]}"
        echo "egs_dir= ${multi_egs_dirs[$i]}"
        echo "###### END LANGUAGE INFO ######"
        echo ""
    done
fi

    
if [ $stage -le 8 ]; then

    echo "### ============================ ###";
    echo "### CREATE CONFIG FILES FOR NNET ###";
    echo "### ============================ ###";

    # Remove old generated files
    rm -rf $exp_dir
    for i in `seq 0 $[$num_langs-1]`; do
        rm -rf exp/${lang_list[$i]}/nnet3
    done

    mkdir -p $exp_dir/configs

    feat_dim=`feat-to-dim scp:${multi_data_dirs[0]}/feats.scp -`

    # The following definition is for the shared hidden layers of the nnet!
    cat <<EOF > $exp_dir/configs/network.xconfig
input dim=$feat_dim name=input
relu-renorm-layer name=tdnn1 input=Append(input@-2,input@-1,input,input@1,input@2) dim=50
relu-renorm-layer name=tdnn2 dim=50
relu-renorm-layer name=tdnn3 input=Append(-1,2) dim=50
# relu-renorm-layer name=tdnn4 input=Append(-3,3) dim=50
# relu-renorm-layer name=tdnn5 input=Append(-3,3) dim=50
# relu-renorm-layer name=tdnn6 input=Append(-7,2) dim=50
# adding the layers for diffrent language's output
EOF
    
    # Create separate outptut layer and softmax for all languages.
    
    for i in `seq 0 $[$num_langs-1]`;do
        
        num_targets=`tree-info ${multi_ali_dirs[$i]}/tree 2>/dev/null | grep num-pdfs | awk '{print $2}'` || exit 1;

        echo " relu-renorm-layer name=prefinal-affine-lang-${i} input=tdnn3 dim=50"
        echo " output-layer name=output-${i} dim=$num_targets max-change=1.5"
        
    done >> $exp_dir/configs/network.xconfig
    
    steps/nnet3/xconfig_to_configs.py \
        --xconfig-file $exp_dir/configs/network.xconfig \
        --config-dir $exp_dir/configs/ \
        --nnet-edits="rename-node old-name=output-0 new-name=output"

fi



if [ $stage -le 9 ]; then
        
    echo "### ================== ###"
    echo "### MAKE NNET3 EGS DIR ###"
    echo "### ================== ###"


    echo "### MAKE SEPARATE EGS DIR PER LANGUAGE ###"
    
    local/nnet3/prepare_multilingual_egs.sh \
        --cmd "$cmd" \
        --cmvn-opts "--norm-means=false --norm-vars=false" \
        --left-context 16 \
        --right-context 12 \
        $num_langs \
        ${multi_data_dirs[@]} \
        ${multi_ali_dirs[@]} \
        ${multi_egs_dirs[@]} \
        || exit 1;

    
    echo "### MAKE ONE BIG MULTILING EGS DIR ###"
        
    steps/nnet3/multilingual/combine_egs.sh \
        --cmd "$cmd" \
        --samples-per-iter 4000 \
        --lang2weight $lang2weight \
        $num_langs \
        ${multi_egs_dirs[@]} \
        $master_egs_dir \
        || exit 1;

fi



if [ $stage -le 11 ]; then

    echo "### ================= ###"
    echo "### TRAIN THE NNET!!! ###"
    echo "### ================= ###"

    steps/nnet3/train_raw_dnn.py \
        --stage=-5 \
        --cmd="$cmd" \
        --trainer.num-epochs 1 \
        --trainer.optimization.num-jobs-initial=1 \
        --trainer.optimization.num-jobs-final=1 \
        --trainer.optimization.initial-effective-lrate=0.0015 \
        --trainer.optimization.final-effective-lrate=0.00015 \
        --trainer.optimization.minibatch-size=256,128 \
        --trainer.samples-per-iter=4000 \
        --trainer.max-param-change=2.0 \
        --trainer.srand=0 \
        --feat.cmvn-opts="--norm-means=false --norm-vars=false" \
        --feat-dir ${multi_data_dirs[0]} \
        --egs.dir $master_egs_dir \
        --use-dense-targets false \
        --targets-scp ${multi_ali_dirs[0]} \
        --cleanup.remove-egs true \
        --use-gpu false \
        --dir=$exp_dir  \
        || exit 1;
    
fi




if [ $stage -le 12 ]; then

    echo "### ========================== ###"
    echo "### SPLIT & COPY NNET PER LANG ###"
    echo "### ========================== ###"
    
    for i in `seq 0 $[$num_langs-1]`;do
        lang_dir=$exp_dir/${lang_list[$i]}
        
        mkdir -p $lang_dir
        
        echo "$0: rename output name for each lang to 'output' and "
        echo "add transition model."
        
        nnet3-copy \
            --edits="rename-node old-name=output-$i new-name=output" \
            $exp_dir/final.raw \
            - | \
            nnet3-am-init \
                ${multi_ali_dirs[$i]}/final.mdl \
                - \
                $lang_dir/final.mdl \
            || exit 1;
        
        cp $exp_dir/cmvn_opts $lang_dir/cmvn_opts || exit 1;
        
        echo "$0: compute average posterior and readjust priors for language ${lang_list[$i]}."
        
        steps/nnet3/adjust_priors.sh \
            --cmd "$cmd" \
            --use-gpu false \
            --iter "final" \
            --use-raw-nnet false \
            $lang_dir ${multi_egs_dirs[$i]} \
            || exit 1;
    done
fi





if [ $stage -le 13 ]; then

    echo "### ============== ###"
    echo "### BEGIN DECODING ###"
    echo "### ============== ###"

    if [ "${typo_list[0]}" == "mono" ]; then
        echo "Decoding with monophone graph, make sure you compiled"
        echo "it with mkgraph.sh --mono (the flag is important!)"
    fi
    

    test_data_dir=data_org/test
    graph_dir=exp_org/${typo_list[0]}phones/graph
    decode_dir=${exp_dir}/decode
    final_model=${exp_dir}/org/final_adj.mdl
    
    mkdir -p $decode_dir

    unknown_phone="SPOKEN_NOISE"
    silence_phone="SIL"

    steps/nnet3/decode.sh \
        --nj 4 \
        --cmd $cmd \
        --max-active 300 \
        --min-active 100 \
        $graph_dir \
        $test_data_dir\
        $decode_dir \
        $final_model \
        $unknown_phone \
        $silence_phone \
        | tee $decode_dir/decode.log

    printf "\n#### BEGIN CALCULATE WER ####\n";

    # Concatenate langs to for WER filename
    cat_langs=""
    cat_typos=""
    for i in `seq 0 $[$num_langs-1]`; do
        cat_langs="${cat_langs}_${lang_list[$i]}"
        cat_typos="${cat_typos}_${typo_list[$i]}"
    done
    
    for x in ${decode_dir}*; do
        [ -d $x ] && grep WER $x/wer_* | utils/best_wer.sh > WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    done


    echo test_data_dir=$test_data_dir >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    echo graph_dir=$graph_dir >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    echo decode_dir=$decode_dir >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    echo final_model=$final_model >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    
    
    for i in `seq 0 $[$num_langs-1]`;do
        num_targets=`tree-info ${multi_ali_dirs[$i]}/tree 2>/dev/null | grep num-pdfs | awk '{print $2}'` || exit 1;
        
        echo "
    ###### BEGIN LANGUAGE INFO ######
    lang= ${lang_list[$i]}
    num_targets= $num_targets
    data_dir= ${multi_data_dirs[$i]}
    ali_dir= ${multi_ali_dirs[$i]}
    egs_dir= ${multi_egs_dirs[$i]}
    ###### END LANGUAGE INFO ######
    " >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    done

    echo "
    #######################
    ### BEGIN NNET INFO ###
    #######################
    " >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    
    nnet3-info $final_model >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;
    
    echo "
    #####################
    ### END NNET INFO ###
    #####################
    " >> WER_nnet3_multitask${cat_langs}${cat_typos}_${run}.txt;

fi


