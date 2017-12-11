#!/bin/bash


dim=100
num_epochs=1

for run in {1..1}
do
    ./run_nnet3_multilingual.sh "org2 org2" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree"
    
    # ./run_nnet3_multilingual.sh "new new" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "250 250"
        
done
