from sklearn.ensemble import RandomForestClassifier
import numpy as np
import sys

train_X=sys.argv[1]
train_Y=sys.argv[2]
predict_X=sys.argv[3]


### TRAINING ###

f = open(train_X)
X = np.loadtxt(f)
Y = np.genfromtxt(train_Y, dtype='str' )

clf = RandomForestClassifier()

clf.fit(X,Y)



### PREDICTION ###

f = open(predict_X)
predict_data = np.loadtxt(f)


prediction=clf.predict(predict_data)

np.savetxt("forest_output.txt", prediction, delimiter=" ")
