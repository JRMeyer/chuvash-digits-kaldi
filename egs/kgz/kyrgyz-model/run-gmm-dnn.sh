#!/bin/bash




# for gmm in {1..100}; do



#     echo "#####################"
#     echo "### TRAIN NEW GMM ###"
#     echo "#####################"

    
    
#     # clean up from last gmm run
#     rm -rf data_* exp_* plp_*

    for corpus in atai-org; do
        
        # clean up from last gmm run
        rm -rf input_$corpus/audio input_$corpus/phones.txt input_$corpus/transcripts

        # use env -i to run script in clean environment
        env -i ./run_gmm.sh $corpus 2-apr-2018

    done
    

    
#     echo "#####################"
#     echo "### TRAIN NEW DNN ###"
#     echo "#####################"

#     # clean up from last dnn run
#     rm -rf exp data plp
    
#     ./setup_multitask.sh "libri-org libri-boot1Quarter libri-boot2Quarter libri-boot3Quarter"

#     env -i ./run-run-nnet3.sh $gmm
    
# done


