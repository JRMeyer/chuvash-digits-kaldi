

# FORMAT TRAIN DATA

# returns <trans-id> <frame feats data>\n
./extract_trans_ali.sh exp_org/triphones_aligned/ali.1.gz plp_org/raw_plp_train.1.ark

# same output as above, but fewer <trans-ids>
./get_rand_subset_data.sh labeled_frames.txt 500


# FORMAT PREDICT DATA

# returns <utt-id> <frame feats data> in sequence
# so that we can reconstruct ali.ark
./reformat_feats.sh plp_org/raw_plp_train.1.ark


# TRAIN and RUN FOREST

./run_forest.py sub_500_data sub_500_labels reformatted_raw_plp_train.1.ark

# reformat python output so it looks like original ali.ark
./reformat_tree_classes.sh

