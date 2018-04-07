#!/bin/bash


dim=500
num_epochs=5


./run_nnet3_multilingual.sh "chv-digits" "tri" "1.0" $dim $num_epochs "2gramWiki_${num_epochs}epoch_5layer_${dim}dim" "tree" "0"


exit
