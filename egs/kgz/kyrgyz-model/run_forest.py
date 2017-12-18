from sklearn.ensemble import RandomForestClassifier
import numpy as np
import sys
import pandas as pd

train_XY=sys.argv[1]
predict_XY=sys.argv[2]



# READ in training and testing data

print("Assuming input data is a CSV, space-delimited, no header")
train=pd.read_csv(train_XY, sep=" ", header=None)
train_labels = np.array( [ str(i) for i in train.iloc[:,0] ] )
train_frames = train.iloc[:,1:].as_matrix()

predict=pd.read_csv(predict_XY, sep=" ", header=None)
predict_uttIDs = predict.iloc[:,0]
predict_frames = predict.iloc[:,1:].as_matrix()

print(type(predict_frames))

# ### TRAINING ###

print("Training Random Forest on 8 CPUs")
clf = RandomForestClassifier(n_jobs=8,
                             n_estimators=2,
                             criterion='entropy',
                             max_depth=None,
                             min_samples_split=20,
                             min_samples_leaf=10,
                             bootstrap=True,
                             verbose=2)

clf.fit(train_frames, train_labels)



# ### PREDICTION ###

print("Prediction on data with Random Forest")

# the multiprocessing is already in the definition of the classifier!
# no need to redefine here

prediction = clf.predict(predict_frames)






# ### PRINT NEW ALIGNMENTS TO DISK ###

predictions = prediction.reshape(-1,1)
uttIDs = np.array(predict_uttIDs).reshape(-1,1)
output=np.concatenate((uttIDs,predictions), axis=1)

alignments={}
for row in output:
    if row[0] in alignments:
        alignments[row[0]] = str(alignments[row[0]]) +" "+ str(row[1])
    else:
        alignments[row[0]] = str(row[1])
        print("Formatting " + str(row[0]))

fout = "new_alignments.txt"
fo = open(fout, "w")

for k, v in alignments.items():
    fo.write(str(k) + " " + str(v) + '\n')
    
fo.close()
            
           
