#!/bin/bash


dim=100
num_epochs=1

for run in {1..2}
do
    ./run_nnet3_multilingual.sh "org org" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"
  
    ./run_nnet3_multilingual.sh "org org2" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"
        
    ./run_nnet3_multilingual.sh "org no_voice" "tri tri" "0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"

    
    ./run_nnet3_multilingual.sh "org org org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"

    ./run_nnet3_multilingual.sh "org org2 no_voice" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "1gram_${num_epochs}epoch_5layer_${dim}dim_${run}"

done
