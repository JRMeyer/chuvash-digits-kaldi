#!/bin/bash

#decode new audio files with nnet2 model

exp_dir=exp_org
data_dir=new_data



# DO I NEED TO ADD DELTAS?


prep_data=0
if [ "$prep_data" -eq "1" ]; then

    # make wav.scp
    for i in *.wav; do
        echo "${i%.wav} `pwd`/$i" >> wav.scp;
    done
    
    
    # EXTRACT PLP FEATS
    ../../../src/featbin/compute-plp-feats \
        --config=config/plp.conf \
        scp:${data_dir}/wav.scp \
        ark,scp:${data_dir}/feats.ark,${data_dir}/feats.scp;
    
    
    # COMPUTE CMVN
    ../../../src/featbin/compute-cmvn-stats \
        --spk2utt=ark:${data_dir}/utt2spk \
        scp:${data_dir}/feats.scp \
        ark,scp:${data_dir}/cmvn.ark,${data_dir}/cmvn.scp;
    
    # APPLY CMVN
    ../../../src/featbin/apply-cmvn \
        --norm-means=false \
        --norm-vars=false \
        --utt2spk=ark:${data_dir}/utt2spk \
        scp:${data_dir}/cmvn.scp \
        scp:${data_dir}/feats.scp \
        ark,scp:${data_dir}/new-feats.ark,${data_dir}/new-feats.scp;
fi



decode_audio=1
if [ "$decode_audio" -eq "1" ]; then
    
    # TRAINED DNN-HMM + FEATURE VECTORS --> LATTICE
    ../../../src/nnet2bin/nnet-latgen-faster \
        --max-active=7000 \
        --min-active=200 \
        --beam=15 \
        --lattice-beam=10 \
        --acoustic-scale=0.1 \
        --allow-partial=true \
        --word-symbol-table=${exp_dir}/triphones/graph/words.txt \
        ${exp_dir}/nnet2/nnet2_simple/final.mdl \
        ${exp_dir}/triphones/graph/HCLG.fst \
        ark:${data_dir}/feats.ark \
        ark,t:${data_dir}/lattices.ark;
    
    
    # LATTICE --> BEST PATH THROUGH LATTICE
    ../../../src/latbin/lattice-best-path \
        --word-symbol-table=${exp_dir}/triphones/graph/words.txt \
        ark:${data_dir}/lattices.ark \
        ark,t:${data_dir}/one-best.tra;
    
    # BEST PATH INTERGERS --> BEST PATH WORDS
    utils/int2sym.pl \
        -f 2- \
        ${exp_dir}/triphones/graph/words.txt \
        ${data_dir}/one-best.tra \
        > ${data_dir}/one-best-hypothesis.txt;
fi
