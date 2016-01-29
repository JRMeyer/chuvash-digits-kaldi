// nnet3/nnet-general-component.cc

// Copyright      2015  Johns Hopkins University (author: Daniel Povey)

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

#include <iterator>
#include <sstream>
#include "nnet3/nnet-general-component.h"
#include "nnet3/nnet-computation-graph.h"
#include "nnet3/nnet-parse.h"

namespace kaldi {
namespace nnet3 {

//inline
void DistributeComponent::ComputeInputIndexAndBlock(const Index &output_index,
                                                    Index *input_index,
                                                    int32 *block) const {
  int32 num_blocks = input_dim_ / output_dim_;
  *input_index = output_index;
  int32 output_x = output_index.x, input_x;
  if (output_x >= 0) {
    input_x = output_x / num_blocks;
  } else {
    input_x = (output_x - num_blocks + 1) / num_blocks;
  }
  input_index->x = input_x;
  if (block)
    *block = output_x - (input_x * num_blocks);
}

//virtual
void DistributeComponent::GetInputIndexes(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    std::vector<Index> *desired_indexes) const {
  desired_indexes->resize(1);
  ComputeInputIndexAndBlock(output_index, &((*desired_indexes)[0]), NULL);
}

//virtual
bool DistributeComponent::IsComputable(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    const IndexSet &input_index_set,
    std::vector<Index> *used_inputs) const {
  Index input_index;
  ComputeInputIndexAndBlock(output_index, &input_index, NULL);
  if (!input_index_set(input_index))
    return false;
  if (used_inputs) {
    used_inputs->clear();
    used_inputs->push_back(input_index);
  }
  return true;
}

class DistributeComponentPrecomputedIndexes:
      public ComponentPrecomputedIndexes {
 public:

  // each pair is a pair (row, dim_offset), and by
  // computing (input.Data() + row * input.Stride() + dim_offset)
  // we get an address that points to the correct input location.
  std::vector<std::pair<int32, int32> > pairs;

  // this class has a virtual destructor so it can be deleted from a pointer
  // to ComponentPrecomputedIndexes.
  virtual ~DistributeComponentPrecomputedIndexes() { }

  virtual ComponentPrecomputedIndexes* Copy() const {
    return new DistributeComponentPrecomputedIndexes(*this);
  }
  void Write(std::ostream &os, bool binary) const {
    KALDI_ERR << "Write not implemented for this class";
  }
  void Read(std::istream &is, bool binary) {
    KALDI_ERR << "Read not implemented for this class";
  }
};

// virtual
ComponentPrecomputedIndexes* DistributeComponent::PrecomputeIndexes(
    const MiscComputationInfo &, // misc_info
    const std::vector<Index> &input_indexes,
    const std::vector<Index> &output_indexes,
    bool) const {  // the bool is 'need_backprop'- unused.
  unordered_map<Index, int32, IndexHasher> index_to_input_dim;
  int32 num_input_indexes = input_indexes.size(),
      num_output_indexes = output_indexes.size();
  for (int32 i = 0; i < num_input_indexes; i++)
    index_to_input_dim[input_indexes[i]] = i;
  DistributeComponentPrecomputedIndexes *ans = new
      DistributeComponentPrecomputedIndexes;
  ans->pairs.resize(output_indexes.size());

  int32 num_blocks = input_dim_ / output_dim_,
      block_size = input_dim_ / num_blocks;

  for (int32 i = 0; i < num_output_indexes; i++) {
    Index input_index;
    int32 block_index;
    ComputeInputIndexAndBlock(output_indexes[i], &input_index, &block_index);
    unordered_map<Index, int32, IndexHasher>::iterator iter =
        index_to_input_dim.find(input_index);
    if (iter == index_to_input_dim.end())
      KALDI_ERR << "Input index not found (code error)";
    int32 input_row = iter->second;
    ans->pairs[i] = std::pair<int32,int32>(input_row, block_index * block_size);
  }
  return ans;
}


void DistributeComponent::ComputeInputPointers(
    const ComponentPrecomputedIndexes *indexes_in,
    const CuMatrixBase<BaseFloat> &in,
    int32 num_output_rows,
    std::vector<const BaseFloat*> *input_pointers) const {
  const DistributeComponentPrecomputedIndexes *indexes =
      dynamic_cast<const DistributeComponentPrecomputedIndexes*>(indexes_in);
  KALDI_ASSERT(indexes != NULL && "Invalid pointer type");
  KALDI_ASSERT(num_output_rows == static_cast<int32>(indexes->pairs.size()));
  input_pointers->resize(num_output_rows);

  const BaseFloat *input_data = in.Data();
  int32 input_stride = in.Stride();
  const BaseFloat **input_pointers_data = &((*input_pointers)[0]);
  const std::pair<int32, int32> *pairs_data = &(indexes->pairs[0]);
  for (int32 i = 0; i < num_output_rows; i++) {
    input_pointers_data[i] = input_data +
        pairs_data[i].first * input_stride +
        pairs_data[i].second;
  }
}

void DistributeComponent::ComputeInputPointers(
    const ComponentPrecomputedIndexes *indexes_in,
    int32 num_output_rows,
    CuMatrixBase<BaseFloat> *in,
    std::vector<BaseFloat*> *input_pointers) const {
  const DistributeComponentPrecomputedIndexes *indexes =
      dynamic_cast<const DistributeComponentPrecomputedIndexes*>(indexes_in);
  KALDI_ASSERT(indexes != NULL && "Invalid pointer type");
  KALDI_ASSERT(num_output_rows == static_cast<int32>(indexes->pairs.size()));
  input_pointers->resize(num_output_rows);

  BaseFloat *input_data = in->Data();
  int32 input_stride = in->Stride();
  BaseFloat **input_pointers_data = &((*input_pointers)[0]);
  const std::pair<int32, int32> *pairs_data = &(indexes->pairs[0]);
  for (int32 i = 0; i < num_output_rows; i++) {
    input_pointers_data[i] = input_data +
        pairs_data[i].first * input_stride +
        pairs_data[i].second;
  }
}


// virtual
void DistributeComponent::Propagate(const ComponentPrecomputedIndexes *indexes,
                                    const CuMatrixBase<BaseFloat> &in,
                                    CuMatrixBase<BaseFloat> *out) const {
  KALDI_ASSERT(indexes != NULL &&
               in.NumCols() == input_dim_ && out->NumCols() == output_dim_);
  int32 num_output_rows = out->NumRows();
  std::vector<const BaseFloat*> input_pointers;
  ComputeInputPointers(indexes, in, num_output_rows, &input_pointers);
  CuArray<const BaseFloat*> input_pointers_cuda(input_pointers);
  out->CopyRows(input_pointers_cuda);
}

// virtual
void DistributeComponent::Backprop(const std::string &debug_info,
                                   const ComponentPrecomputedIndexes *indexes,
                                   const CuMatrixBase<BaseFloat> &, // in_value,
                                   const CuMatrixBase<BaseFloat> &, // out_value
                                   const CuMatrixBase<BaseFloat> &out_deriv,
                                   Component *, // to_update,
                                   CuMatrixBase<BaseFloat> *in_deriv) const {
  if (in_deriv == NULL) return;

  int32 num_blocks = input_dim_ / output_dim_,
      num_output_rows = out_deriv.NumRows();
  if (num_output_rows != in_deriv->NumRows() * num_blocks) {
    // there could be some 'gaps', i.e. some input values that are not ever
    // referred to.  So we need to zero the input.  This would't happen in the
    // setups I plan to use this for.
    in_deriv->SetZero();
  }

  std::vector<BaseFloat*> input_pointers;
  ComputeInputPointers(indexes, num_output_rows, in_deriv, &input_pointers);
  CuArray<BaseFloat*> input_pointers_cuda(input_pointers);
  out_deriv.CopyToRows(input_pointers_cuda);
}


void DistributeComponent::Init(int32 input_dim, int32 output_dim) {
  input_dim_ = input_dim;
  output_dim_ = output_dim;
  KALDI_ASSERT(input_dim > 0 && output_dim > 0 && input_dim % output_dim == 0);
}

// virtual
void DistributeComponent::InitFromConfig(ConfigLine *cfl) {
  int32 input_dim, output_dim;
  bool ok = cfl->GetValue("input-dim", &input_dim) &&
      cfl->GetValue("output-dim", &output_dim);
  if (!ok || cfl->HasUnusedValues())
    KALDI_ERR << "Invalid initializer for layer of type "
              << Type() << ": \"" << cfl->WholeLine() << "\"";
  else
    Init(input_dim, output_dim);
}

void DistributeComponent::Write(std::ostream &os, bool binary) const {
  WriteToken(os, binary, "<DistributeComponent>");
  WriteToken(os, binary, "<InputDim>");
  WriteBasicType(os, binary, input_dim_);
  WriteToken(os, binary, "<OutputDim>");
  WriteBasicType(os, binary, output_dim_);
  WriteToken(os, binary, "</DistributeComponent>");
}

void DistributeComponent::Read(std::istream &is, bool binary) {
  ExpectOneOrTwoTokens(is, binary, "<DistributeComponent>", "<InputDim>");
  ReadBasicType(is, binary, &input_dim_);
  ExpectToken(is, binary, "<OutputDim>");
  ReadBasicType(is, binary, &output_dim_);
  ExpectToken(is, binary, "</DistributeComponent>");
}


class StatisticsExtractionComponentPrecomputedIndexes:
      public ComponentPrecomputedIndexes {
 public:
  // While creating the output we sum over row ranges of the input.
  // forward_indexes.Dim() equals the number of rows of the output, and each
  // element is a (start, end) range of inputs, that is summed over.
  CuArray<Int32Pair> forward_indexes;

  // this vector stores the number of inputs for each output.  Normally this will be
  // the same as the component's output_period_ / input_period_, but could be less
  // due to edge effects at the utterance boundary.
  Vector<BaseFloat> counts;

  // Each input row participates in exactly one output element, and
  // 'backward_indexes' identifies which row of the output each row
  // of the input is part of.  It's used in backprop.
  CuArray<int32> backward_indexes;


  virtual void Write(std::ostream &os, bool binary) const {
    WriteToken(os, binary, "<StatisticsExtractionComponentPrecomputedIndexes>");
    WriteToken(os, binary, "<ForwardIndexes>");
    forward_indexes.Write(os, binary);
    WriteToken(os, binary, "<Counts>");
    counts.Write(os, binary);
    WriteToken(os, binary, "<BackwardIndexes>");
    backward_indexes.Write(os, binary);
    WriteToken(os, binary, "</StatisticsExtractionComponentPrecomputedIndexes>");
  }
  virtual void Read(std::istream &is, bool binary) {
    ExpectOneOrTwoTokens(is, binary,
                         "<StatisticsExtractionComponentPrecomputedIndexes>",
                         "<ForwardIndexes>");
    forward_indexes.Read(is, binary);
    ReadToken(is, binary, "<Counts>");
    counts.Read(is, binary);
    ExpectToken(is, binary, "<BackwardIndexes>");
    backward_indexes.Read(is, binary);
    ExpectToken(is, binary, "</StatisticsExtractionComponentPrecomputedIndexes>");
  }
  virtual ~StatisticsExtractionComponentPrecomputedIndexes() { }
};


ComponentPrecomputedIndexes*
StatisticsExtractionComponent::PrecomputeIndexes(
    const MiscComputationInfo &misc_info,
    const std::vector<Index> &input_indexes,
    const std::vector<Index> &output_indexes,
    bool need_backprop) const {
  int32 num_input_indexes = input_indexes.size(),
      num_output_indexes = output_indexes.size();
  StatisticsExtractionComponentPrecomputedIndexes *ans = new
      StatisticsExtractionComponentPrecomputedIndexes();
  // both input and output indexes are assumed sorted first on
  // n and x, then on t.
  Int32Pair invalid_pair;
  invalid_pair.first = -1;
  invalid_pair.second = -1;
  std::vector<Int32Pair> forward_indexes_cpu(output_indexes.size(),
                                             invalid_pair);
  std::vector<int32> backward_indexes_cpu(input_indexes.size(), -1);
  Vector<BaseFloat> counts_cpu(output_indexes.size());

  // this map maps from Index to the position in 'input_indexes'.
  std::unordered_map<Index, int32> index_to_input_pos;
  for (i = 0; i < num_input_indexes; i++)
    index_to_input_pos[input_indexes[i]] = i;

  for (int32 i = 0; i < num_output_indexes; i++) {
    Index output_index = output_indexes[i];
    Index input_index(output_index);
    int32 t = output_index.t,
        t_start = output_period_ * (t / output_period_);
    if (t_start > output_period_)   // could happen for negative t_start due to
      t_start -= output_period_;    // the way modulus works in c.
    int32 t_end = t_start + output_period_;
    for (int32 t = t_start; t < t_end; t += input_period_) {
      input_index.t = t;
      std::unordered_map<Index, int32>::iterator iter =
          index_to_input_pos.find(input_index);
      if (iter != index_to_input_pos.end()) {
        int32 input_pos = iter->second;
        if (forward_indexes_cpu[i].first == -1) {
          forward_indexes_cpu[i].first = input_pos;
          forward_indexes_cpu[i].second = input_pos + 1;
          counts_cpu(i) = 1.0;
        } else {
          // the following might fail, for instance, if the sorting
          // of the input or output indexes was not as expected.
          KALDI_ASSERT(forward_indexes_cpu[i].second == input_post);
          forward_indexes_cpu[i].second++;
          counts_cpu(i) += 1.0;
        }
        KALDI_ASSERT(input_indexes[input_pos] == -1);
        input_indexes[input_pos] = i;
      }
      KALDI_ASSERT(counts_cpu(i) != 0.0);
    }
  }
  for (int32 i = 0; i < num_input_indexes; i++) {
    KALDI_ASSERT(input_indexes[i] != -1);
  }
  ans->forward_indexes = forward_indexes_cpu;
  ans->counts = counts_cpu;
  if (need_backprop)
    ans->backward_indexes = backward_indexes_cpu;
  return ans;
}

StatisticsExtractionComponent::StatisticsExtractionComponent():
    input_dim_(-1), input_period_(1), output_period_(1),
    include_variance_(true) { }

StatisticsExtractionComponent::StatisticsExtractionComponent(
    const StatisticsExtractionComponent &other):
    input_dim_(other.input_dim_),
    input_period_(other.input_period_),
    output_period_(other.output_period_),
    include_variance_(other.include_variance_) {
  Check();
}

virtual void StatisticsExtractionComponent::InitFromConfig(ConfigLine *cfl) {
  // input-dim is required.
  bool ok = cfl->GetValue("input-dim", &input_dim_);
  cfl->GetValue("input-period", &input_period_);
  cfl->GetValue("output-period", &output_period_);
  cfl->GetValue("include-variance", &include_variance_);
  if (cfl->HasUnusedValues())
    KALDI_ERR << "Could not process these elements in initializer: "
              << cfl->UnusedValues();
  if (!ok || input_dim_ <= 0 && input_period_ <= 0 || output_period_ <= 0 ||
      (output_period_ % input_period_ != 0))
    KALDI_ERR << "Invalid initializer for layer of type "
              << Type() << ": \"" << cfl->WholeLine() << "\"";
  Check();
}

void StatisticsExtractionComponent::Check() const {
  if (!(input_dim_ > 0 && input_period_ > 0 && output_period_ > 0 &&
        (output_period_ % input_period_) == 0))
    KALDI_ERR << "Invalid configuration of StatisticsExtractionComponent";
}

void StatisticsExtractionComponent::ReorderIndexes(
    std::vector<Index> *input_indexes,
    std::vector<Index> *output_indexes) const {
    std::sort(input_indexes->begin(), input_indexes->end(),
              IndexLessNxt());
    std::sort(output_indexes->begin(), output_indexes->end(),
              IndexLessNxt());
}

bool StatisticsExtractionComponent::IsComputable(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    const IndexSet &input_index_set,
    std::vector<Index> *used_inputs) const {
  Index input_index(output_index);
  int32 t = output_index.t,
      t_start = output_period_ * (t / output_period_);
  if (t_start > output_period_)   // could happen for negative t_start due to
    t_start -= output_period_;    // the way modulus works in c.
  int32 t_end = t_start + output_period_;
  if (!used_inputs) {
    for (int32 t = t_start; t < t_end; t += input_period_) {
      input_index.t = t;
      if (input_index_set(input_index))
        return true;
    }
    return false;
  } else {
    used_inputs->clear();
    bool ans = false;
    for (int32 t = t_start; t < t_end; t += input_period_) {
      input_index.t = t;
      if (input_index_set(input_index)) {
        ans = true;
        used_inputs->push_back(input_index);
      }
    }
    return ans;
  }
}

void StatisticsExtractionComponent::GetInputIndexes(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    std::vector<Index> *desired_indexes) const {
  desired_indexes->clear();
  Index input_index(output_index);
  int32 t = output_index.t,
      t_start = output_period_ * (t / output_period_);
  if (t_start > output_period_)   // could happen for negative t_start due to
    t_start -= output_period_;    // the way modulus works in c.
  for (int32 t = t_start; t < t_end; t += input_period_) {
    input_index.t = t;
    desired_indexes->push_back(input_index);
  }
}


void StatisticsExtractionComponent::Propagate(
    const ComponentPrecomputedIndexes *indexes_in,
    const CuMatrixBase<BaseFloat> &in,
    CuMatrixBase<BaseFloat> *out) const {
  KALDI_ASSERT(indexes_in != NULL);
  StatisticsExtractionComponentPrecomputedIndexes *indexes =
     dynamic_cast<StatisticsExtractionComponentPrecomputedIndexes*>(indexes_in);
  int32 num_rows_in = in.NumRows(), num_rows_out = out->NumRows();
  KALDI_ASSERT(indexes != NULL &&
               indexes->forward_indexes.Dim() == num_rows_out &&
               in.NumCols() == input_dim_ &&
               out->NumCols() == OutputDim());
  out->SetZero();
  // store the counts.
  out->CopyColFromVec(indexes->counts, 1);
  // store the mean stats
  out->ColRange(1, input_dim_).AddRowRanges(indexes->forward_indexes, in);
  if (include_variance_) {
    // store the variance (sum-squared) stats.
    CuMatrix<BaseFloat> in_squared(in);
    in_squared.ApplyPow(2.0);
    out->ColRange(input_dim_ + 1,
                  input_dim_).AddRowRanges(indexes->forward_indexes,
                                           in_squared);
  }
}

void StatisticsExtractionComponent::Backprop(
    const std::string &debug_info,
    const ComponentPrecomputedIndexes *indexes_in,
    const CuMatrixBase<BaseFloat> &in_value,
    const CuMatrixBase<BaseFloat> &, // out_value,
    const CuMatrixBase<BaseFloat> &out_deriv,
    Component *, // to_update,
    CuMatrixBase<BaseFloat> *in_deriv) const {
  KALDI_ASSERT(indexes_in != NULL);
  StatisticsExtractionComponentPrecomputedIndexes *indexes =
      dynamic_cast<StatisticsExtractionComponentPrecomputedIndexes*>(indexes_in);
  in_deriv->SetZero();
  in_deriv->AddRows(1.0, out_deriv.ColRange(1, input_dim_),
                    indexes->backward_indexes);
  if (include_variance_) {
    CuMatrix<BaseFloat> variance_deriv(in_value.NumRows(),
                                       in_value.NumCols(),
                                       kUndefined);
    variance_deriv.CopyRows(out_deriv,
                            indexes->backward_indexes);
    in_deriv->ColRange(1 + input_dim_, input_dim_).
        AddMatMatElements(2.0, variance_deriv, in_value);
  }
}

void StatisticsExtractionComponent::Read(std::istream &is, bool binary) {
  ExpectOneOrTwoTokens(is, binary, "<StatisticsExtractionComponent>",
                       "<InputDim>");
  ReadBasicType(is, binary, &input_dim_);
  ExpectToken(is, binary, "<InputPeriod>");
  ReadBasicType(is, binary, &input_period_);
  ExpectToken(is, binary, "<OutputPeriod>");
  ReadBasicType(is, binary, &output_period_);
  ExpectToken(is, binary, "<IncludeVarinance>");
  ReadBasicType(is, binary, &include_variance_);
  ExpectToken(is, binary, "</StatisticsExtractionComponent>");
  Check();
}

void StatisticsExtractionComponent::Write(std::ostream &os, bool binary) const {
  WriteToken(os, binary, "<StatisticsExtractionComponent>");
  WriteToken(os, binary, "<InputDim>");
  WriteBasicType(os, binary, input_dim_);
  WriteToken(os, binary, "<InputPeriod>");
  WriteBasicType(os, binary, input_period_);
  WriteToken(os, binary, "<OutputPeriod>");
  WriteBasicType(os, binary, output_period_);
  WriteToken(os, binary, "<IncludeVarinance>");
  WriteBasicType(os, binary, include_variance_);
  WriteToken(os, binary, "</StatisticsExtractionComponent>");
}


class StatisticsPoolingComponentPrecomputedIndexes:
      public ComponentPrecomputedIndexes {
 public:

  // in the first stage of creating the output we sum over row ranges of
  // the input.  forward_indexes.Dim() equals the number of rows of the
  // output, and each element is a (start, end) range of inputs, that is
  // summed over.
  CuArray<Int32Pair> forward_indexes;

  // backward_indexes contains the same information as forward_indexes, but in a
  // different format.  backward_indexes.Dim() is the same as the number of rows
  // of input, and each element contains the (start,end) of the range of outputs
  // for which this input index appears as an element of the sum for that
  // output.  This is possible because of the way the inputs and outputs are
  // ordered and because of how we select the elments to appear in the sum using
  // a window.  This quantity is used in backprop.
  CuArray<Int32Pair> backward_indexes;

  virtual ~StatisticsPoolingComponentPrecomputedIndexes() { }

  virtual void Write(std::ostream &os, bool binary) const {
    WriteToken(os, binary, "<StatisticsPoolingComponentPrecomputedIndexes>");
    WriteToken(os, binary, "<ForwardIndexes>");
    forward_indexes.Write(os, binary);
    WriteToken(os, binary, "<BackwardIndexes>");
    backward_indexes.Write(os, binary);
    WriteToken(os, binary, "</StatisticsPoolingComponentPrecomputedIndexes>");
  }
  virtual void Read(std::istream &is, bool binary) {
    ExpectOneOrTwoTokens(is, binary,
                         "<StatisticsPoolingComponentPrecomputedIndexes>",
                         "<ForwardIndexes>");
    forward_indexes.Read(is, binary);
    ExpectToken(is, binary, "<BackwardIndexes>");
    backward_indexes.Read(is, binary);
    ExpectToken(is, binary, "</StatisticsPoolingComponentPrecomputedIndexes>");
  }
};


void StatisticsPoolingComponent::InitFromConfig(ConfigLine *cfl) {
  bool ok = cfl->GetValue("input-dim", &input_dim_);
  cfl->GetValue("input-period", &input_period_);
  cfl->GetValue("output-period", &output_period_);
  cfl->GetValue("left-context", &left_context_);
  cfl->GetValue("log-count-features", &log_count_featues_);
  cfl->GetValue("output-stddevs", &output_stddev_);
  cfl->GetValue("variance-floor", &variance_floor_);

  if (cfl->HasUnusedValues())
    KALDI_ERR << "Could not process these elements in initializer: "
	      << cfl->UnusedValues();
  // do some basic checks here but Check() will check more completely.
  if (!ok || input_dim_ <= 0 || left_context_ + right_context_ <= 0 ||
      log_count_features_ < 0)
    KALDI_ERR << "Invalid initializer for layer of type "
              << Type() << ": \"" << cfl->WholeLine() << "\"";
  Check();
}

StatisticsPoolingComponent::StatisticsPoolingComponent():
    input_dim_(-1), input_period_(1), output_period_(1),
    left_context_(-1), right_context_(-1), log_count_features_(0),
    output_stddevs_(false) { }

StatisticsPoolingComponent::StatisticsPoolingComponent(
    const StatisticsPoolingComponent &other):
    input_dim_(other.input_dim_), input_period_(other.input_period_),
    output_period_(other.output_period_), left_context_(other.left_context_),
    right_context_(other.right_context_),
    log_count_features_(other.log_count_features_),
    output_stddevs_(other.output_stddevs_) {
  Check();
}

void StatisticsPoolingComponent::Check() const {
  KALDI_ASSERT(input_dim_ > 0);
  KALDI_ASSERT(input_period_ > 0 && output_period_ > 0 &&
               output_period_ % input_period_ == 0);
  KALDI_ASSERT(left_context_ >= 0 && right_context_ >= 0 &&
               left_context_ + right_context_ > 0);
  KALDI_ASSERT(left_context_ % input_period_ == 0 &&
               right_context_ % input_period_ == 0);
  KALDI_ASSERT(variance_floor_ >= 0.0 && variance_floor_ < 1.0);
  KALDI_ASSERT(!output_stddev_ || (input_dim_ - 1) % 2 == 0);
}

void StatisticsPoolingComponent::Read(std::istream &is, bool binary) {
  ExpectOneOrTwoTokens(is, binary, "<StatisticsPoolingComponent>",
                       "<InputDim>");
  ReadBasicType(os, binary, &input_dim_);
  ExpectToken(os, binary, "<InputPeriod>");
  ReadBasicType(os, binary, &input_period_);
  ExpectToken(os, binary, "<OutputPeriod>");
  ReadBasicType(os, binary, &output_period_);
  ExpectToken(os, binary, "<LeftContext>");
  ReadBasicType(os, binary, &left_context_);
  ExpectToken(os, binary, "<RightContext>");
  ReadBasicType(os, binary, &right_context_);
  ExpectToken(os, binary, "<LogCountFeatures>");
  ReadBasicType(os, binary, &log_count_features_);
  ExpectToken(os, binary, "<OutputStddevs>");
  ReadBasicType(os, binary, &output_stddevs_);
  ExpectToken(os, binary, "<VarianceFloor>");
  ReadBasicType(os, binary, &variance_floor_);
  ExpectToken(os, binary, "</StatisticsPoolingComponent>");
  Check();
}

void StatisticsPoolingComponent::Write(std::ostream &os, bool binary) const {
  WriteToken(os, binary, "<StatisticsPoolingComponent>");
  WriteToken(os, binary, "<InputDim>");
  WriteBasicType(os, binary, input_dim_);
  WriteToken(os, binary, "<InputPeriod>");
  WriteBasicType(os, binary, input_period_);
  WriteToken(os, binary, "<OutputPeriod>");
  WriteBasicType(os, binary, output_period_);
  WriteToken(os, binary, "<LeftContext>");
  WriteBasicType(os, binary, left_context_);
  WriteToken(os, binary, "<RightContext>");
  WriteBasicType(os, binary, right_context_);
  WriteToken(os, binary, "<LogCountFeatures>");
  WriteBasicType(os, binary, log_count_features_);
  WriteToken(os, binary, "<OutputStddevs>");
  WriteBasicType(os, binary, output_stddevs_);
  WriteToken(os, binary, "<VarianceFloor>");
  WriteBasicType(os, binary, variance_floor_);
  WriteToken(os, binary, "</StatisticsPoolingComponent>");
}

void StatisticsPoolingComponent::ReorderIndexes(
    std::vector<Index> *input_indexes,
    std::vector<Index> *output_indexes) const {
    std::sort(input_indexes->begin(), input_indexes->end(),
              IndexLessNxt());
    std::sort(output_indexes->begin(), output_indexes->end(),
              IndexLessNxt());
}

void StatisticsPoolingComponent::GetInputIndexes(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    std::vector<Index> *desired_indexes) const {
  desired_indexes->clear();
  Index input_index(output_index);
  int32 middle_t = output_index.t,
      t_start = middle_t - left_context_,
      t_last = middle_t + right_context_;
  KALDI_ASSERT(middle_t % input_period_ == 0);
  for (int32 t = t_start; t <= t_last; t += input_period_) {
    input_index.t = t;
    desired_indexes->push_back(input_index);
  }
}

bool StatisticsPoolingComponent::IsComputable(
    const MiscComputationInfo &misc_info,
    const Index &output_index,
    const IndexSet &input_index_set,
    std::vector<Index> *used_inputs) const {
  if (used_inputs)
    used_inputs->clear();
  // you are not supposed to access the output of this component other than at
  // multiples of the output period.  We could make this an error but decided to
  // just have it return false.
  if (output_index.t % output_period_ != 0)
    return false;

  Index input_index(output_index);
  int32 output_t = output_index.t,
      t_start = output_t - left_context_,
      t_last = output_t + right_context_;
  if (!used_inputs) {
    for (int32 t = t_start; t <= t_last; t += input_period_) {
      input_index.t = t;
      if (input_index_set(input_index))
        return true;
    }
    return false;
  } else {
    bool ans = false;
    for (int32 t = t_start; t <= t_last; t += input_period_) {
      input_index.t = t;
      if (input_index_set(input_index)) {
        ans = true;
        used_inputs->push_back(input_index);
      }
    }
    return ans;
  }
}

virtual ComponentPrecomputedIndexes*
StatisticsPoolingComponent::PrecomputeIndexes(
    const MiscComputationInfo &misc_info,
    const std::vector<Index> &input_indexes,
    const std::vector<Index> &output_indexes,
    bool need_backprop) const {
  int32 num_input_indexes = input_indexes.size(),
      num_output_indexes = output_indexes.size();
  StatisticsPoolingComponentPrecomputedIndexes *ans = new
      StatisticsPoolingComponentPrecomputedIndexes();

  Int32Pair invalid_pair;
  invalid_pair.first = -1;
  invalid_pair.second = -1;
  // forward_indexes_cpu[i] will be the (begin, end) of input indexes
  // included in the sum for the i'th output index.
  std::vector<Int32Pair> forward_indexes_cpu(num_output_indexes,
                                             invalid_pair);
  // backward_indexes_cpu[i] will be the (begin, end) of output indexes
  // for which the i'th input index participates in the sum.
  // because of the way the indexes are sorted (and the fact that only
  // required indexes are present at the input), it naturally has this
  // structure [i.e. no gaps in the sets of indexes].
  std::vector<Int32Pair> backward_indexes_cpu(num_input_indexes,
                                              invalid_pair);

  // this map maps from Index to the position in 'input_indexes'.
  std::unordered_map<Index, int32> index_to_input_pos;
  for (i = 0; i < num_input_indexes; i++)
    index_to_input_pos[input_indexes[i]] = i;

  for (int32 i = 0; i < num_output_indexes; i++) {
    Index input_index(output_indexes[i]);
    int32 middle_t = input_index.t,
        t_start = middle_t - left_context,
        t_last = middle_t + right_context;
    for (int32 t = t_start; t <= t_last; t += input_period_) {
      input_index.t = t;
      std::unordered_map<Index, int32>::iterator iter =
          index_to_input_pos.find(input_index);
      if (iter != index_to_input_pos.end()) {
        int32 input_pos = iter->second;
        if (forward_indexes_cpu[i].first == -1) {
          forward_indexes_cpu[i].first = input_pos;
          forward_indexes_cpu[i].second = input_pos + 1;
        } else {
          KALDI_ASSERT(forward_indexes_cpu[i].second == input_pos);
          forward_indexes_cpu[i].second++;
        }
        if (backward_indexes_cpu[i].first == -1) {
          backward_indexes_cpu[i].first = i;
          backward_indexes_cpu[i].second = i + 1;
        } else {
          KALDI_ASSERT(backward_indexes_cpu[i].second == i);
          backward_indexes_cpu[i].second++;
        }
      }
    }
    KALDI_ASSERT(forward_indexes_cpu[i].first != -1);
  }
  for (int32 i = 0; i < num_input_indexes; i++) {
    KALDI_ASSERT(backward_indexes_cpu[i].first != -1);
  }

  ans->forward_indexes = forward_indexes_cpu;
  if (need_backprop)
    ans->backward_indexes = backward_indexes_cpu;
  return ans;
}

void StatisticsPoolingComponent::Propagate(
    const ComponentPrecomputedIndexes *indexes_in,
    const CuMatrixBase<BaseFloat> &in,
    CuMatrixBase<BaseFloat> *out) const {
  KALDI_ASSERT(indexes_in != NULL);
  StatisticsPoolingComponentPrecomputedIndexes *indexes =
      dynamic_cast<StatisticsPoolingComponentPrecomputedIndexes*>(indexes_in);
  int32 num_rows_in = in.NumRows(), num_rows_out = out->NumRows();
  KALDI_ASSERT(indexes != NULL &&
               indexes->forward_indexes.Dim() == num_rows_out &&
               in.NumCols() == input_dim_ &&
               out->NumCols() == OutputDim());
  CuVector<BaseFloat> counts(num_rows_out);
  // counts_mat is a fake matrix with one column, containing the counts.
  CuSubMatrix<BaseFloat> counts_mat(counts.Data(), num_rows_out, 1, 1);
  counts_mat.AddRowRanges(in.ColRange(0, 1), indexes->forward_indexes);

  CuSubMatrix<BaseFloat> out_non_count(*out, 0, num_rows_out,
                                       log_count_features_, input_dim_ - 1);
  out_non_count.SetZero();
  out_non_count.SetZero();
  out_non_count.AddRowRanges(in.ColRange(1, input_dim_ - 1),
                             indexes->forward_indexes);
  out_non_count.DivRowsVec(counts);

  if (log_count_features_ > 0) {
    counts.ApplyLog();
    out->ColRange(0, log_count_features_).CopyColsFromVec(counts);
  }

  if (output_stddevs_) {
    // if this is true, then we assume the input contains x^2 stats as well as x
    // stats, and we want to process them into a standard deviation.
    KALDI_ASSERT((input_dim_ - 1) % 2 == 0);
    int32 feature_dim = (input_dim_ - 1) / 2;
    CuSubMatrix<BaseFloat> mean(*out, 0, num_rows_out,
                                log_count_features_, feature_dim),
        variance(*out, 0, num_rows_out,
                 log_count_features_ + feature_dim, feature_dim);
    // subtract mean-squared from average of x^2 to get the variance.
    variance.AddMatMatElements(-1.0, mean, mean, 1.0);
    variance.ApplyFloor(variance_floor_);
    // compute the standard deviation via square root.
    variance.ApplyPow(0.5);
  }
}

void StatisticsPoolingComponent::Backprop(
    const std::string &debug_info,
    const ComponentPrecomputedIndexes *indexes,
    const CuMatrixBase<BaseFloat> &in_value,
    const CuMatrixBase<BaseFloat> &out_value,
    const CuMatrixBase<BaseFloat> &out_deriv_in,
    Component *, // to_update,
    CuMatrixBase<BaseFloat> *in_deriv) const {
  KALDI_ASSERT(indexes_in != NULL);
  StatisticsPoolingComponentPrecomputedIndexes *indexes =
      dynamic_cast<StatisticsPoolingComponentPrecomputedIndexes*>(indexes_in);
  int32 num_rows_in = in_deriv->NumRows(),
      num_rows_out = out_deriv_in.NumRows();
  CuMatrix<BaseFloat> out_deriv(out_deriv_in);
  if (output_stddevs_) {
    // for now we actually ignore the covariance flooring in the backprop- this
    // is an approximation.  Typically the derivatives computed will be quite
    // tiny for floored variances (they should be zero), so it won't affect the
    // derivatives much.
    int32 feature_dim = (input_dim_ - 1) / 2;
    CuSubMatrix<BaseFloat> mean_deriv(out_deriv, 0, num_rows_out,
                                      log_count_features_, feature_dim),
        variance_deriv(out_deriv, 0, num_rows_out,
                       log_count_features_ + feature_dim, feature_dim),
        mean_value(out, 0, num_rows_out,
                   log_count_features_, feature_dim),
        stddev_value(out, 0, num_rows_out,
                     log_count_features_ + feature_dim, feature_dim);
    // we currently have the deriv w.r.t. the stddev.  step 1 is to get it
    // w.r.t. the centered variance.  If the centered variance is s,
    // and the stddev is sqrt(s), then d/ds sqrt(s) = 0.5 / sqrt(s),
    // so we need to multiply variance_deriv by 0.5 / the stddev.
    variance_deriv.DivElements(stddev_value);
    variance_deriv.Scale(0.5);

    // the deriv w.r.t. the uncentered variance is the same as w.r.t.  the
    // uncentered variance (since they difer by a constant term of -(mean *
    // mean), but we need to add to dF/dmean, the value -2.0 * mean *
    // dF/dvariance.
    mean_deriv.AddMatMatElements(-2.0, mean_value, variance_deriv, 1.0);
  }
  // now we have to account for the effect of division by the count, on
  // the derivative.
  Vector<BaseFloat> counts(num_rows_out, kUndefined);
  if (log_count_features_ > 0) {
    counts.CopyRowFromMat(out_value, 0);
    counts.ApplyExp();
  } else {
    counts.SetZero();
    // we need to recompute the counts from the input since they are not in the
    // output.  The submatrix initializer below takes num-rows, num-cols,
    // stride;  num-cols and stride are 1.
    SubMatrix<BaseFloat> counts_mat(counts.Data(), num_rows_out, 1, 1);
    counts_mat.AddRowRanges(in_value, indexes->forward_indexes);
  }
  // Divide the output derivative by the counts.  This is what we want as it
  // concerns the mean and x^2 stats.  As for the counts themselves, the
  // derivative will end up being discarded when we backprop to the
  // StatisticsExtractionComponent (as the count is not differentiable) so it
  // doesn't really matter.
  out_deriv.DivRowsVec(counts);

  // Now propagate the derivative back to the input.  we don't bother excluding
  // the count's row (the first row of the input), to keep the code simple,
  // although this row of the derivative is not valid, since it will be ignored
  // in the StatisticsExtractionComponent's backprop.
  in_deriv->AddRowRanges(indexes->backward_indexes, out_deriv);
}

} // namespace nnet3
} // namespace kaldi
