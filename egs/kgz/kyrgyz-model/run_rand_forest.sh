
# EXTRACT ALIGNMENTS AND FEATS

# returns <trans-id> <frame feats data>\n
./extract_trans_ali.sh in_ali.ark in_feats.ark

# same output as above, but fewer <trans-ids>
./get_sub_train.sh labeled_data.txt 500


# TRAIN FOREST

./train_python_script.py subset.trans.ali


# RECLASSIFY

# returns <utt-id> <frame feats data> in sequence
# so that we can reconstruct ali.ark
./reformat_feats_ark.sh feats.ark new_txt_feats

# classify those feats with new trained forest
./classify_python_script.py new_txt_feats output.txt

# reformat python output so it looks like original ali.ark
./reformat_tree_classes.sh

