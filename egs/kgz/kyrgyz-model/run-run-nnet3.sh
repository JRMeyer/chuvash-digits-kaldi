#!/bin/bash



for run in {1..20}
do
    ./run_nnet3_multilingual.sh "orgWiki org2" "tri tri" "0.5,0.5" "${run}_2gram"
  
    ./run_nnet3_multilingual.sh "orgWiki org2" "tri mono" "0.5,0.5" "${run}_2gram"
  
    ./run_nnet3_multilingual.sh "orgWiki manner" "tri tri" "0.5,0.5" "${run}_2gram"
  
    ./run_nnet3_multilingual.sh "orgWiki manner" "tri mono" "0.5,0.5" "${run}_2gram"
  
    ./run_nnet3_multilingual.sh "orgWiki no_voice" "tri tri" "0.5,0.5" "${run}_2gram"
  
    ./run_nnet3_multilingual.sh "orgWiki no_voice" "tri mono" "0.5,0.5" "${run}_2gram"

done
