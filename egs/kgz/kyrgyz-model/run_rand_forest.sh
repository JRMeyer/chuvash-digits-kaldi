

# first, take the aligments and feats and random subset them, so we only
# have N out of M transitionIDs for training data

./extract_trans_ali.sh ali.ark feats.ark output.trans.ali
# now I've got some <trans-id> <frame feats data>

./get_subset.sh output.trans.ali subset.trans.ali
# now ive got the same as above, but fewer <trans-ids>

# then train the forest on that subset
./train_python_script.py subset.trans.ali

# now get feats ready to reclassify
./reformat_feats_ark.sh feats.ark new_txt_feats
# now I've got <utt-id> <frame feats data>

# classify those feats with new trained forest
./classify_python_script.py new_txt_feats output.txt

# reformat python output so it looks like original ali.ark
./reformat_tree_classes.sh




