#!/bin/bash



for run in {1..20}
do
    ./run_nnet3_multilingual.sh "org babel" "tri tri" "0.5,0.5" "${run}"
  
    ./run_nnet3_multilingual.sh "org babel" "tri mono" "0.5,0.5" "${run}"
  
done
