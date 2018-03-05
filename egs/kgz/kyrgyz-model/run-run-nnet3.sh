#!/bin/bash


dim=500
num_epochs=10

###
### librispeech has 4.86 hours and atai has 1.59
###  1:2 == 1.53
###  1:1 == 3.06
###  2:1 == 6.11



dnn="1-to-2"
weight="1.53"


# dnn="1-to-1"
# weight="3.06"


# dnn="2-to-1"
# weight="6.12"



rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3


# baseline
./run_nnet3_multilingual.sh "atai atai" "tri tri" "1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_BASELINE" "tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3


# mono
./run_nnet3_multilingual.sh "atai libri-mono" "tri mono" "${weight},1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}"  "tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3

# half
./run_nnet3_multilingual.sh "atai libri-half" "tri tri" "${weight},1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}"  "tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3

# tri
./run_nnet3_multilingual.sh "atai libri-tri" "tri tri" "${weight},1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}"  "tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3

doubleWeight=( `echo "$weight * 2" | bc` )
# mono half
./run_nnet3_multilingual.sh "atai libri-mono libri-half" "tri mono tri" "${doubleWeight},1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}"  "tree tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3

# half tri
./run_nnet3_multilingual.sh "atai libri-half libri-tri" "tri tri tri" "${doubleWeight},1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}"  "tree tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3

tripleWeight=( `echo "$weight * 3" | bc` )
# mono half tri
./run_nnet3_multilingual.sh "atai libri-mono libri-half libri-tri" "tri mono tri tri" "${tripleWeight},1.0,1.0,1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim_weight-${dnn}" "tree tree tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-mono/nnet3
rm -rf /data/MTL/exp/libri-half/nnet3
rm -rf /data/MTL/exp/libri-tri/nnet3


exit
