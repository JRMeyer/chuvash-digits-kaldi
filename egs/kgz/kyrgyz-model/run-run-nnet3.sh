#!/bin/bash



for run in {1..25}
do
    ./run_nnet3_multilingual.sh "org org2" "tri tri" "0.5,0.5" "${run}"
  
    ./run_nnet3_multilingual.sh "org org2" "tri mono" "0.5,0.5" "${run}"
  
    ./run_nnet3_multilingual.sh "org no_voice" "tri mono" "0.5,0.5" "${run}"
    
    ./run_nnet3_multilingual.sh "org no_voice" "tri tri" "0.5,0.5" "${run}"
done
