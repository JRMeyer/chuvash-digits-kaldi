#!/bin/bash


dim=100
num_epochs=5


for run in {1..1}
do

    # # 2 baselines 
    # ./run_nnet3_multilingual.sh "libri-org libri-org" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree"
    # # 3 baselines 
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # 4 baselines
    # ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

    
    # # voice + place
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-place" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-place libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # manner + voice
    # ./run_nnet3_multilingual.sh "libri-org libri-manner libri-voice" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # voice + place + manner
    # ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner libri-place" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

    
    # # mono + half
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half" "tri mono tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-half libri-3Quarters" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # 3/4 + mono
    # ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-mono" "tri tri mono" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    # # mono + half + 3/4
    # ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half libri-3Quarters" "tri mono tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

    
    # # bootstrap1
    # ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_1" "tree tree"   
    # bootstrap2
    ./run_nnet3_multilingual.sh "libri-org libri-boot2Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_1" "tree tree"   
    # # bootstrap3
    # ./run_nnet3_multilingual.sh "libri-org libri-boot3Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_1" "tree tree"   
done

# sudo shutdown now
