#!/usr/bin/env perl

# list of all wave files
$full_list = $ARGV[0];
# new file name for test audio
$test_list = $ARGV[1];
# new file name for train audio
$train_list = $ARGV[2];
$trainPercentage = $ARGV[3];

open FULL_LIST, $full_list;
# count total number of WAV files (ie. num of lines)
$numLines = 0;
while (<FULL_LIST>) {
	$numLines++;
}
close FULL_LIST; 

# reopening full file again here because perl
open FULL_LIST, $full_list;
# create files
open TEST_LIST, ">$test_list";
open TRAIN_LIST, ">$train_list";

#for each line, decide between test and train
$i = 0;
while ($line = <FULL_LIST>)
{
	chomp($line);
	$i++;
    # put first X percent in train
	if ($i <= $numLines * $trainPercentage )
	{
		print TRAIN_LIST "$line\n";
	}
    # put rest in test
	else
	{
		print TEST_LIST "$line\n";
	}
}

close FULL_LIST;
close TEST_LIST;
close TRAIN_LIST;
