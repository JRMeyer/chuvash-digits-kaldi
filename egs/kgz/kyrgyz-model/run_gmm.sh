#!/bin/bash

# Joshua Meyer (2017)


# USAGE:
#
#      ./run.sh <corpus_name>
#
# INPUT:
#
#    input_dir/
#       lexicon.txt
#       lexicon_nosil.txt
#       phones.txt
#       task.arpabo
#       transcripts
#
#       audio_dir/
#          utterance1.wav
#          utterance2.wav
#          utterance3.wav
#               .
#               .
#          utteranceN.wav
#
#    config_dir/
#       mfcc.conf
#       topo_orig.proto
#
#
# OUTPUT:
#
#    exp_dir
#    feat_dir
#    data_dir
# 


corpus_name=$1

if [ "$#" -ne 1 ]; then
    echo "ERROR: $0"
    echo "USAGE: $0 <corpus_name>"
    exit 1
fi


### STAGES
##
#
prep_data=1
extract_feats=0
train_monophones=0
train_triphones=0
adapt_models=0
compile_graph=0
decode_test=0
#
##
###


### HYPER-PARAMETERS
##
#
train_percentage=.9
tot_gauss_mono=500
num_leaves_tri=500
tot_gauss_tri=1000
decode_beam=13
decode_lattice_beam=7
decode_max_active_states=700
num_iters_mono=10
num_iters_tri=10
#
##
###


### SHOULD ALREADY EXIST
##
#
num_processors=$(nproc)
unknown_word="<unk>"
unknown_phone="SPOKEN_NOISE"
silence_phone="SIL"
input_dir=input_${corpus_name}
audio_dir="${input_dir}/audio"
config_dir=config
cmd="utils/run.pl"
#
##
###


### GENERATED BY SCRIPT
##
#
data_dir=data_${corpus_name}
exp_dir=exp_${corpus_name}
# mfcc_dir=mfcc_${corpus_name}
plp_dir=plp_${corpus_name}
train_dir=train
test_dir=test
#
##
###



if [ "$prep_data" -eq "1" ]; then
    
    printf "\n####=================####\n";
    printf "#### BEGIN DATA PREP ####\n";
    printf "####=================####\n\n";

    echo "$0: looking for audio data in $audio_dir"
    
    # Make sure we have the audio data (WAV file utterances)
    if [ ! -d $audio_dir ]; then
        printf '\n####\n#### ERROR: '"${audio_dir}"' not found \n####\n\n';
        exit 1;
    fi

    # sort input files by bytes (C-style) and re-save them with orginal filename
    for fileName in \
        lexicon.txt \
            lexicon_nosil.txt \
            phones.txt \
            transcripts; do
        LC_ALL=C sort -i ${input_dir}/${fileName} -o ${input_dir}/${fileName};
    done;

    # Given dir of WAV files, create dirs for train and test, create 'wav.scp',
    # create 'text', create 'utt2spk' and 'spk2utt', and copy the language model
    # from elsewhere (ARPA format)

    local/prepare_data.sh \
        $audio_dir \
        $input_dir \
        $train_percentage \
        $data_dir \
        $train_dir \
        $test_dir \
        || printf "\n####\n#### ERROR: prepare_data.sh \n####\n\n" \
        || exit 1;

    # Copy and paste existing phonetic dictionary, language model, and phone list

    local/prepare_dict.sh \
        $data_dir \
        $input_dir \
        $silence_phone \
        || printf "\n####\n#### ERROR: prepare_dict.sh\n####\n\n" \
        || exit 1;

    # Prepare a dir such as data/lang/
    # This script can add word-position-dependent phones, and constructs a host of
    # other derived files, that go in data/lang/.
    # This creates the FST for the lexicon.

    local/prepare_lang.sh \
        --position-dependent-phones false \
        ${data_dir}/local/dict \
        ${data_dir}/local/lang \
        ${data_dir}/lang \
        $unknown_word \
        || printf "\n####\n#### ERROR: prepare_lang.sh\n####\n\n" \
        || exit 1;

    # Create the FST (G.fst) for the grammar

    local/prepare_lm.sh \
        $data_dir \
        || printf "\n####\n#### ERROR: prepare_lm.sh\n####\n\n" \
        || exit 1;

    printf "\n####===============####\n";
    printf "#### END DATA PREP ####\n";
    printf "####===============####\n\n";

fi



if [ "$extract_feats" -eq "1" ]; then

    printf "\n####==========================####\n";
    printf "#### BEGIN FEATURE EXTRACTION ####\n";
    printf "####==========================####\n\n";

    # printf "#### MFCCs ####\n";

    # for dir in $train_dir $test_dir; do

    #     steps/make_mfcc.sh \
    #         --cmd $cmd \
    #         --nj $num_processors \
    #         --mfcc-config ${config_dir}/mfcc.conf \
    #         ${data_dir}/${dir} \
    #         ${exp_dir}/make_mfcc_log/${dir} \
    #         $mfcc_dir \
    #         || printf "\n####\n#### ERROR: make_mfcc.sh \n####\n\n" \
    #         || exit 1;

    #     steps/compute_cmvn_stats.sh \
    #         ${data_dir}/${dir} \
    #         ${exp_dir}/make_mfcc_log/${dir} \
    #         $mfcc_dir \
    #         || printf "\n####\n#### ERROR: compute_cmvn_stats.sh \n####\n\n" \
    #         || exit 1;
    # done

    printf "#### PLPs ####\n";

    for dir in $train_dir $test_dir; do

        echo "MAKE PLPs: $dir"
        
        steps/make_plp.sh \
            --cmd $cmd \
            --nj $num_processors \
            ${data_dir}/${dir} \
            ${exp_dir}/make_plp_log/${dir} \
            $plp_dir
        
        utils/fix_data_dir.sh ${data_dir}/${dir}
        
        steps/compute_cmvn_stats.sh \
            ${data_dir}/${dir} \
            ${exp_dir}/make_plp_log/${dir} \
            $plp_dir
        
        utils/fix_data_dir.sh ${data_dir}/${dir}
    done
    
    printf "\n####========================####\n";
    printf "#### END FEATURE EXTRACTION ####\n";
    printf "####========================####\n\n";

fi



if [ "$train_monophones" -eq "1" ]; then

    printf "\n####===========================####\n";
    printf "#### BEGIN TRAINING MONOPHONES ####\n";
    printf "####===========================####\n\n";

    printf "#### Train Monophones ####\n";

    steps/train_mono.sh \
        --cmd "$cmd" \
        --nj $num_processors \
        --num-iters $num_iters_mono \
        --totgauss $tot_gauss_mono \
        --beam 6 \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/monophones \
        || printf "\n####\n#### ERROR: train_mono.sh \n####\n\n" \
        || exit 1;

    ../../../src/gmmbin/gmm-info ${exp_dir}/monophones/final.mdl


    printf "#### Align Monophones ####\n";

    steps/align_si.sh \
        --cmd "$cmd" \
        --nj $num_processors \
        --boost-silence 1.25 \
        --beam 10 \
        --retry-beam 40 \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/monophones \
        ${exp_dir}/monophones_aligned \
        || printf "\n####\n#### ERROR: align_si.sh \n####\n\n" \
        || exit 1;

    
    printf "\n####===========================####\n";
    printf "#### END TRAINING MONOPHONES ####\n";
    printf "####===========================####\n\n";

fi



if [ "$train_triphones" -eq "1" ]; then

    printf "\n####==========================####\n";
    printf "#### BEGIN TRAINING TRIPHONES ####\n";
    printf "####==========================####\n\n";


    printf "### Train Triphones ###\n"

    steps/train_deltas.sh \
        --cmd "$cmd" \
        --num-iters $num_iters_tri \
        --beam 10 \
        $num_leaves_tri \
        $tot_gauss_tri \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/monophones_aligned \
        ${exp_dir}/triphones \
        || printf "\n####\n#### ERROR: train_deltas.sh \n####\n\n" \
        || exit 1;

    ../../../src/gmmbin/gmm-info ${exp_dir}/triphones/final.mdl


    printf "### Align Triphones ###\n"

    steps/align_si.sh \
        --cmd "$cmd" \
        --nj $num_processors \
        --boost-silence 1.25 \
        --beam 10 \
        --retry-beam 40 \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/triphones \
        ${exp_dir}/triphones_aligned \
        || printf "\n####\n#### ERROR: align_si.sh \n####\n\n" \
        || exit 1;

    
    printf "\n####========================####\n";
    printf "#### END TRAINING TRIPHONES ####\n";
    printf "####========================####\n\n";

fi



if [ "$adapt_models" -eq "1" ]; then
    
    printf "\n####==========================####\n";
    printf "#### BEGIN SPEAKER ADAPTATION ####\n";
    printf "####==========================####\n\n";

    printf "### Begin LDA + MLLT Triphones ###\n"

    steps/train_lda_mllt.sh \
        --cmd "$cmd" \
        --splice-opts "--left-context=3 --right-context=3" \
        $num_leaves_tri \
        $tot_gauss_tri \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/triphones_aligned \
        ${exp_dir}/triphones_lda_mllt \
        || printf "\n####\n#### ERROR: train_lda_mllt.sh \n####\n\n" \
        || exit 1;

    ../../../src/gmmbin/gmm-info ${exp_dir}/triphones_lda_mllt/final.mdl


    printf "### Align LDA + MLLT Triphones ###\n"

    steps/align_si.sh \
        --cmd "$cmd" \
        --nj $num_processors \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/triphones_lda_mllt \
        ${exp_dir}/triphones_lda_mllt_aligned \
        || printf "\n####\n#### ERROR: align_si.sh \n####\n\n" \
        || exit 1;



    printf "\n####===========================####\n";
    printf "#### BEGIN TRAINING SAT (fMLLR) ####\n";
    printf "####============================####\n\n";


    printf "### Train LDA + MLLT + SAT Triphones ###\n"

    steps/train_sat.sh \
        --cmd "$cmd" \
        $num_leaves_tri \
        $tot_gauss_tri \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/triphones_lda_mllt_aligned \
        ${exp_dir}/triphones_lda_mllt_sat \
        || printf "\n####\n#### ERROR: train_sat.sh \n####\n\n" \
        || exit 1;

    ../../../src/gmmbin/gmm-info ${exp_dir}/triphones_lda_mllt_sat/final.mdl


    printf "### Align LDA + MLLT + SAT Triphones ###\n"

    steps/align_fmllr.sh \
        --cmd "$cmd" \
        --nj $num_processors \
        ${data_dir}/${train_dir} \
        ${data_dir}/lang \
        ${exp_dir}/triphones_lda_mllt_sat \
        ${exp_dir}/triphones_lda_mllt_sat_aligned \
        || printf "\n####\n#### ERROR: align_si.sh \n####\n\n" \
        || exit 1;
fi



if [ "$compile_graph" -eq "1" ]; then
    
    printf "\n####=========================####\n";
    printf "#### BEGIN GRAPH COMPILATION ####\n";
    printf "####=========================####\n\n";
    
    # Graph compilation
    
    # This script creates a fully expanded decoding graph (HCLG) that represents
    # the language-model, pronunciation dictionary (lexicon), context-dependency,
    # and HMM structure in our model.  The output is a Finite State Transducer
    # that has word-ids on the output, and pdf-ids on the input (these are indexes
    # that resolve to Gaussian Mixture Models).
    
    # echo "### Compile monophone graph in ${exp_dir}/monophones/graph"
    
    # utils/mkgraph.sh \
    #     --mono \
    #     ${data_dir}/lang_test \
    #     ${exp_dir}/monophones \
    #     ${exp_dir}/monophones/graph \
    #     || printf "\n####\n#### ERROR: mkgraph.sh \n####\n\n" \
    #     || exit 1;
        
    echo "### Compile triphone graph in ${exp_dir}/triphones/graph"
    
    utils/mkgraph.sh \
        ${data_dir}/lang_test \
        ${exp_dir}/triphones \
        ${exp_dir}/triphones/graph \
        || printf "\n####\n#### ERROR: mkgraph.sh \n####\n\n" \
        || exit 1;
    
    printf "\n####=======================####\n";
    printf "#### END GRAPH COMPILATION ####\n";
    printf "####=======================####\n\n";

fi



if [ "$decode_test" -eq "1" ]; then
    
    printf "\n####================####\n";
    printf "#### BEGIN DECODING ####\n";
    printf "####================####\n\n";
    
    # DECODE WITH TRIPHONES WITH SAT ADJUSTED FEATURES
    
    # steps/decode_fmllr.sh --cmd "$cmd" \
    #     --nj $num_processors \
    #     ${exp_dir}/triphones_lda_mllt_sat/graph \
    #     ${data_dir}/${test_dir} \
    #     "${exp_dir}"'/triphones_lda_mllt_sat/decode_'"${test_dir}" \
    #     $unknown_phone \
    #     $silence_phone \
    #     || exit 1;

    
    # DECODE WITH REGULAR TRIPHONES WITH VANILLA DELTA FEATURES

    printf "\n ### Decoding with only 1 job, 4 jobs crashes:/ ### "
    
    steps/decode.sh \
        --cmd "$cmd" \
        --nj 1 \
        --beam $decode_beam \
        --lattice-beam $decode_lattice_beam \
        --max-active $decode_max_active_states \
        ${exp_dir}/triphones/graph \
        ${data_dir}/${test_dir} \
        "${exp_dir}"'/triphones/decode_'"${test_dir}" \
        $unknown_phone \
        $silence_phone \
        || printf "\n####\n#### ERROR: decode.sh \n####\n\n" \
        || exit 1;
    

    printf "#### BEGIN CALCULATE WER ####\n";
    
    for x in ${exp_dir}/triphones/decode*; do
        [ -d $x ] && grep WER $x/wer_* | utils/best_wer.sh > WER_triphones_${corpus_name}.txt;
    done

    printf "\n####==============####\n";
    printf "#### END DECODING ####\n";
    printf "####==============####\n\n";

fi



exit;


