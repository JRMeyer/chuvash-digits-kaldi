#!/bin/bash



for run in {1..2}
do
    time ./run_nnet3_multilingual.sh "org no_voice" "tri mono" "0.5,0.5" "${run}"
    time ./run_nnet3_multilingual.sh "org no_voice" "tri tri" "0.5,0.5" "${run}"
done
