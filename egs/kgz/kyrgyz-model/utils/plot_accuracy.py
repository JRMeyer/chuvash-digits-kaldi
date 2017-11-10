# takes as input the output from format_acc.sh script

# train 'output-1' 993 0.584804
# train 'output-0' 993 0.465945
# train 'output-1' 994 0.58572
# train 'output-0' 994 0.461604

import matplotlib.pyplot as plt
import numpy as np
import csv
from collections import defaultdict
from operator import itemgetter


# example
# train: output1: 100: .53

data = defaultdict(dict)

with open('results.txt') as csvfile:
    reader = csv.reader(csvfile, delimiter=" ")
    
    for row in reader:
        if row[2] == "final":
            pass

        else:
            try:
                data[row[0]][row[1]].append( ( int(row[2]) , float(row[3]) ) )
            except KeyError:
                data[row[0]][row[1]] = [ ( int(row[2]) , float(row[3]) ) ]


                
def pretty(d, indent=0):
   for key, value in d.items():
      print('\t' * indent + str(key))
      if isinstance(value, dict):
         pretty(value, indent+1)
      else:
         print('\t' * (indent+1) + str(value))


train0 = [ [*x] for x in zip(* sorted(data["train"]["'output-0'"], key=itemgetter(1))) ]
train1 = [ [*x] for x in zip(* sorted(data["train"]["'output-1'"], key=itemgetter(1))) ]
valid0 = [ [*x] for x in zip(* sorted(data["valid"]["'output-0'"], key=itemgetter(1))) ]
valid1 = [ [*x] for x in zip(* sorted(data["valid"]["'output-1'"], key=itemgetter(1))) ]

plt.plot(train0[0], train0[1], label='train-TASK-A')
plt.plot(train1[0], train1[1], label='train-TASK-B')
plt.plot(valid0[0], valid0[1], label='valid-TASK-A')
plt.plot(valid1[0], valid1[1], label='valid-TASK-B')
plt.legend()
plt.xlabel('Training Iteration')
plt.title('5 Epochs | 5 Layers | 100-dim ')
plt.ylabel('Frame Classification Accuracy')
plt.show()
