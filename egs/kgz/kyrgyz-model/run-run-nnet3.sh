#!/bin/bash



for run in {1..20}
do
    for dim in 100 150 200 250 300 350 400 450 500
    do
        ./run_nnet3_multilingual.sh "org no_voice" "tri mono" "0.5,0.5" $dim "2gram_5epoch_5layer_${dim}dim_${run}"
        
    done
done
