#!/bin/bash

# extract feats for some dir

# INPUT:

cmd=utils/run.pl
num_jobs=`nproc`

data_dir=$1
feat_dir=$2



if [ "$#" -ne 2 ]; then
    echo "ERROR: $0"
    echo ""
    echo "USAGE: $0 <data_dir> <feat_dir>"
    echo "     : $0 data_org/train data_org/plp"
    exit 1
fi



echo "MAKE PLPs:"
echo "nj=$num_jobs"
echo "Source data dir: $data_dir"
echo "Destination feats dir: $feat_dir"
echo "Log dir: $data_dir/make_plp_log"


for i in spk2utt text utt2spk wav.scp; do
    LC_ALL=C sort -i $data_dir/$i -o $data_dir/$i;
done


steps/make_plp.sh \
    --cmd $cmd \
    --nj $num_jobs \
    $data_dir \
    $feat_dir/make_plp_log \
    $feat_dir \
    || printf "\n####\n#### ERROR: make_plp.sh\n####\n\n" \
    || exit 1;

utils/fix_data_dir.sh $data_dir

steps/compute_cmvn_stats.sh \
    $data_dir \
    $feat_dir/make_plp_log \
    $feat_dir \
    || printf "\n####\n#### ERROR: compute_cmvn_stats.sh\n####\n\n" \
    || exit 1;

utils/fix_data_dir.sh $data_dir




# printf "#### MFCCs ####\n";

#     steps/make_mfcc.sh \
#         --cmd $cmd \
#         --nj $num_processors \
#         --mfcc-config ${config_dir}/mfcc.conf \
#         ${data_dir}/${dir} \
#         ${exp_dir}/make_mfcc_log/${dir} \
#         $feat_dir \
#         || printf "\n####\n#### ERROR: make_mfcc.sh \n####\n\n" \
#         || exit 1;

#     steps/compute_cmvn_stats.sh \
#         ${data_dir}/${dir} \
#         ${exp_dir}/make_mfcc_log/${dir} \
#         $feat_dir \
#         || printf "\n####\n#### ERROR: compute_cmvn_stats.sh \n####\n\n" \
#         || exit 1;
