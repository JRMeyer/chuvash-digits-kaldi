

bin_ark_feats=$1

. ./path.sh

# convert bin ark file to txt file 
#copy-feats ark:$bin_ark_feats ark,t:tmp_feats.txt

# segment_name utt-id frame_i frame_j
echo "phoneme_ah org_atai_02 25 30" > segments_file.txt

extract-rows segments_file.txt ark:$bin_ark_feats ark,t:out_feats.txt

