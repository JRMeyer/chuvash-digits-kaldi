#!/bin/bash


gmm=$1
dim=500
num_epochs=10



for dnn in {1..5}
do


# ### A + B ###
# ## 2 baselines 
./run_nnet3_multilingual.sh "atai atai" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"

rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3


./run_nnet3_multilingual.sh "atai atai atai" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"

rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3


./run_nnet3_multilingual.sh "atai libri-boot1" "tri tri" "1.0,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"

rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

./run_nnet3_multilingual.sh "atai libri-boot2" "tri mono" "1.0,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

./run_nnet3_multilingual.sh "atai libri-boot2 libri-boot1" "tri mono tri" "1.0,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"
rm -rf /data/MTL/exp/nnet3
rm -rf /data/MTL/exp/atai/nnet3
rm -rf /data/MTL/exp/libri-boot1/nnet3
rm -rf /data/MTL/exp/libri-boot2/nnet3

done

exit



# ./run_nnet3_multilingual.sh "libri-boot1 libri-boot12 libri-boot15 libri-boot18 libri-boot20 libri-boot23 libri-boot26 libri-boot29 libri-boot4 libri-boot7 libri-boot10 libri-boot13 libri-boot16 libri-boot19 libri-boot21 libri-boot24 libri-boot27 libri-boot3 libri-boot5 libri-boot8 libri-boot11 libri-boot14 libri-boot17 libri-boot2 libri-boot22 libri-boot25 libri-boot28 libri-boot30 libri-boot6 libri-boot9" "tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri tri" "0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree tree" "0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5"









    

################
### BASELINE ###
################


# ### A + B ###
# ## 2 baselines 
# ./run_nnet3_multilingual.sh "libri-org libri-org" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree" "0"

# ### A + B + C ###
# ## 3 baselines 
# ./run_nnet3_multilingual.sh "libri-org libri-org libri-org" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" "0"

# ### A + B + C + D ###
# ## 4 baselines
# ./run_nnet3_multilingual.sh "libri-org libri-org libri-org libri-org" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" "0"

for dnn in {1..5}
do


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
    ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter" "tri tri" "0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}_COPIES" "tree tree" ".1"

    # # ### A + B + C ###
    # # ## 25per + 50per
    # ./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter libri-boot2Quarter" "tri tri tri" "0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree" ".25,.25"

    # # ### A + B + C + D ###
    # # ## 25per + 50per + 75per
    #./run_nnet3_multilingual.sh "libri-org libri-boot1Quarter libri-boot2Quarter libri-boot3Quarter" "tri tri tri tri" "0.5,0.5,0.5,0.5" $dim $num_epochs "2gram_${num_epochs}epoch_5layer_${dim}dim_${gmm}_${dnn}" "tree tree tree tree" ".25,.25,.25"
    
    
done

