#!/bin/bash


dim=500
num_epochs=2

###
### librispeech has 4.86 hours and atai has 1.59
###  1:2 == 1.53
###  1:1 == 3.06
###  2:1 == 6.12



rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-org" "tri mono" "1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-voice" "tri mono" "1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-manner" "tri mono" "1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree" "0"


rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai-org/nnet3
rm -rf /data/MTL/exp/libri-org/nnet3
rm -rf /data/MTL/exp/libri-place/nnet3
rm -rf /data/MTL/exp/libri-manner/nnet3
rm -rf /data/MTL/exp/libri-voice/nnet3

./run_nnet3_multilingual.sh "atai-org libri-place" "tri mono" "1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree tree" "0"




exit




















# tripleWeight=( `echo "$weight * 3" | bc` )
# # mono half tri
# ./run_nnet3_multilingual.sh "atai libri-mono libri-half libri-tri" "tri mono tri tri" "${tripleWeight},1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}" "tree tree tree tree" "0"


