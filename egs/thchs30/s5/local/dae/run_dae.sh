#!/bin/bash

#dnn dae training

dwntest=false
stdtest=false
stage=0
nj=8

. ./cmd.sh ## You'll want to change cmd.sh to something that will work on your system.
           ## This relates to the queue.

. ./path.sh ## Source the tools/utils (import the queue.pl)
. utils/parse_options.sh || exit 1;

thchs=$1

#generate noisy data. We focuse on the 0db condition.
#For training set, generate noisy data with SNR mean=0, variance=10, with three noise types mixed together.  
#For dev, generate noisy data with SNR mean=0, variance=0, with three niose types mixed together
#For test, either use the standard test set (stdtest=true) or randomly generated data (stdtest=false)

if [ $stage =  0 ]; then
   #generat noise.scp
   mkdir -p data/dae/noise && \
   awk '{print $1 " '$thchs'/resource/noise/"$2}' $thchs/resource/noise/noise.scp >  data/dae/noise/noise.scp || exit 1

   echo "DAE: generate training data..."
   noise_scp=data/dae/noise/noise.scp
   noise_prior="0.0,10.0,10.0,10.0" #define noise type to sample. [S_clean, S_white, S_car, S_cafe]
   noise_level=0 #0db condition
   sigma0=10 #some random in SNR
   seed=32
   verbose=0
   wavdir=wav/dae/train 

   rm -rf data/dae/train && mkdir -p data/dae/train || exit 1
   cp data/fbank/train/{spk2utt,utt2spk,text} data/dae/train || exit 1
   mkdir -p $wavdir && awk '{print $1 " '$wavdir'/"$1".wav"}' data/fbank/train/wav.scp > data/dae/train/wav.scp || exit 1

   mkdir -p exp/dae/gendata 
   split_scps=""
   for n in $(seq $nj); do
      split_scps="$split_scps exp/dae/gendata/train_split_${n}.scp"
   done
   utils/split_scp.pl data/fbank/train/wav.scp  $split_scps || exit 1
   $train_cmd JOB=1:$nj exp/dae/gendata/add_noise_train.JOB.log \
     local/dae/add-noise-mod.py --noise-level $noise_level \
       --sigma0 $sigma0 --seed $seed --verbose $verbose \
       --noise-prior $noise_prior --noise-src $noise_scp \
       --wav-src exp/dae/gendata/train_split_JOB.scp --wavdir $wavdir \
       || exit 1

   steps/make_fbank.sh --nj $nj --cmd "$train_cmd"  \
     data/dae/train exp/dae/gendata fbank/dae/train || exit 1
   steps/compute_cmvn_stats.sh data/dae/train exp/dae/cmvn \
     fbank/dae/train/_cmvn || exit 1

   #genreate dev data. Just the 0db condition is produced.  Multiple noise types mixed together.
   echo "DAE: generating dev data..."
   wavdir=wav/dae/dev/0db
   sigma0=0 #no random in SNR
   rm -rf data/dae/dev/0db && mkdir -p data/dae/dev/0db && \
   cp -L data/fbank/dev/{spk2utt,utt2spk,text} data/dae/dev/0db || exit 1
   mkdir -p $wavdir && awk '{print $1 " '$wavdir'/"$1".wav"}' data/fbank/dev/wav.scp > data/dae/dev/0db/wav.scp || exit 1

   split_scps=""
   for n in $(seq $nj); do
      split_scps="$split_scps exp/dae/gendata/dev_split_${n}.scp"
   done
   utils/split_scp.pl data/fbank/dev/wav.scp  $split_scps || exit 1

   $train_cmd JOB=1:$nj exp/dae/gendata/add_noise_dev.JOB.log \
     local/dae/add-noise-mod.py --noise-level $noise_level \
       --sigma0 $sigma0 --seed $seed --verbose $verbose \
       --noise-prior $noise_prior --noise-src $noise_scp \
       --wav-src exp/dae/gendata/dev_split_JOB.scp --wavdir $wavdir \
       || exit 1
   steps/make_fbank.sh --nj $nj --cmd "$train_cmd"  \
     data/dae/dev/0db exp/dae/gendata fbank/dae/dev/0db || exit 1
   steps/compute_cmvn_stats.sh data/dae/dev/0db exp/dae/cmvn \
     fbank/dae/dev/0db/_cmvn || exit 1

   #generate test data. Note that if you want to compare with the standard results, set stdtest=true
   echo "DAE: generating test data..."
   if [ $stdtest = true ]; then
     #download noisy wav if use the standard test data
     echo "using standard test data"
     if [ $dwntest = true ];then
       echo "downloading the noisy test data from openslr..."
       (
        mkdir -p wav && cd wav && \
        wget http://www.openslr.org/resources/18/test-noise.tgz || exit 1
        tar xvf test-noise.tgz || exit 1
       )
     fi
     #generate fbank
     for x in car white cafe; do
       echo "producing fbanks for $x"
       mkdir -p data/dae/test/0db/$x && \
       cp -L data/fbank/test/{spk2utt,utt2spk,text} data/dae/test/0db/$x && \
       awk '{print $1 " wav/test-noise/0db/'$x'/"$1".wav"}' data/fbank/test/wav.scp > data/dae/test/0db/$x/wav.scp || exit 1
       steps/make_fbank.sh --nj $nj --cmd "$train_cmd"  \
         data/dae/test/0db/$x exp/dae/gendata fbank/dae/test/0db/$x || exit 1
     done

   else
     #generate test data randomly
     sigma0=0 #no random in SNR
     echo "generating noisy test data randomly"
     for x in car white cafe; do
       echo "generating noisy wav for $x"

       case $x in
         car)
            noise_prior="0.0,0.0,10.0,0.0" 
            ;;
         white)
            noise_prior="0.0,10.0,0.0,0.0" 
            ;;
         cafe)
            noise_prior="0.0,0.0,0.0,10.0" 
            ;;
       esac

       wavdir=wav/test-noise/0db/$x 
       rm -rf data/dae/test/0db/$x && mkdir -p data/dae/test/0db/$x && \
       cp -L data/fbank/test/{spk2utt,utt2spk,text} data/dae/test/0db/$x || exit 1
       mkdir -p $wavdir && awk '{print $1 " '$wavdir'/"$1".wav"}' data/fbank/test/wav.scp > data/dae/test/0db/$x/wav.scp || exit 1

       split_scps=""
       for n in $(seq $nj); do
         split_scps="$split_scps exp/dae/gendata/test_split_${n}.scp"
       done
       utils/split_scp.pl data/fbank/test/wav.scp  $split_scps || exit 1


       $train_cmd JOB=1:$nj exp/dae/gendata/add_noise_dev.JOB.log \
         local/dae/add-noise-mod.py --noise-level $noise_level \
           --sigma0 $sigma0 --seed $seed --verbose $verbose \
           --noise-prior $noise_prior --noise-src $noise_scp \
           --wav-src exp/dae/gendata/test_split_JOB.scp --wavdir $wavdir \
           || exit 1

       echo "producing fbanks for test data $x"
       steps/make_fbank.sh --nj $nj --cmd "$train_cmd"  \
         data/dae/test/0db/$x exp/dae/gendata fbank/dae/test/0db/$x || exit 1

     done
   fi
   for x in car white cafe; do
     echo "generating cmvn for test data $x"
     steps/compute_cmvn_stats.sh data/dae/test/0db/$x exp/dae/cmvn \
       fbank/dae/test/0db/$x/_cmvn || exit 1
     cp -R data/dae/test/0db/$x data/dae/test/0db/${x}_phone && cp data/test/phone.txt data/dae/test/0db/${x}_phone/text || exit 1
   done
fi

#DAE training
if [ $stage -le 1 ]; then
  #train dnn dae using data with mixed noise
  #produce merged feats.scp as --labels for both training and cv
  dir=exp/tri4b_dnn_dae && mkdir -p exp/tri4b_dnn_dae || exit 1
  cat data/fbank/train/feats.scp data/fbank/dev/feats.scp | sort -u  > $dir/tgt_feats.scp
  cat data/fbank/train/cmvn.scp data/fbank/dev/cmvn.scp   | sort -u  > $dir/tgt_cmvn.scp

  num_fea=$(feat-to-dim scp:$dir/tgt_feats.scp -)
  echo "num_fea = $num_fea"

  $cuda_cmd exp/tri4b_dnn_dae/log/train_nnet.log \
    steps/nnet/train.sh --hid-layers 2 --hid-dim 1200 \
      --cmvn-opts "--norm-vars=false"  --splice 10 \
      --learn-rate 0.0001 \
      --train_tool_opts "--objective-function=mse" \
      --copy_feats false \
      --labels "ark:copy-feats scp:$dir/tgt_feats.scp ark:- | apply-cmvn --norm-vars=false scp:$dir/tgt_cmvn.scp ark:- ark:- | feat-to-post ark:- ark:-|" \
      --num-tgt  $num_fea \
      --proto-opts '--no-softmax '  \
      data/dae/train data/dae/dev/0db data/lang  \
      data/fbank/train  data/fbank/dev  \
      exp/tri4b_dnn_dae || exit 1;
  nnet-concat exp/tri4b_dnn_dae/final.feature_transform exp/tri4b_dnn_dae/final.nnet \
    exp/tri4b_dnn_mpe/final.feature_transform exp/tri4b_dnn_dae/dae.nnet  || exit 1
  
fi

#decoding 
if [ $stage -le 2 ]; then
   for x in car white cafe; do
     (
       #decode word 
       steps/nnet/decode.sh --cmd "$decode_cmd" --nj $nj \
         --srcdir exp/tri4b_dnn_mpe \
         exp/tri4b/graph_word data/dae/test/0db/$x exp/tri4b_dnn_mpe/decode_word_0db/$x || exit 1;
       steps/nnet/decode.sh --cmd "$decode_cmd" --nj $nj \
         --srcdir exp/tri4b_dnn_mpe --feature-transform exp/tri4b_dnn_dae/dae.nnet \
         exp/tri4b/graph_word data/dae/test/0db/$x exp/tri4b_dnn_dae/decode_word_0db/$x || exit 1;

       #decode phone
       steps/nnet/decode.sh --cmd "$decode_cmd" --nj $nj \
         --srcdir exp/tri4b_dnn_mpe \
         exp/tri4b/graph_phone data/dae/test/0db/${x}_phone exp/tri4b_dnn_mpe/decode_phone_0db/$x || exit 1;
       steps/nnet/decode.sh --cmd "$decode_cmd" --nj $nj \
         --srcdir exp/tri4b_dnn_mpe --feature-transform exp/tri4b_dnn_dae/dae.nnet \
         exp/tri4b/graph_phone data/dae/test/0db/${x}_phone exp/tri4b_dnn_dae/decode_phone_0db/$x || exit 1;
    ) &
   done
fi

