// nnet3/nnet-compute.cc

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
#include "nnet3/nnet-compute.h"

namespace kaldi {
namespace nnet3 {


NnetComputer::NnetComputer(const NnetComputeOptions &options,
                           const NnetComputation &computation,
                           const Nnet &nnet,
                           Nnet *nnet_to_update):
    options_(options), computation_(computation), nnet_(nnet),
    nnet_to_update_(nnet_to_update) {
  KALDI_ASSERT(computation.indexes_cuda.size() == computation.indexes.size() &&
 computation.indexes_ranges_cuda.size() == computation.indexes_ranges.size() &&
               "You must call NnetComputation::ComputeCudaIndexes() before "
               "executing the computation.");
  matrices_.resize(computation.matrices.size());
  if (options_.debug) {
    ComputationVariables variables;
    variables.Init(computation);
    ComputeCommandAttributes(nnet, computation, variables,
                             &command_attributes_);
    std::string preamble;
    computation.GetCommandStrings(nnet, &preamble, &command_strings_);
    KALDI_LOG << preamble;
  }
}

void NnetComputer::DebugBeforeExecute(int32 command,
                                      CommandDebugInfo *info) const {
  const std::vector<int32> &matrices_written =
      command_attributes_[command].matrices_written;
  size_t size = matrices_written.size();
  info->matrices_written_sums.resize(size);
  for (size_t i = 0; i < size; i++) {
    int32 m = matrices_written[i];
    info->matrices_written_sums[i] = matrices_[m].Sum();
  }
}


void NnetComputer::DebugAfterExecute(int32 command,
                                     const CommandDebugInfo &info) const {
  std::ostringstream os;
  os << command_strings_[command] << "\t|\t";
  const std::vector<int32> &matrices_written =
      command_attributes_[command].matrices_written;
  size_t size = matrices_written.size();
  KALDI_ASSERT(info.matrices_written_sums.size() == size);
  for (size_t i = 0; i < size; i++) {
    int32 m = matrices_written[i];
    BaseFloat old_sum = info.matrices_written_sums[i],
                  sum = matrices_[m].Sum();
    os << 'm' << m << ": " << old_sum << "->" << sum << " ";
  }
  KALDI_LOG << os.str();
}


void NnetComputer::ExecuteCommand(int32 command) {
  const NnetComputation::Command &c = computation_.commands[command];
  switch (c.command_type) {
    case NnetComputation::kAllocMatrixZeroed:
      matrices_[c.arg1].Resize(computation_.matrices[c.arg1].num_rows,
                               computation_.matrices[c.arg1].num_cols,
                               kSetZero);
      break;
    case NnetComputation::kAllocMatrixUndefined:
      matrices_[c.arg1].Resize(computation_.matrices[c.arg1].num_rows,
                               computation_.matrices[c.arg1].num_cols,
                               kUndefined);
      break;
    case NnetComputation::kDeallocMatrix:
      matrices_[c.arg1].Resize(0, 0);
      break;
    case NnetComputation::kPropagate: {
      const Component *component = nnet_.GetComponent(c.arg1);
      ComponentPrecomputedIndexes *indexes =
          computation_.component_precomputed_indexes[c.arg2];
      const CuSubMatrix<BaseFloat> input(GetSubMatrix(c.arg3));
      CuSubMatrix<BaseFloat> output(GetSubMatrix(c.arg4));
      component->Propagate(indexes, input, &output);
      break;
    }
    case NnetComputation::kStoreStats: {
      KALDI_ASSERT(nnet_to_update_ != NULL);
      Component *upd_component = nnet_to_update_->GetComponent(c.arg1);
      CuSubMatrix<BaseFloat> output(GetSubMatrix(c.arg2));
      upd_component->StoreStats(output);
      break;
    }
    case NnetComputation::kBackprop: {
      int32 node_index = c.arg1;
      std::ostringstream debug_str;
      KALDI_ASSERT(nnet_to_update_ != NULL);      
      debug_str << "node " << node_index << '['
                << nnet_.GetNodeNames()[node_index] << ']';
      const Component *component = nnet_.GetComponentForNode(c.arg1);
      KALDI_ASSERT(!(computation_.need_model_derivative && !nnet_to_update_));
      Component *upd_component = (nnet_to_update_ &&
                                  computation_.need_model_derivative ?
                                  nnet_to_update_->GetComponentForNode(c.arg1) :
                                  NULL);
      ComponentPrecomputedIndexes *indexes =
          computation_.component_precomputed_indexes[c.arg2];
      const CuSubMatrix<BaseFloat> in_value(GetSubMatrix(c.arg3));
      const CuSubMatrix<BaseFloat> out_value(GetSubMatrix(c.arg4));
      const CuSubMatrix<BaseFloat> out_deriv(GetSubMatrix(c.arg5));
      CuSubMatrix<BaseFloat> in_deriv(GetSubMatrix(c.arg6));
      component->Backprop(debug_str.str(), indexes,
                          in_value, out_value, out_deriv, upd_component,
                          c.arg6 == 0 ? NULL : &in_deriv);
      break;
    }
    case NnetComputation::kMatrixCopy: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));      
      const CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg2));
      dest.CopyFromMat(src);
      break;
    }
    case NnetComputation::kMatrixAdd: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      const CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg2));
      dest.AddMat(1.0, src);
      break;
    }
    case NnetComputation::kAddRows: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      const CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg2));
      const CuArray<int32> &indexes = computation_.indexes_cuda[c.arg3];
      dest.AddRows(1.0, src, indexes);
      break;
    }
    case NnetComputation::kCopyRows: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      const CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg2));
      const CuArray<int32> &indexes = computation_.indexes_cuda[c.arg3];
      dest.CopyRows(src, indexes);
      break;
    }
    case NnetComputation::kCopyRowsMulti: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      CuArray<const BaseFloat*> pointers;
      GetPointers(c.arg2, dest.NumCols(), &pointers);
      dest.CopyRows(pointers);
      break;
    }
    case NnetComputation::kCopyToRowsMulti: {
      CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg1));
      CuArray<BaseFloat*> pointers;
      GetPointers(c.arg2, src.NumCols(), &pointers);
      src.CopyToRows(pointers);
      break;
    }
    case NnetComputation::kAddRowsMulti: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      CuArray<const BaseFloat*> pointers;
      GetPointers(c.arg2, dest.NumCols(), &pointers);
      dest.AddRows(1.0, pointers);
      break;
    }
    case NnetComputation::kAddToRowsMulti: {
      CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg1));
      CuArray<BaseFloat*> pointers;
      GetPointers(c.arg2, src.NumCols(), &pointers);
      src.AddToRows(1.0, pointers);
      break;
    }
    case NnetComputation::kAddRowRanges: {
      CuSubMatrix<BaseFloat> dest(GetSubMatrix(c.arg1));
      const CuSubMatrix<BaseFloat> src(GetSubMatrix(c.arg2));
      const CuArray<Int32Pair> &pairs = computation_.indexes_ranges_cuda[c.arg3];
      dest.AddRowRanges(src, pairs);
      break;
    }
    case NnetComputation::kNoOperation: case NnetComputation::kNoOperationMarker:
      break;
    default:
      KALDI_ERR << "Invalid command in computation";
  }
}

CuSubMatrix<BaseFloat> NnetComputer::GetSubMatrix(int32 submatrix_index) {
  KALDI_PARANOID_ASSERT(static_cast<size_t>(submatrix_index) <
                        computation_.submatrices.size());
  const NnetComputation::SubMatrixInfo &info =
      computation_.submatrices[submatrix_index];
  const CuMatrix<BaseFloat> &mat = matrices_[info.matrix_index];
  return CuSubMatrix<BaseFloat>(
      mat, info.row_offset, info.num_rows, info.col_offset, info.num_cols);
}

void NnetComputer::GetPointers(int32 indexes_multi_index,
                               int32 num_cols,
                               CuArray<BaseFloat*> *pointers) {
  KALDI_ASSERT(static_cast<size_t>(indexes_multi_index)
               < computation_.indexes_multi.size());
  const std::vector<std::pair<int32,int32> > &pairs =
      computation_.indexes_multi[indexes_multi_index];
  int32 size = pairs.size();
  std::vector<BaseFloat*> vec(size);

  // the map "lookup" maps from submatrix index to the Data()
  // pointer of that submatrix, and the corresponding Stride().
  unordered_map<int32, std::pair<BaseFloat*, int32> > lookup;
  
  for (int32 i = 0; i < size; i++) {
    int32 submatrix_index = pairs[i].first, row = pairs[i].second;
    unordered_map<int32, std::pair<BaseFloat*, int32> >::iterator
        iter = lookup.find(submatrix_index);
    if (iter == lookup.end()) {
      CuSubMatrix<BaseFloat> m = GetSubMatrix(submatrix_index);
      lookup[submatrix_index] = std::pair<BaseFloat*, int32>(m.Data(),
                                                             m.Stride());
      iter = lookup.find(submatrix_index);
    }
    BaseFloat *data = iter->second.first;
    int32 stride = iter->second.second;
    vec[i] = data + (row * stride);
  }
#ifdef KALDI_PARANOID
  for (int32 i = 0; i < size; i += 30 + RandInt(0, 9)) {
    // Do a pseudo-random spot check that the row-indexes are not out of range.
    int32 submatrix_index = pairs[i].first, row = pairs[i].second;
    CuSubMatrix<BaseFloat> m = GetSubMatrix(submatrix_index);
    KALDI_ASSERT(row >= 0 && row < m.NumRows() && num_cols == m.NumCols());
  }
#endif  
  pointers->CopyFromVec(vec);
}

void NnetComputer::GetPointers(int32 indexes_multi_index,
                               int32 num_cols,
                               CuArray<const BaseFloat*> *pointers) {
  GetPointers(indexes_multi_index, num_cols,
              reinterpret_cast<CuArray<BaseFloat*>*>(pointers));
}

void NnetComputer::Forward() {
  CheckInputs(false);
  int32 size = computation_.commands.size(), i = 0;
  const std::vector<NnetComputation::Command> &c = computation_.commands;
  CommandDebugInfo info;

  for (; i < size && c[i].command_type != NnetComputation::kNoOperationMarker;
       i++) {
    if (options_.debug)
      DebugBeforeExecute(i, &info);
    ExecuteCommand(i);
    if (options_.debug)
      DebugAfterExecute(i, info);
  }
    
}


void NnetComputer::Backward() {
  CheckInputs(true);
  int32 size = computation_.commands.size(), i = 0;
  const std::vector<NnetComputation::Command> &c = computation_.commands;
  for (; i < size && c[i].command_type != NnetComputation::kNoOperationMarker;
       i++);
  CommandDebugInfo info;
  for (; i < size; i++) {
    if (options_.debug)
      DebugBeforeExecute(i, &info);
    ExecuteCommand(i);
    if (options_.debug)
      DebugAfterExecute(i, info);
  }
}

void NnetComputer::AcceptInput(const std::string &input_name,
                             CuMatrix<BaseFloat> *input) {
  bool is_output = false, is_deriv = false;
  int32 matrix_index = GetMatrixIndex(input_name, is_output, is_deriv);

  KALDI_ASSERT(static_cast<size_t>(matrix_index) < matrices_.size());
  if (input->NumRows() != computation_.matrices[matrix_index].num_rows)
    KALDI_ERR << "Num-rows mismatch for input '" << input_name
              << "': " << computation_.matrices[matrix_index].num_rows
              <<  " in computation-request, " << input->NumRows()
              << " provided.";
  if (input->NumCols() != computation_.matrices[matrix_index].num_cols)
    KALDI_ERR << "Num-cols mismatch for input '" << input_name
              << "': " << computation_.matrices[matrix_index].num_cols
              <<  " in computation-request, " << input->NumCols()
              << " provided.";
  matrices_[matrix_index].Swap(input);
  input->Resize(0, 0);
}

const CuMatrixBase<BaseFloat> &NnetComputer::GetInputDeriv(
    const std::string &input_name) const {
  bool is_output = false, is_deriv = true;
  int32 matrix_index = GetMatrixIndex(input_name, is_output, is_deriv);
  if (matrices_[matrix_index].NumRows() == 0)
    KALDI_ERR << "GetInputDeriv called before it is ready (before Backward()?)";
  return matrices_[matrix_index];
}


const CuMatrixBase<BaseFloat> &NnetComputer::GetOutput(
    const std::string &output_name) const {
  bool is_output = true, is_deriv = false;
  int32 matrix_index = GetMatrixIndex(output_name, is_output, is_deriv);
  if (matrices_[matrix_index].NumRows() == 0)
    KALDI_ERR << "GetOutput called when output not ready (before Forward()?)";
  return matrices_[matrix_index];
}

void NnetComputer::AcceptOutputDeriv(const std::string &output_name,
                                     CuMatrix<BaseFloat> *output_deriv) {
  bool is_output = true, is_deriv = true;
  int32 matrix_index = GetMatrixIndex(output_name, is_output, is_deriv);
  KALDI_ASSERT(static_cast<size_t>(matrix_index) < matrices_.size());
  if (output_deriv->NumRows() != computation_.matrices[matrix_index].num_rows)
    KALDI_ERR << "Num-rows mismatch for output-deriv '" << output_name
              << "': " << computation_.matrices[matrix_index].num_rows
              <<  " in computation-request, " << output_deriv->NumRows()
              << " provided.";
  if (output_deriv->NumCols() != computation_.matrices[matrix_index].num_cols)
    KALDI_ERR << "Num-cols mismatch for output_deriv '" << output_name
              << "': " << computation_.matrices[matrix_index].num_cols
              <<  " in computation-request, " << output_deriv->NumCols()
              << " provided.";
  matrices_[matrix_index].Swap(output_deriv);
  output_deriv->Resize(0, 0);
}

int32 NnetComputer::GetMatrixIndex(
    const std::string &node_name, bool expect_output, bool is_deriv) const {
  int32 node_index = nnet_.GetNodeIndex(node_name);
  if (node_index == -1)
    KALDI_ERR << "No node named '" << node_name << "'in network.";
  if (expect_output) {
    if (!nnet_.IsOutputNode(node_index))
      KALDI_ERR << "Expecting output node; node named '"
                << node_name  << "' is not output node.";
  } else {
    if (nnet_.IsOutputNode(node_index))
      KALDI_ERR << "Expecting input node or component node; node named '"
                << node_name  << "' is output node.";
  }
  unordered_map<int32, std::pair<int32, int32> >::const_iterator
      iter = computation_.input_output_info.find(node_index),
      end = computation_.input_output_info.end();
  if (iter == end)
    KALDI_ERR << "Not expecting input or output for node named '" << node_name
              << "' (not in computation request)";
  std::pair<int32,int32> locations = iter->second;
  int32 location;
  if (is_deriv) {
    location = locations.second;
    if (locations.second <= 0) // No deriv expected.
      KALDI_ERR << "Not expecting derivative information for node named '"
                << node_name << "' (not in computation request)";
  } else {
    location = locations.first;
  }
  KALDI_ASSERT(static_cast<size_t>(location) < matrices_.size());
  return location;
}

void NnetComputer::CheckInputs(bool check_output_deriv) const {
  unordered_map<int32, std::pair<int32, int32> >::const_iterator
      iter = computation_.input_output_info.begin(),
      end = computation_.input_output_info.end();
  for (; iter != end; ++iter) {
    int32 node_index = iter->first,
      value_matrix_index = iter->second.first,
      deriv_matrix_index = iter->second.second;
    std::string name = nnet_.GetNodeName(node_index);
    if (nnet_.IsOutputNode(node_index)) {
      if (check_output_deriv && deriv_matrix_index > 0) {
        KALDI_ASSERT(static_cast<size_t>(deriv_matrix_index) < matrices_.size());
        if (matrices_[deriv_matrix_index].NumRows() == 0)
          KALDI_ERR << "Output-derivative required but not provided for node '"
                    << name << "'.";
      }
    } else {
      if (!check_output_deriv) {
        if (matrices_[value_matrix_index].NumRows() == 0)
          KALDI_ERR << "Input required but not provided for node '"
                    << name << "'.";
      }
    }
  }
}

void NnetComputer::AcceptInputs(const Nnet &nnet,
                                const NnetExample &example) {
  for (size_t i = 0; i < example.io.size(); i++) {
    const NnetIo &io = example.io[i];
    int32 node_index = nnet.GetNodeIndex(io.name);
    if (node_index == -1)
      KALDI_ERR << "No node named '" << io.name << "' in nnet.";
    if (nnet.IsInputNode(node_index)) {
      CuMatrix<BaseFloat> cu_input(io.features.NumRows(),
                                   io.features.NumCols(),
                                   kUndefined);
      cu_input.CopyFromGeneralMat(io.features);
      this->AcceptInput(io.name, &cu_input);
    }
  }
}

} // namespace nnet3
} // namespace kaldi
