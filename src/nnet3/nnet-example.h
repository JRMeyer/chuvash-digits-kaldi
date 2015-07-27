// nnet3/nnet-example.h

// Copyright 2012-2015  Johns Hopkins University (author: Daniel Povey)
//                2014  Vimal Manohar

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#ifndef KALDI_NNET3_NNET_EXAMPLE_H_
#define KALDI_NNET3_NNET_EXAMPLE_H_

#include "nnet3/nnet-nnet.h"
#include "hmm/posterior.h"
#include "util/table-types.h"
#include "lat/kaldi-lattice.h"
#include "hmm/posterior.h"
#include "thread/kaldi-semaphore.h"

namespace kaldi {
namespace nnet3 {


struct NnetIo {
  /// the name of the input in the neural net; in simple setups it
  /// will just be "input".
  std::string name;

  /// "indexes" is a vector the same length as features.NumRows(), explaining
  /// the meaning of each row of the "features" matrix.  Note: the "n" values
  /// in the indexes will always be zero in individual examples, but in general
  /// nonzero after we aggregate the examples into the minibatch level.
  std::vector<Index> indexes;
  
  /// The features or labels.  GeneralMatrix may contain either a CompressedMatrix,
  /// a Matrix, or SparseMatrix (a SparseMatrix would be the natural format for posteriors).
  GeneralMatrix features;
  
  /// This constructor creates NnetIo with name "name", indexes with n=0, x=0,
  /// and t values ranging from t_begin to t_begin + feats.NumRows() - 1, and
  /// the provided features.  t_begin should be the frame that feats.Row(0)
  /// represents.
  NnetIo(const std::string &name,
         int32 t_begin, const MatrixBase<BaseFloat> &feats);

  /// This constructor sets "name" to the provided string, sets "indexes" with
  /// n=0, x=0, and t from t_begin to t_begin + labels.size() - 1, and the labels
  /// as provided.  t_begin should be the frame to which labels[0] corresponds.
  NnetIo(const std::string &name,
         int32 dim,
         int32 t_begin,
         const Posterior &labels);
  
  NnetIo() { }
  
  // Use default copy constructor and assignment operators.
  void Write(std::ostream &os, bool binary) const;

  void Read(std::istream &is, bool binary);
};



/// NnetExample is the input data and corresponding label (or labels) for one or
/// more frames of input, used for standard cross-entropy training of neural
/// nets (and possibly for other objective functions). 
struct NnetExample {

  /// "io" contains the input and output.  In principle there can be multiple
  /// types of both input and output, with different names.  The order is
  /// irrelevant.
  std::vector<NnetIo> io;

  void Write(std::ostream &os, bool binary) const;
  void Read(std::istream &is, bool binary);

  NnetExample() { }

  NnetExample(const NnetExample &other): io(other.io) { }

  void Swap(NnetExample *other) { io.swap(other->io); }

  /// Compresses any features that are not sparse.
  void Compress();
};

/** Merge a set of input examples into a single example (typically the size of
    "src" will be the minibatch size).  Will crash if "src" is the empty vector.
 */
void MergeExamples(const std::vector<NnetExample> &src,
                   NnetExample *dest);


typedef TableWriter<KaldiObjectHolder<NnetExample > > NnetExampleWriter;
typedef SequentialTableReader<KaldiObjectHolder<NnetExample > > SequentialNnetExampleReader;
typedef RandomAccessTableReader<KaldiObjectHolder<NnetExample > > RandomAccessNnetExampleReader;

} // namespace nnet3
} // namespace kaldi

#endif // KALDI_NNET3_NNET_EXAMPLE_H_
