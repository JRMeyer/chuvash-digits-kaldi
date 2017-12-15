


# FORMAT TRAIN DATA

# concatenate all transitions and feats (separately)
cat exp_new/triphones_aligned/ali.* > ali.all.gz
cat plp_new/raw_plp_train.*.ark > raw_plp_train.all.ark

# returns <trans-id> <frame feats data>\n
./extract_trans_ali.sh ali.all.gz raw_plp_train.all.ark

# same output as above, but fewer <trans-ids>
./get_rand_subset_data.sh labeled_frames.txt 500


# FORMAT PREDICT DATA

# returns <utt-id> <frame feats data> in sequence
# so that we can reconstruct ali.ark
./reformat_feats.sh raw_plp_train.all.ark


# TRAIN and RUN FOREST

./run_forest.py sub_frames_labels reformatted_raw_plp_train.1.ark

# reformat python output so it looks like original ali.ark
./reformat_tree_classes.sh



# REPLICATE ORIGINAL ALI FORMAT
# split into num_jobs as before (8) and tar them up
split -da 1 -l $((wc -l < new_alignments.txt/8 + 1)) new_alignments.txt ali. --additional-suffix=""
j=1; for i in ali.*; do tar -czvf ${i%.*}.$j.gz $i; ((j++)); rm $i; done



# copy and rename data dirs to be used in NNET training
cp -r data_org data_new
find ./data_new/ -type f -exec sed -i 's/org_atai/new_atai/g' {} +

cp -r exp_org exp_new
find ./exp_new/ -type f -exec sed -i 's/org_atai/new_atai/g' {} +

cp -r plp_org plp_new
find ./plp_new/ -type f -exec sed -i 's/org_atai/new_atai/g' {} +

cp -r input_org input_new
find ./input_new/ -type f -exec sed -i 's/org_atai/new_atai/g' {} +
cd input_new/audio
for i in *.wav; do mv $i `echo $i | sed s/org_atai/new_atai/g`; done
