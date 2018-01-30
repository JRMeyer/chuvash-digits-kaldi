#!/bin/bash


dim=100
num_epochs=1

for run in {1..1}
do
     ./run_nnet3_multilingual.sh "libri-org libri-org" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree"
            
done
