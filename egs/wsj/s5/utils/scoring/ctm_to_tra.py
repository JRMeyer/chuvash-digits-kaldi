#!/bin/env python

# Copyright 2015  Brno University of Technology (author: Karel Vesely)
# Apache 2.0

import sys
import numpy as np

# This scripts loads a 'ctm' file and converts it into the 'tra' format,
# while keeping only the word sequence, the info about time, channel
# and confidence is discarded.

# The CTM must have timing relative to the speech segment.
# So that it matches with the reference in the 'data/*/text' file.

if len(sys.argv) != 3:
  print 'Usage: %s ctm-in tra-out' % __file__
  sys.exit(1)
dummy, ctm_in, tra_out = sys.argv

if ctm_in == '-': ctm_in = '/dev/stdin'
if tra_out == '-': tra_out = '/dev/stdout'

ctm = np.loadtxt(ctm_in, dtype='object,object,f8,f8,object,f8')
ctm = np.sort(ctm, order=['f0','f1','f2'])

# Split CTM per keys from 1st column,
ctm_parts = np.split(ctm, np.nonzero(ctm['f0'][1:] != ctm['f0'][:-1])[0]+1)

# Build the 'tra' file,
tra = []
for part in ctm_parts:
  utt = part[0][0]
  words = ' '.join(part['f4'])
  tra.append((utt, words))

# Save the output 'tra' file,
np.savetxt(tra_out, np.array(tra, dtype='object,object'), fmt=['%s','%s'])

