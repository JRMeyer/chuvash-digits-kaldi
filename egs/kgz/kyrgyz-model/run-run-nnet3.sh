#!/bin/bash


dim=500
num_epochs=10

for run in {2..20}
do
    ./run_nnet3_multilingual.sh "org org" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"
        
    ./run_nnet3_multilingual.sh "org org2" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"
    
done
