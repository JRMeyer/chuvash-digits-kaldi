#!/bin/bash

# extract feats for some dir



cmp=utils/run.pl
data_dir=new_data
plp_dir=${data_dir}/plp


echo "MAKE PLPs: $dir"

steps/make_plp.sh \
    --cmd $cmd \
    --nj `nproc` \
    ${data_dir} \
    ${data_dir}/make_plp_log \
    ${plp_dir}

utils/fix_data_dir.sh ${data_dir}

steps/compute_cmvn_stats.sh \
    ${data_dir} \
    ${data_dir}/make_plp_log \
    ${plp_dir}

utils/fix_data_dir.sh ${data_dir}
