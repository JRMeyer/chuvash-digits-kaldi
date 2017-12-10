from sklearn.ensemble import RandomForestClassifier
import numpy as np
import sys
import pandas as pd

train_XY=sys.argv[1]
predict_XY=sys.argv[2]

train=pd.read_csv(train_XY, sep=" ")
train_labels = np.array( [ str(i) for i in train.iloc[:,0] ] )
train_frames = train.iloc[:,1:].as_matrix()


predict=pd.read_csv(predict_XY, sep=" ")
predict_uttIDs = predict.iloc[:,0]
predict_frames = predict.iloc[:,1:].as_matrix()



# ### TRAINING ###

print("Training Random Forest")
clf = RandomForestClassifier()
clf.fit(train_frames, train_labels)



# ### PREDICTION ###


print("Prediction on data with Random Forest")
prediction=clf.predict(predict_frames)

uttIDs = np.array(predict_uttIDs).reshape(-1,1)
predictions = prediction.reshape(-1,1)
output=np.concatenate((uttIDs,predictions), axis=1)

alignments={}
for row in output:
    print(row[0])
    if row[0] in alignments:
        alignments[row[0]] = str(alignments[row[0]]) +" "+ str(row[1])
    else:
        alignments[row[0]] = str(row[1])
        



fout = "new_alignmentts.txt"
fo = open(fout, "w")

for k, v in alignments.items():
    fo.write(str(k) + str(v) + '\n')
    
fo.close()
            
           
