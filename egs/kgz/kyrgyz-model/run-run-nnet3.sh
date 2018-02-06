#!/bin/bash


dim=100
num_epochs=5


for run in {5..10}
do

    # # 2 baselines 
    ./run_nnet3_multilingual.sh "libri-org libri-org" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree" "0"
    # # 3 baselines 
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # 4 baselines
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree" "0"

    
    # # voice + place
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-place" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-place libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # manner + voice
    # ./run_nnet3_multilingual.sh "libri-org libri-manner libri-voice" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # voice + place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner libri-place" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree" "0"

    
    # # mono + half
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half" "tri mono tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-half libri-3Quarters" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # 3/4 + mono
    # ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-mono" "tri tri mono" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree" "0"
    # # mono + half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half libri-3Quarters" "tri mono tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree" "0"

    
    # # bootstrap1
    ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree" "1"
    # bootstrap2
    ./run_nnet3_multilingual.sh "libri-org libri-boot2Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree" "1"  
    # # bootstrap3
    ./run_nnet3_multilingual.sh "libri-org libri-boot3Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree" "1"  
done

# sudo shutdown now
