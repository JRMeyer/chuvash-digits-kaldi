#!/bin/bash


dim=250

for run in {1..20}
do
    ./run_nnet3_multilingual.sh "org org" "tri tri" "0.5,0.5" $dim "2gram_2epoch_5layer_${dim}dim_${run}"
        
    ./run_nnet3_multilingual.sh "org org2" "tri tri" "0.5,0.5" $dim "2gram_2epoch_5layer_${dim}dim_${run}"
    
    ./run_nnet3_multilingual.sh "org org org org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim "2gram_2epoch_5layer_${dim}dim_${run}"

    ./run_nnet3_multilingual.sh "org org org2 org2" "tri mono tri mono" "0.5,0.5,0.5,0.5" $dim "2gram_2epoch_5layer_${dim}dim_${run}"
    
done
