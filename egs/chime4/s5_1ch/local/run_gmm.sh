#!/bin/bash

# Copyright 2016 University of Sheffield (Jon Barker, Ricard Marxer)
#                Inria (Emmanuel Vincent)
#                Mitsubishi Electric Research Labs (Shinji Watanabe)
#  Apache 2.0  (http://www.apache.org/licenses/LICENSE-2.0)

# This script is made from the kaldi recipe of the 2nd CHiME Challenge Track 2
# made by Chao Weng

. ./path.sh
. ./cmd.sh ## You'll want to change cmd.sh to something that will work on your system.
           ## This relates to the queue.

# Config:
nj=30
stage=0 # resume training with --stage=N
train=noisy # noisy data multi-condition training
eval_flag=true # make it true when the evaluation data are released

. utils/parse_options.sh || exit 1;

# This is a shell script, but it's recommended that you run the commands one by
# one by copying and pasting into the shell.

if [ $# -ne 3 ]; then
  printf "\nUSAGE: %s <enhancement method> <enhanced speech directory> <chime4 root directory>\n\n" `basename $0`
  echo "First argument specifies a unique name for different enhancement method"
  echo "Second argument specifies the directory of enhanced wav files"
  echo "Third argument specifies the CHiME4 root directory"
  exit 1;
fi

# set enhanced data
enhan=$1
enhan_data=$2
# set chime4 data
chime4_data=$3

# Set bash to 'debug' mode, it will exit on :
# -e 'error', -u 'undefined variable', -o ... 'error in pipeline', -x 'print commands',
set -e
set -u
set -o pipefail

# check whether run_init is executed
if [ ! -d data/lang ]; then
  echo "error, execute local/run_init.sh, first"
  exit 1;
fi

#######################
#### training #########
if [ $stage -le 1 ]; then
  # process for distant talking speech for real and simulation data
  local/real_noisy_chime4_data_prep.sh $chime4_data
  local/simu_noisy_chime4_data_prep.sh $chime4_data
fi

# Now make MFCC features for clean, close, and noisy data
# mfccdir should be some place with a largish disk where you
# want to store MFCC features.
mfccdir=mfcc
if [ $stage -le 2 ]; then
  if $eval_flag; then
    tasks="tr05_real_${train} dt05_real_${train} tr05_simu_${train} dt05_simu_${train} et05_real_${train} et05_simu_${train}"
  else
    tasks="tr05_real_${train} dt05_real_${train} tr05_simu_${train} dt05_simu_${train}"
  fi
  for x in $tasks; do
    steps/make_mfcc.sh --nj 8 --cmd "$train_cmd" \
      data/$x exp/make_mfcc/$x $mfccdir
    steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir
  done
fi

# make mixed training set from real and simulation training data
# multi = simu + real
if [ $stage -le 3 ]; then
  utils/combine_data.sh data/tr05_multi_${train} data/tr05_simu_${train} data/tr05_real_${train}
  utils/combine_data.sh data/dt05_multi_${train} data/dt05_simu_${train} data/dt05_real_${train}
  if $eval_flag; then
  utils/combine_data.sh data/et05_multi_${train} data/et05_simu_${train} data/et05_real_${train}
  fi
fi

# training models for noisy data
if [ $stage -le 4 ]; then
  nspk=`wc -l data/tr05_multi_${train}/spk2utt | awk '{print $1}'`
  if [ $nj -gt $nspk ]; then
    nj2=$nspk
  else
    nj2=$nj
  fi
  # training monophone model
  steps/train_mono.sh --boost-silence 1.25 --nj $nj2 --cmd "$train_cmd" \
    data/tr05_multi_${train} data/lang exp/mono0a_tr05_multi_${train}
  steps/align_si.sh --boost-silence 1.25 --nj $nj2 --cmd "$train_cmd" \
    data/tr05_multi_${train} data/lang exp/mono0a_tr05_multi_${train} exp/mono0a_ali_tr05_multi_${train}

  # training triphone model with lda mllt features
  steps/train_deltas.sh --boost-silence 1.25 --cmd "$train_cmd" \
    2000 10000 data/tr05_multi_${train} data/lang exp/mono0a_ali_tr05_multi_${train} exp/tri1_tr05_multi_${train}
  steps/align_si.sh --nj $nj2 --cmd "$train_cmd" \
    data/tr05_multi_${train} data/lang exp/tri1_tr05_multi_${train} exp/tri1_ali_tr05_multi_${train}

  steps/train_lda_mllt.sh --cmd "$train_cmd" \
    --splice-opts "--left-context=3 --right-context=3" \
    2500 15000 data/tr05_multi_${train} data/lang exp/tri1_ali_tr05_multi_${train} exp/tri2b_tr05_multi_${train}
  steps/align_si.sh  --nj $nj2 --cmd "$train_cmd" \
    --use-graphs true data/tr05_multi_${train} data/lang exp/tri2b_tr05_multi_${train} exp/tri2b_ali_tr05_multi_${train}

  steps/train_sat.sh --cmd "$train_cmd" \
    2500 15000 data/tr05_multi_${train} data/lang exp/tri2b_ali_tr05_multi_${train} exp/tri3b_tr05_multi_${train}
  utils/mkgraph.sh data/lang_test_tgpr_5k exp/tri3b_tr05_multi_${train} exp/tri3b_tr05_multi_${train}/graph_tgpr_5k
fi
#### training done ####
#######################


#####################
#### tsting #########
# process for enhanced data
if [ $stage -le 5 ]; then
  if [ ! -d data/dt05_real_$enhan ] || [ ! -d data/et05_real_$enhan ]; then
    local/real_enhan_chime4_data_prep.sh $enhan $enhan_data
    local/simu_enhan_chime4_data_prep.sh $enhan $enhan_data
  fi
fi

# Now make MFCC features for enhanced data
# mfccdir should be some place with a largish disk where you
# want to store MFCC features.
mfccdir=mfcc/$enhan
if [ $stage -le 6 ]; then
  if $eval_flag; then
    tasks="dt05_real_$enhan dt05_simu_$enhan et05_real_$enhan et05_simu_$enhan"
  else
    tasks="dt05_real_$enhan dt05_simu_$enhan"
  fi
  for x in $tasks; do
    if [ ! -e data/$x/feats.scp ]; then
      steps/make_mfcc.sh --nj 8 --cmd "$train_cmd" \
	data/$x exp/make_mfcc/$x $mfccdir
      steps/compute_cmvn_stats.sh data/$x exp/make_mfcc/$x $mfccdir
    fi
  done
fi

# make mixed training set from real and simulation enhanced data
# multi = simu + real
if [ $stage -le 7 ]; then
  if [ ! -d data/dt05_multi_$enhan ]; then
    utils/combine_data.sh data/dt05_multi_$enhan data/dt05_simu_$enhan data/dt05_real_$enhan
    if $eval_flag; then
    utils/combine_data.sh data/et05_multi_$enhan data/et05_simu_$enhan data/et05_real_$enhan
    fi
  fi
fi

# decode enhanced speech using AMs trained with enhanced data
if [ $stage -le 8 ]; then
  steps/decode_fmllr.sh --nj 4 --num-threads 3 --cmd "$decode_cmd" \
    exp/tri3b_tr05_multi_${train}/graph_tgpr_5k data/dt05_real_$enhan exp/tri3b_tr05_multi_${train}/decode_tgpr_5k_dt05_real_$enhan &
  steps/decode_fmllr.sh --nj 4 --num-threads 3 --cmd "$decode_cmd" \
    exp/tri3b_tr05_multi_${train}/graph_tgpr_5k data/dt05_simu_$enhan exp/tri3b_tr05_multi_${train}/decode_tgpr_5k_dt05_simu_$enhan &
  if $eval_flag; then
  steps/decode_fmllr.sh --nj 4 --num-threads 3 --cmd "$decode_cmd" \
    exp/tri3b_tr05_multi_${train}/graph_tgpr_5k data/et05_real_$enhan exp/tri3b_tr05_multi_${train}/decode_tgpr_5k_et05_real_$enhan &
  steps/decode_fmllr.sh --nj 4 --num-threads 3 --cmd "$decode_cmd" \
    exp/tri3b_tr05_multi_${train}/graph_tgpr_5k data/et05_simu_$enhan exp/tri3b_tr05_multi_${train}/decode_tgpr_5k_et05_simu_$enhan &
  fi
  wait;
fi

# scoring
if [ $stage -le 9 ]; then
  # decoded results of enhanced speech using AMs trained with enhanced data
  local/chime4_calc_wers.sh exp/tri3b_tr05_multi_${train} $enhan exp/tri3b_tr05_multi_${train}/graph_tgpr_5k \
    > exp/tri3b_tr05_multi_${train}/best_wer_$enhan.result
  head -n 15 exp/tri3b_tr05_multi_${train}/best_wer_$enhan.result
fi
#### tsting done ####
#####################

echo "`basename $0` Done."
