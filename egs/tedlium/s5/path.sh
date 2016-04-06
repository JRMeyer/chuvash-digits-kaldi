export KALDI_ROOT=`pwd`/../../..
[ -f $KALDI_ROOT/tools/env.sh ] && . $KALDI_ROOT/tools/env.sh
export PATH=$PWD/utils/:$KALDI_ROOT/tools/openfst/bin:$PWD:$PATH:$KALDI_ROOT/tools/sph2pipe_v2.5
[ ! -f $KALDI_ROOT/src/path.sh ] && echo >&2 "The standard file $KALDI_ROOT/src/path.sh is not present -> Exit!" && exit 1
. $KALDI_ROOT/src/path.sh
export LC_ALL=C
