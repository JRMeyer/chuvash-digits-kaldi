#!/bin/bash


gmm=$1

dim=100
num_epochs=5




    


for dnn in {1..2}
do

    ################
    ### BASELINE ###
    ################

    
    ### A + B ###
    ## 2 baselines 
    ./run_nnet3_multilingual.sh "libri-org libri-org" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"

    ### A + B + C ###
    ## 3 baselines 
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"

    ### A + B + C + D ###
    ## 4 baselines
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" "0"

    ############
    ### LING ###
    ############

    
    ### A + B ###
    ## voice
    # ./run_nnet3_multilingual.sh "libri-org libri-voice" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"
    ## place
    # ./run_nnet3_multilingual.sh "libri-org libri-place" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"
    ## manner
    # ./run_nnet3_multilingual.sh "libri-org libri-manner" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"


    ### A + B + C ###
    ## voice + place
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-place" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"
    ## place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-place libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"
    ## manner + voice
    # ./run_nnet3_multilingual.sh "libri-org libri-manner libri-voice" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"

    ### A + B + C + D ###
    ## voice + place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner libri-place" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" "0"


    ###########
    ### ASR ###
    ###########

    ### A + B ###
    ## mono
    # ./run_nnet3_multilingual.sh "libri-org libri-mono" "tri mono" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"
    ## half
    # ./run_nnet3_multilingual.sh "libri-org libri-half" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"
    ## 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-3Quarters" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"

    ### A + B + C ###
    ## mono + half
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half" "tri mono tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"
    ## half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-half libri-3Quarters" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"
    ## 3/4 + mono
    # ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-mono" "tri tri mono" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"

    ### A + B + C + D ###
    ## mono + half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half libri-3Quarters" "tri mono tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" "0"



    #################
    ### BOOTSTRAP ###
    #################

    
    ### A + B ###
    ## 25per
    ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" ".25"
    ## 50per
    ./run_nnet3_multilingual.sh "libri-org libri-boot2Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" ".50"  
    ## 75per
    ./run_nnet3_multilingual.sh "libri-org libri-boot3Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" ".75"  

    # ### A + B + C ###
    # ## 25per + 50per
    # ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter libri-boot2Quarter" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "1"
    # ## 50per + 75per
    # ./run_nnet3_multilingual.sh "libri-org libri-boot2Quarter libri-boot3Quarter" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "1"  
    # ## 75per + 25per
    # ./run_nnet3_multilingual.sh "libri-org libri-boot3Quarter libri-boot1Quarter" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "1"  

    # ### A + B + C + D ###
    # ## 25per + 50per + 75per
    # ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter libri-boot2Quarter libri-boot3Quarter" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" "1"
    
    
done

