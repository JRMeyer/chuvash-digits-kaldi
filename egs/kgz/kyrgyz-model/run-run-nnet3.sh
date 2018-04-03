#!/bin/bash


dim=500
num_epochs=2



rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-voice libri-place" "tri mono mono" "1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-voice libri-manner" "tri mono mono" "1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-place libri-manner" "tri mono mono" "1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-place libri-manner libri-voice" "tri mono mono mono" "1.0,1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree tree tree" "0"




exit
