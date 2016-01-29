#!/usr/bin/env python

# script to parse the train logs generated by nnet-compute-prob
from __future__ import division
import sys, glob, re, numpy, math, datetime, argparse
from subprocess import Popen, PIPE

def parse_train_logs(exp_dir):
  train_log_files = "%s/log/train.*.log" % (exp_dir)
  train_log_proc = Popen('grep -e Accounting {0}'.format(train_log_files),
                          shell=True,
                          stdout=PIPE,
                          stderr=PIPE)
  train_log_lines = train_log_proc.communicate()[0]
  parse_regex = re.compile(".*train\.([0-9]+)\.([0-9]+)\.log:# Accounting: time=([0-9]+) thread.*")
  train_times = {}
  for line in train_log_lines.split('\n'):
    mat_obj = parse_regex.search(line)
    if mat_obj is not None:
        groups = mat_obj.groups()
        try:
            train_times[int(groups[0])][int(groups[1])] = float(groups[2])
        except KeyError:
            train_times[int(groups[0])] = {}
            train_times[int(groups[0])][int(groups[1])] = float(groups[2])
  iters = train_times.keys()
  for iter in iters:
      values = train_times[iter].values()
      train_times[iter] = max(values)
  return train_times

def parse_prob_logs(exp_dir, key = 'accuracy'):
    train_prob_files = "%s/log/compute_prob_train.*.log" % (exp_dir)
    valid_prob_files = "%s/log/compute_prob_valid.*.log" % (exp_dir)
    train_prob_proc = Popen('grep -e {0} {1}'.format(key, train_prob_files),
                            shell=True,
                            stdout=PIPE,
                            stderr=PIPE)
    train_prob_strings = train_prob_proc.communicate()[0]
    valid_prob_proc = Popen('grep -e {0} {1}'.format(key, valid_prob_files),
                            shell=True,
                            stdout=PIPE,
                            stderr=PIPE)
    valid_prob_strings = valid_prob_proc.communicate()[0]
    #LOG (nnet3-chain-compute-prob:PrintTotalStats():nnet-chain-diagnostics.cc:149) Overall log-probability for 'output' is -0.399395 + -0.013437 = -0.412832 per frame, over 20000 fra
    #LOG (nnet3-chain-compute-prob:PrintTotalStats():nnet-chain-diagnostics.cc:144) Overall log-probability for 'output' is -0.307255 per frame, over 20000 frames.
    parse_regex = re.compile(".*compute_prob_.*\.([0-9]+).log:LOG .nnet3.*compute-prob:PrintTotalStats..:nnet.*diagnostics.cc:[0-9]+. Overall ([a-zA-Z\-]+) for 'output'.*is ([0-9.\-]+) .*per frame")
    train_loss={}
    valid_loss={}


    for line in train_prob_strings.split('\n'):
        mat_obj = parse_regex.search(line)
        if mat_obj is not None:
            groups = mat_obj.groups()
            if groups[1] == key:
                train_loss[int(groups[0])] = groups[2]

    for line in valid_prob_strings.split('\n'):
        mat_obj = parse_regex.search(line)
        if mat_obj is not None:
            groups = mat_obj.groups()
            if groups[1] == key:
                valid_loss[int(groups[0])] = groups[2]
    iters = list(set(valid_loss.keys()).intersection(train_loss.keys()))
    iters.sort()
    return numpy.array(map(lambda x: (int(x), float(train_loss[x]), float(valid_loss[x])), iters))

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="Prints accuracy/log-probability across iterations")
  parser.add_argument("--key", type=str, default="accuracy",
                       help="Value to print out")
  parser.add_argument("exp_dir", help="experiment directory, e.g. exp/nnet3/tdnn")

  args = parser.parse_args()
  exp_dir = args.exp_dir
  times = parse_train_logs(exp_dir)
  data = parse_prob_logs(exp_dir, key = args.key)
  print "%Iter\tduration\ttrain_loss\tvalid_loss\tdifference"
  for x in data:
    try:
      print "%d\t%s\t%g\t%g\t%g" % (x[0], str(times[x[0]]), x[1], x[2], x[2]-x[1])
    except KeyError:
      continue

  total_time = 0
  for iter in times.keys():
    total_time += times[iter]
  print "Total training time is {0}\n".format(str(datetime.timedelta(seconds = total_time)))
