for i in ACC_nnet3_multitask_atai_*; do exp=( `echo $i | cut -d'_' -f5- | rev | cut -d'_' -f7- |rev` ); grep "valid 'output-0'" $i > $exp; done
