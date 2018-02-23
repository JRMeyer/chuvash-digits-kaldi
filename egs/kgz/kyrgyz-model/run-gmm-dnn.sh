#!/bin/bash

for gmm in {3..4}; do



    echo "#####################"
    echo "### TRAIN NEW GMM ###"
    echo "#####################"

    
    
    # clean up from last gmm run
    rm -rf data_* exp_* plp_*

    for corpus in libri-boot2Quarter libri-boot3Quarter libri-org libri-boot1Quarter; do
        
        # clean up from last gmm run
        rm -rf input_$corpus/audio input_$corpus/phones.txt input_$corpus/transcripts

        # use env -i to run script in clean environment
        env -i ./run_gmm.sh $corpus $gmm

    done
    

    
    # echo "#####################"
    # echo "### TRAIN NEW DNN ###"
    # echo "#####################"

    # # clean up from last dnn run
    # rm -rf exp data plp
    
    # ./setup_multitask.sh "libri-org libri-boot1Quarter libri-boot2Quarter libri-boot3Quarter"

    # env -i ./run-run-nnet3.sh $gmm
    
done


sudo shutdown now