// nnet3/nnet-example-utils.cc

// Copyright 2012-2015    Johns Hopkins University (author: Daniel Povey)
//                2014    Vimal Manohar

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

#include "nnet3/nnet-example-utils.h"
#include "lat/lattice-functions.h"
#include "hmm/posterior.h"

namespace kaldi {
namespace nnet3 {


// get a sorted list of all NnetIo names in all examples in the list (will
// normally be just the strings "input" and "output", but maybe also "ivector").
static void GetIoNames(const std::vector<NnetExample> &src,
                            std::vector<std::string> *names_vec) {
  std::set<std::string> names;
  std::vector<NnetExample>::const_iterator iter = src.begin(), end = src.end();
  for (; iter != end; ++iter) {
    std::vector<NnetIo>::const_iterator iter2 = iter->io.begin(),
                                         end2 = iter->io.end();
    for (; iter2 != end2; ++iter2)
      names.insert(iter2->name);
  }
  CopySetToVector(names, names_vec);
}

// Get feature "sizes" for each NnetIo name, which are the total number of
// Indexes for that NnetIo (needed to correctly size the output matrix).  Also
// make sure the dimensions are consistent for each name.
static void GetIoSizes(const std::vector<NnetExample> &src,
                       const std::vector<std::string> &names,
                       std::vector<int32> *sizes) {
  std::vector<int32> dims(names.size(), -1);  // just for consistency checking.
  sizes->clear();
  sizes->resize(names.size(), 0);
  std::vector<std::string>::const_iterator names_begin = names.begin(),
                                             names_end = names.end();
  std::vector<NnetExample>::const_iterator iter = src.begin(), end = src.end();
  for (; iter != end; ++iter) {
    std::vector<NnetIo>::const_iterator iter2 = iter->io.begin(),
                                         end2 = iter->io.end();
    for (; iter2 != end2; ++iter2) {
      const NnetIo &io = *iter2;
      std::vector<std::string>::const_iterator names_iter =
          std::lower_bound(names_begin, names_end, io.name);
      KALDI_ASSERT(*names_iter == io.name);
      int32 i = names_iter - names_begin;
      int32 this_dim = io.features.NumCols();
      if (dims[i] == -1)
        dims[i] = this_dim;
      else if(dims[i] != this_dim) {
        KALDI_ERR << "Merging examples with inconsistent feature dims: "
                  << dims[i] << " vs. " << this_dim << " for '"
                  << io.name << "'.";
      }
      KALDI_ASSERT(io.features.NumRows() == io.indexes.size());
      int32 this_size = io.indexes.size();
      (*sizes)[i] += this_size;
    }
  }
}




// Do the final merging of NnetIo, once we have obtained the names, dims and
// sizes for each feature/supervision type.
static void MergeIo(const std::vector<NnetExample> &src,
                    const std::vector<std::string> &names,
                    const std::vector<int32> &sizes,
                    bool compress,
                    NnetExample *merged_eg) {
  int32 num_feats = names.size();
  std::vector<int32> cur_size(num_feats, 0);
  std::vector<std::vector<GeneralMatrix const*> > output_lists(num_feats);
  merged_eg->io.clear();
  merged_eg->io.resize(num_feats);
  for (int32 f = 0; f < num_feats; f++) {
    NnetIo &io = merged_eg->io[f];
    int32 size = sizes[f];
    KALDI_ASSERT(size > 0);
    io.name = names[f];
    io.indexes.resize(size);
  }

  std::vector<std::string>::const_iterator names_begin = names.begin(),
                                             names_end = names.end();
  std::vector<NnetExample>::const_iterator iter = src.begin(), end = src.end();
  for (int32 n = 0; iter != end; ++iter,++n) {
    std::vector<NnetIo>::const_iterator iter2 = iter->io.begin(),
                                         end2 = iter->io.end();
    for (; iter2 != end2; ++iter2) {
      const NnetIo &io = *iter2;
      std::vector<std::string>::const_iterator names_iter =
          std::lower_bound(names_begin, names_end, io.name);
      KALDI_ASSERT(*names_iter == io.name);
      int32 f = names_iter - names_begin;
      int32 this_size = io.indexes.size(),
          &this_offset = cur_size[f];
      KALDI_ASSERT(this_size + this_offset <= sizes[f]);
      output_lists[f].push_back(&(io.features));
      NnetIo &output_io = merged_eg->io[f];
      std::copy(io.indexes.begin(), io.indexes.end(),
                output_io.indexes.begin() + this_offset);
      std::vector<Index>::iterator output_iter = output_io.indexes.begin();
      // Set the n index to be different for each of the original examples.
      for (int32 i = this_offset; i < this_offset + this_size; i++) {
        // we could easily support merging already-merged egs, but I don't see a
        // need for it right now.
        KALDI_ASSERT(output_iter[i].n == 0 &&
                     "Merging already-merged egs?  Not currentlysupported.");
        output_iter[i].n = n;
      }
      this_offset += this_size;  // note: this_offset is a reference.
    }
  }
  KALDI_ASSERT(cur_size == sizes);
  for (int32 f = 0; f < num_feats; f++) {
    AppendGeneralMatrixRows(output_lists[f],
                            &(merged_eg->io[f].features));
    if (compress) {
      // the following won't do anything if the features were sparse.
      merged_eg->io[f].features.Compress();
    }
  }
}



void MergeExamples(const std::vector<NnetExample> &src,
                   bool compress,
                   NnetExample *merged_eg) {
  KALDI_ASSERT(!src.empty());
  std::vector<std::string> io_names;
  GetIoNames(src, &io_names);
  // the sizes are the total number of Indexes we have across all examples.
  std::vector<int32> io_sizes;
  GetIoSizes(src, io_names, &io_sizes);
  MergeIo(src, io_names, io_sizes, compress, merged_eg);
}

void ShiftExampleTimes(int32 t_offset,
                       const std::vector<std::string> &exclude_names,
                       NnetExample *eg) {
  if (t_offset == 0)
    return;
  std::vector<NnetIo>::iterator iter = eg->io.begin(),
      end = eg->io.end();
  for (; iter != end; iter++) {
    bool name_is_excluded = false;
    std::vector<std::string>::const_iterator
        exclude_iter = exclude_names.begin(),
        exclude_end = exclude_names.end();
    for (; exclude_iter != exclude_end; ++exclude_iter) {
      if (iter->name == *exclude_iter) {
        name_is_excluded = true;
        break;
      }
    }
    if (!name_is_excluded) {
      // name is not something like "ivector" that we exclude from shifting.
      std::vector<Index>::iterator index_iter = iter->indexes.begin(),
          index_end = iter->indexes.end();
      for (; index_iter != index_end; ++index_iter)
        index_iter->t += t_offset;
    }
  }
}

void GetComputationRequest(const Nnet &nnet,
                           const NnetExample &eg,
                           bool need_model_derivative,
                           bool store_component_stats,
                           ComputationRequest *request) {
  request->inputs.clear();
  request->inputs.reserve(eg.io.size());
  request->outputs.clear();
  request->outputs.reserve(eg.io.size());
  request->need_model_derivative = need_model_derivative;
  request->store_component_stats = store_component_stats;
  for (size_t i = 0; i < eg.io.size(); i++) {
    const NnetIo &io = eg.io[i];
    const std::string &name = io.name;
    int32 node_index = nnet.GetNodeIndex(name);
    if (node_index == -1 &&
        !nnet.IsInputNode(node_index) && !nnet.IsOutputNode(node_index))
      KALDI_ERR << "Nnet example has input or output named '" << name
                << "', but no such input or output node is in the network.";

    std::vector<IoSpecification> &dest =
        nnet.IsInputNode(node_index) ? request->inputs : request->outputs;
    dest.resize(dest.size() + 1);
    IoSpecification &io_spec = dest.back();
    io_spec.name = name;
    io_spec.indexes = io.indexes;
    io_spec.has_deriv = nnet.IsOutputNode(node_index) && need_model_derivative;
  }
  // check to see if something went wrong.
  if (request->inputs.empty())
    KALDI_ERR << "No inputs in computation request.";
  if (request->outputs.empty())
    KALDI_ERR << "No outputs in computation request.";
}

void WriteVectorAsChar(std::ostream &os,
                       bool binary,
                       const VectorBase<BaseFloat> &vec) {
  if (binary) {
    int32 dim = vec.Dim();
    std::vector<unsigned char> char_vec(dim);
    const BaseFloat *data = vec.Data();
    for (int32 i = 0; i < dim; i++) {
      BaseFloat value = data[i];
      KALDI_ASSERT(value >= 0.0 && value <= 1.0);
      // below, the adding 0.5 is done so that we round to the closest integer
      // rather than rounding down (since static_cast will round down).
      char_vec[i] = static_cast<unsigned char>(255.0 * value + 0.5);
    }
    WriteIntegerVector(os, binary, char_vec);
  } else {
    // the regular floating-point format will be more readable for text mode.
    vec.Write(os, binary);
  }
}

void ReadVectorAsChar(std::istream &is,
                      bool binary,
                      Vector<BaseFloat> *vec) {
  if (binary) {
    BaseFloat scale = 1.0 / 255.0;
    std::vector<unsigned char> char_vec;
    ReadIntegerVector(is, binary, &char_vec);
    int32 dim = char_vec.size();
    vec->Resize(dim, kUndefined);
    BaseFloat *data = vec->Data();
    for (int32 i = 0; i < dim; i++)
      data[i] = scale * char_vec[i];
  } else {
    vec->Read(is, binary);
  }
}

void RoundUpNumFrames(int32 frame_subsampling_factor,
                      int32 *num_frames,
                      int32 *num_frames_overlap) {
  if (*num_frames % frame_subsampling_factor != 0) {
    int32 new_num_frames = frame_subsampling_factor *
        (*num_frames / frame_subsampling_factor + 1);
    KALDI_LOG << "Rounding up --num-frames=" << (*num_frames)
              << " to a multiple of --frame-subsampling-factor="
              << frame_subsampling_factor
              << ", now --num-frames=" << new_num_frames;
    *num_frames = new_num_frames;
  }
  if (*num_frames_overlap % frame_subsampling_factor != 0) {
    int32 new_num_frames_overlap = frame_subsampling_factor *
        (*num_frames_overlap / frame_subsampling_factor + 1);
    KALDI_LOG << "Rounding up --num-frames-overlap=" << (*num_frames_overlap)
              << " to a multiple of --frame-subsampling-factor="
              << frame_subsampling_factor
              << ", now --num-frames-overlap=" << new_num_frames_overlap;
    *num_frames_overlap = new_num_frames_overlap;
  }
  if (*num_frames_overlap < 0 || *num_frames_overlap >= *num_frames) {
    KALDI_ERR << "--num-frames-overlap=" << (*num_frames_overlap) << " < "
              << "--num-frames=" << (*num_frames);
  }

}


/*
  This comment describes the idea behind what InitChunkSize() is supposed to do,
  and how it relates to the purpose of class UtteranceSplitter.

  Class UtteranceSplitter is supposed to tell us, for a given utterance length,
  what chunk sizes to use.  The chunk sizes it may choose are:
    - zero or more chunks of the 'principal' size (the first-listed value in
      num-frames)
    - at most two chunks of 'alternative' num-frames (any but the first-listed
      num-frames).

  (and an empty list of chunks is not allowed as a split).  A split is
  effectively a multiset of chunk-sizes (the order will be randomized by the
  caller).  We represent it in code as a list of chunk-sizes, represented as a
  std::vector, which is sorted to get a unique representation without repeats of
  different orderings.

  The choice of spilt is determined by a cost-function that depends on the sum
  of the chunk-sizes in the split and the length of the utterance: the idea is
  that we want the sum of chunk-sizes in the split to be as close as possible to
  the utterance length.  The cost-function penalizes the sum of chunk-sizes
  being smaller than the utterance-length (leading to gaps) twice as much as
  when the sum of chunk-sizes is larger than the utterance length.  I.e.
    cost(chunk_size_sum, utt_length) = (chunk_size_sum > utt_length ?
                                         chunk_size_sum - utt_length :
                                         2 * (utt_length - chunk_size_sum))
  [but as a special case, set c to infinity if the largest chunk size in the
  split is longer than the utterance length; we couldn't, in that case, use
  this split for this utterance].


  We want to make sure a good variety of combinations of chunk sizes are chosen
  in case there are ties from the cost function.  For each utterance length
  we store the set of splits, whose costs are within 2
  of the best cost available for that utterance length.  When asked to find
  chunks for a particular utterance of that length, we will choose randomly
  from that pool of splits.
 */
void UtteranceSplitter::InitChunkSize() {
  int32 max_utterance_length = MaxUtteranceLength();

  // The 'splits' vector is a list of possible splits (a split being
  // a multiset of chunk-sizes, represented as a sorted vector).
  // The vector 'splits' is itself sorted.
  std::vector<std::vector<int32> > splits;
  InitSplits(&splits);


  // Define a split-index 0 <= s < splits.size() as index into the 'splits'
  // vector, and let a cost c >= 0 represent the mismatch between an
  // utterance length and the total length of the chunk sizes in a split:

  //  c(chunk_size_sum, utt_length) = (chunk_size_sum > utt_length ?
  //                                    chunk_size_sum - utt_length :
  //                                    2 * (utt_length - chunk_size_sum))
  // [but as a special case, set c to infinity if the largest chunk size in the
  //  split is longer than the utterance length; we couldn't, in that case, use
  //  this split for this utterance].

  // 'costs_for_length[u][s]', indexed by utterance-length u and then split,
  // contains the cost for utterance-length u and split s.

  std::vector<std::vector<int32> > costs_for_length(
      max_utterance_length + 1);
  int32 num_splits = splits.size();


  for (int32 u = 0; u <= max_utterance_length; u++)
    pairs_for_length[u].reserve(num_splits);

  for (int32 s = 0; s < num_splits; s++) {
    const std::vector<int32> &split = splits[s];
    int32 chunk_size_sum = std::accumulate(split.begin(), split.end(),
                                           int32(0)),
        max_chunk_size = *std::max_element(split.begin(), split.end());
    for (int32 u = 0; u <= max_utterance_length; u++) {
      // c is the cost for this utterance length and this split.  We penalize
      // gaps twice as strongly as overlaps, based on the intuition that
      // completely throwing out frames of data is worse than counting them
      // twice.  It might be possible to come up with some kind of mathematical
      // justification for this based on variance of the estimated gradient.
      int32 c = (chunk_size_sum > u ? chunk_size_sum - u :
                 2 * (u - chunk_size_sum));
      if (max_chunk_size > u)
        c = std::numeric_limits<int32>::max();
      pairs_for_length[u].push_back(c);
    }
  }


  splits_for_length_.resize(max_utterance_length + 1);


  for (int32 u = 0; u <= max_utterance_length; u++) {
    const std::vector<int32> &costs = costs_for_length[u];
    int32 min_cost = std::min_element(costs.begin(), costs.end());
    if (min_cost == std::numeric_limits<int32>::max()) {
      // All costs were infinity, becaues this utterance-length u is shorter
      // than the smallest chunk-size.  Leave splits_for_length_[u] as empty
      // for this utterance-length, meaning we will not be able to choose any
      // split, and such utterances will be discarded.
      continue;
    }
    int32 cost_threshold = 2;  // We will choose pseudo-randomly from splits
                               // that are within this distance from the best
                               // cost.
    std::vector<int32> possible_splits;
    std::vector<int32>::const_iterator iter = costs.begin(), end = costs.end();
    int32 s = 0;
    for (; iter != end; ++iter,++s)
      if (*iter < min_cost + cost_threshold)
        splits_for_length_[u].push_back(splits[s]);
  }

  if (GetVerboseLevel() >= 3) {
    std::ostringstream os;
    for (int32 u = 0; u <= max_utterance_length; u++) {
      if (!splits_for_length_[u].empty()) {
        os << u << "=(";
        std::vector<std::vector<int32 > >::const_iterator
            iter1 = splits_for_length_[u].begin(),
            end1 = splits_for_length_[u].end();

        while (iter1 != end1) {
          std::vector<int32>::const_iterator iter2 = iter1->begin(),
              end2 = iter1->end();
          while (iter2 != end2) {
            os << *iter2;
            ++iter2;
            if (iter2 != end2) os << ",";
          }
          ++iter1;
          if (iter1 != end1) os << "/";
        }
        os << ")";
        if (u < max_utterance_length) os << ", ";
      }
    }
    KALDI_VLOG(3) << "Utterance-length-to-splits map is: " << os.str();
  }
}


void GetChunkSizesForUtterance(int32 utterance_length,
                               std::vector<int32> *chunk_sizes) const {
  KALDI_ASSERT(!splits_for_length.empty());
  // 'primary_length' is the first-specified num-frames.
  // It's the only chunk that may be repeated an arbitrary number
  // of times.
  int32 primary_length = config_.num_frames[0],
      max_tabulated_length = splits_for_length_.size() - 1,
      num_primary_length_repeats = 0;

  KALDI_ASSERT(utterance_length >= 0);
  while (utterance_length > max_tabulated_length) {
    utterance_length -= primary_length;
    num_primary_length_repeats++;
  }
  KALDI_ASSERT(utterance_length >= 0);
  const std::vector<std::vector<int32> > &possible_splits =
      splits_for_length_[utterance_length];
  int32 num_possible_splits = possible_splits.size(),
      randomly_chosen_split = RandInt(0, num_possible_splits - 1);
  *chunk_sizes = possible_splits[randomly_chosen_split];
  for (int32 i = 0; i < num_primary_length_repeats; i++)
    chunk_sizes->push_back(primary_length);
  // Randomize the order in which the chunks appear.
  std::random_shuffle(chunk_sizes->begin(),
                      chunk_sizes->end());
}


int32 UtteranceSplitter::MaxUtteranceLength() const {
  int32 num_lengths = config_.num_frames.size();
  KALDI_ASSERT(num_lengths > 0);
  // 'primary_length' is the first-specified num-frames.
  // It's the only chunk that may be repeated an arbitrary number
  // of times.
  int32 primary_length = config_.num_frames[0],
      max_length = primary_length;
  for (int32 i = 0; i < num_lengths; i++) {
    KALDI_ASSERT(config_.num_frames[i] > 0);
    max_length = std::max(config_.num_frames[i], max_length);
  }
  return 2 * max_length + primary_length;
}

void UtteranceSplitter::InitSplits(std::vector<std::vector<int32> > *splits) const {
  // we consider splits whose total length is up to MaxUtteranceLength() +
  // primary_length.  We can be confident without doing a lot of math, that
  // multisets above this length will never be chosen for any utterance-length
  // up to MaxUtteranceLength().
  int32 primary_length = config_.num_frames[0],
      length_ceiling = MaxUtteranceLength() + primary_length;

  typedef std::unordered_set<std::vector<int32>, VectorHasher<int32> > SetType;

  SetType splits_set;

  int32 num_lengths = config_.num_frames.size();

  // The splits we are allow are: zero to two 'alternate' lengths, plus
  // an arbitrary number of repeats of the 'primary' length.  The repeats
  // of the 'primary' length are handled by the inner loop over n.
  // The zero two two 'alternate' lengths are handled by the loops over
  // i and j.  i == 0 and j == 0 are special cases; they mean, no
  // alternate is chosen.
  for (int32 i = 0; i < num_lengths; i++) {
    for (int32 j = 0; j < num_length; j++) {
      std::vector<int32> vec;
      if (i > 0)
        vec.push_back(config_.num_frames[i]);
      if (j > 0)
        vec.push_back(config_.num_frames[j]);
      for (int32 n = 0;
           std::accumulate(vec.begin(), vec.end(), int32(0)) <= length_ceiling;
           ++n, vec.push_back(primary_length)) {
        std::sort(vec.begin(), vec.end());  // we don't want to treat different
                                            // orderings of the same values as
                                            // different, so sort them.
        if (!vec.empty()) // Don't allow the empty vector as a split.
          splits_set.insert(vec);
      }
    }
  }
  for (SetType::const_iterator iter = splits_set.begin();
       iter != splits_set.end(); ++iter)
    splits->push_back(*iter);
  std::sort(splits->begin(), splits->end());  // make the order deterministic,
                                              // for consistency of output
                                              // between runs and C libraries.
}


// static
void UtteranceSplitter::DistributeRandomly(int32 n, std::vector<int32> *vec) {
  KALDI_ASSERT(!vec->empty());
  int32 size = vec->size();
  if (n < 0) {
    DistributeRandomly(n, vec);
    for (int32 i = 0; i < size; i++)
      (*vec)[i] *= -1;
    return;
  }
  // from this point we know n >= 0.
  int32 common_part = n / size,
      remainder = n % size, i;
  for (i = 0; i < remainder; i++) {
    (*vec)[i] = common_part + 1;
  }
  for (; i < size; i++) {
    (*vec)[i] = common_part;
  }
  std::random_shuffle(vec->begin(), vec->end());
  KALDI_ASSERT(std::accumulate(vec->begin(), vec->end(), int32(0)) == n);
}


void UtteranceSplitter::GetGapSizes(int32 utterance_length,
                                    bool enforce_subsampling_factor,
                                    const std::vector<int32> &chunk_sizes,
                                    std::vector<int32> *gap_sizes) const {
  if (chunk_sizes.empty()) {
    gap_sizes->clear();
    return;
  }
  if (enforce_subsamping_factor && config_.frame_subsampling_factor > 1) {
    int32 sf = config_.frame_subsampling_factor, size = chunk_sizes.size();
    int32 utterance_length_reduced = (utterance_length + (sf - 1)) / sf;
    std::vector<int32> chunk_sizes_reduced(chunk_sizes);
    for (int32 i = 0; i < size; i++) {
      KALDI_ASSERT(chunk_sizes[i] % config_.frame_subsampling_factor == 0);
      chunk_sizes_reduced[i] /= config_.frame_subsampling_factor;
    }
    GetGapSizes(utterance_length_reduced, false,
                chunk_sizes_reduced, gap_sizes);
    KALDI_ASSERT(gap_sizes->size() == static_cast<size_t>(size));
    for (int32 i = 0; i < size; i++)
      (*gap_sizes)[i] *= config_.frame_subsampling_factor;
    return;
  }
  int32 num_chunks = chunk_sizes.size(),
      total_of_chunk_sizes = std::accumulate(chunk_sizes.begin(),
                                             chunk_sizes.end(),
                                             int32(0)),
      total_gap = utterance_length - total_of_chunk_sizes;
  gap_sizes->resize(num_chunks);

  if (total_gap < 0) {
    // there is an overlap.  Overlaps can only go between chunks, not at the
    // beginning or end of the utterance.
    if (num_chunks == 1) {
      // there needs to be an overlap, but there is only one chunk... this means
      // the chunk-size exceeds the utterance length, which is not allowed.
      KALDI_ERR << "Chunk size is " << chunk_sizes[0]
                << " but utterance length is only "
                << utterance_length;
    }

    // note the elements of 'overlaps' will be <= 0.
    std::vector<int32> overlaps(num_chunks - 1);
    DistributeRandomly(total_gap, &num_overlap_locations);
    (*gap_sizes)[0] = 0;  // no gap before 1st chunk.
    for (int32 i = 1; i < num_chunks; i++)
      (*gap_sizes)[i] = overlaps[i-1];
  } else {
    // There may be a gap.  Gaps can go at the start or end of the utterance, or
    // between segments.
    std::vector<int32> gaps(num_chunks + 1);
    DistributeRandomly(total_gap, &gaps);
    // the last element of 'gaps', the one at the end of the utterance, is
    // implicit and doesn't have to be written to the output.
    for (int32 i = 0; i < num_chunks; i++)
      (*gap_sizes)[i] = gaps[i];
  }
}


void UtteranceSplitter::GetChunksForUtterance(
    int32 utterance_length,
    std::vector<ChunkTimeInfo> *chunk_info) const {
  std::vector<int32> chunk_sizes;
  GetChunkSizesForUtterance(utterance_length, &chunk_sizes);
  std::vector<int32> gaps(chunk_sizes.size());
  GetGapSizes(utterance_length, true, chunk_sizes, &gap_sizes);
  int32 num_chunks = chunk_sizes.size();
  chunk_info->resize(num_chunks);
  int32 t = 0;
  for (int32 i = 0; i < num_chunks; i++) {
    t += gaps[i];
    ChunkTimeInfo &info = (*chunk_info)[i];
    info.first_frame = t;
    info.num_frames = chunk_sizes[i];
    info.left_context = (i == 0 && config_.left_context_initial >= 0 ?
                         config_.left_context_initial : config_.left_context);
    info.right_context = (i == 0 && config_.right_context_final >= 0 ?
                         config_.right_context_final : config_.right_context);
    t += chunk_sizes[i];
  }
  // check that the end of the last chunk doesn't go more than
  // 'config_.frame_subsampling_factor - 1' frames past the end
  // of the utterance.  That amount, we treat as rounding error.
  KALDI_ASSERT(t - utterance_length < config_.frame_subsampling_factor);
}

} // namespace nnet3
} // namespace kaldi
