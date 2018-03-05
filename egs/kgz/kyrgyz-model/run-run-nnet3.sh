#!/bin/bash


dim=500
num_epochs=10

###
### librispeech has 4.8 hours and atai has 1.3
###  1:2 == 1.875
###  1:1 == 3.75
###  2:1 == 7.5




# baseline
./run_nnet3_multilingual.sh "atai atai" "tri tri" "1.0,1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3



dnn="1-to-2"
weight="1.875"

rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

# mono
./run_nnet3_multilingual.sh "atai libri-boot1" "tri mono" "${weight},1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

# half
./run_nnet3_multilingual.sh "atai libri-half" "tri tri" "${weight},1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

# tri
./run_nnet3_multilingual.sh "atai libri-boot2" "tri tri" "${weight},1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

doubleWeight=( `echo "$weight * 2" | bc` )
# mono half
./run_nnet3_multilingual.sh "atai libri-boot1 libri-half" "tri mono tri" "${doubleWeight},1.0,1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

# half tri
./run_nnet3_multilingual.sh "atai libri-half libri-boot2" "tri tri tri" "${doubleWeight},1.0,1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

tripleWeight=( `echo "$weight * 3" | bc` )
# mono half tri
./run_nnet3_multilingual.sh "atai libri-boot1 libri-half libri-boot1" "tri mono tri tri" "${tripleWeight},1.0,1.0,1.0" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3


exit
