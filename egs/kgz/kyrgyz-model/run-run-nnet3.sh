#!/bin/bash


dim=100
num_epochs=5

<<<<<<< HEAD


for run in {2..5}
do

#    ./run_nnet3_multilingual.sh "libri-org libri-boot" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_1" "tree tree"
    
    ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"
    
    ./run_nnet3_multilingual.sh "libri-org libri-voice libri-place" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-place libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner libri-place" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"
    
    ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half" "tri mono tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-half libri-3Quarters" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-mono" "tri tri mono" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
    ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-half libri-mono" "tri tri tri mono" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"
    
done

sudo shutdown now
=======
./run_nnet3_multilingual.sh "libri-org libri-boot" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_1" "tree tree"

exit


for run in {1..1}
do
     
     ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

     ./run_nnet3_multilingual.sh "libri-org libri-voice libri-place" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-place libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-voice libri-manner libri-place" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

     ./run_nnet3_multilingual.sh "libri-org libri-mono libri-half" "tri mono tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-half libri-3Quarters" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-mono" "tri tri mono" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree"
     ./run_nnet3_multilingual.sh "libri-org libri-3Quarters libri-half libri-mono" "tri tri tri mono" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${run}" "tree tree tree tree"

done

>>>>>>> 1bdf618b39b818781199c2b17f3db1e905f76c13
