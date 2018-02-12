#!/bin/bash

for run in {1..1}; do


    echo "####################"
    echo "### CLEAN UP GMM ###"
    echo "####################"
    
    # clean up from last gmm run
    rm -rf data_* exp_* plp_*


    echo "#####################"
    echo "### TRAIN NEW GMM ###"
    echo "#####################"
    
    for corpus in libri-org libri-boot1Quarter; do
        
        # clean up from last gmm run
        rm -rf input_$corpus/audio input_$corpus/phones.txt input_$corpus/transcripts
        
        ./run_gmm.sh $corpus $run

    done 

    echo "####################"
    echo "### CLEAN UP DNN ###"
    echo "####################"
    
    # clean up from last dnn run
    rm -rf exp data plp

    echo "#####################"
    echo "### TRAIN NEW DNN ###"
    echo "#####################"
    
    ./setup_multitask.sh "libri-org libri-boot1Quarter"

    ./run-run-nnet3.sh $run
    
done
