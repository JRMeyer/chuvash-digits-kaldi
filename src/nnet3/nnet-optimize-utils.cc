// nnet3/nnet-optimize-utils.cc

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

#include <map>
#include "nnet3/nnet-optimize-utils.h"
#include "nnet3/nnet-optimize.h"


namespace kaldi {
namespace nnet3 {


void IdentifySubmatrixArgs(NnetComputation::Command *c,
                           std::vector<int32*> *submatrix_args) {
  submatrix_args->clear();
  switch (c->command_type) {
    case kAllocMatrixZeroed:
    case kAllocMatrixUndefined:
    case kDeallocMatrix:
    case kAllocMatrixFromOther:
    case kAllocMatrixFromOtherZeroed:
      break;
    case kPropagate:
      submatrix_args->push_back(&c->arg3);
      submatrix_args->push_back(&c->arg4);
      break;
    case kStoreStats:
      submatrix_args->push_back(&c->arg2);
      break;
    case kBackprop:
    case kBackpropNoModelUpdate:
      submatrix_args->push_back(&c->arg3);
      submatrix_args->push_back(&c->arg4);
      submatrix_args->push_back(&c->arg5);
      submatrix_args->push_back(&c->arg6);
      break;
    case kMatrixCopy:
    case kMatrixAdd:
    case kAddRows:
    case kCopyRows:
    case kAddRowRanges:
      submatrix_args->push_back(&c->arg1);
      submatrix_args->push_back(&c->arg2);
      break;
    case kAddRowsMulti:
    case kCopyRowsMulti:
    case kAddToRowsMulti:
    case kCopyToRowsMulti:
      submatrix_args->push_back(&c->arg1);
      break;
    case kNoOperation:
    case kNoOperationMarker:
      break;
    default:
      KALDI_ERR << "Unknown command type.";
  }
}

void IdentifySubmatrixArgs(std::vector<NnetComputation::Command> *commands,
                           std::vector<int32*> *submatrix_args) {
  submatrix_args->clear();
  std::vector<NnetComputation::Command>::iterator iter = commands->begin(),
      end = commands->end();
  std::vector<int32*> this_submatrix_args;
  for (; iter != end; ++iter) {
    IdentifySubmatrixArgs(&(*iter), &this_submatrix_args);
    submatrix_args->insert(submatrix_args->end(),
                           this_submatrix_args.begin(),
                           this_submatrix_args.end());
  }
}


void IdentifyMatrixArgs(std::vector<NnetComputation::Command> *commands,
                        std::vector<int32*> *matrix_args) {
  matrix_args->clear();
  std::vector<NnetComputation::Command>::iterator iter = commands->begin(),
      end = commands->end();
  std::vector<int32*> this_matrix_args;
  for (; iter != end; ++iter) {
    IdentifyMatrixArgs(&(*iter), &this_matrix_args);
    matrix_args->insert(matrix_args->end(),
                        this_matrix_args.begin(),
                        this_matrix_args.end());
  }
}


void IdentifyMatrixArgsInComputation(NnetComputation *computation,
                                     std::vector<int32*> *matrix_args) {
  IdentifyMatrixArgs(&(computation->commands), matrix_args);
  int32 num_submatrices = computation->submatrices.size();
  matrix_args->reserve(matrix_args->size() + computation->submatrices.size() +
                       2 * computation->input_output_info.size());
  for (int32 s = 1; s < num_submatrices; s++)
    matrix_args->push_back(&(computation->submatrices[s].matrix_index));
  unordered_map<int32, std::pair<int32, int32> >::iterator
      iter = computation->input_output_info.begin(),
      end = computation->input_output_info.end();
  for (; iter != end; ++iter) {
    matrix_args->push_back(&(iter->second.first));
    matrix_args->push_back(&(iter->second.second));
  }
}


void IdentifyIndexesMultiArgs(std::vector<NnetComputation::Command> *commands,
                              std::vector<int32*> *indexes_multi_args) {
  indexes_multi_args->clear();
  std::vector<NnetComputation::Command>::iterator iter = commands->begin(),
      end = commands->end();
  for (; iter != end; ++iter) {
    NnetComputation::Command &command = *iter;
    if (command.command_type == kAddRowsMulti ||
        command.command_type == kAddToRowsMulti ||
        command.command_type == kCopyRowsMulti ||
        command.command_type == kCopyToRowsMulti)
      indexes_multi_args->push_back(&(command.arg2));
  }
}


void IdentifyIndexesArgs(std::vector<NnetComputation::Command> *commands,
                         std::vector<int32*> *indexes_args) {
  indexes_args->clear();
  std::vector<NnetComputation::Command>::iterator iter = commands->begin(),
      end = commands->end();
  for (; iter != end; ++iter) {
    NnetComputation::Command &command = *iter;
    if (command.command_type == kCopyRows ||
        command.command_type == kAddRows)
      indexes_args->push_back(&(command.arg3));
  }
}



void IdentifyMatrixArgs(NnetComputation::Command *c,
                        std::vector<int32*> *matrix_args) {
  matrix_args->clear();
  switch (c->command_type) {
    case kAllocMatrixZeroed:
    case kAllocMatrixUndefined:
    case kDeallocMatrix:
      matrix_args->push_back(&c->arg1);
      break;
    case kAllocMatrixFromOther:
    case kAllocMatrixFromOtherZeroed:
      matrix_args->push_back(&c->arg1);
      matrix_args->push_back(&c->arg2);
      break;
    default:
      break;
  }
}

// static
int32 ComputationRenumberer::CreateRenumbering(
    const std::vector<bool> &used,
    std::vector<int32> *renumbering) {
  renumbering->clear();
  renumbering->reserve(used.size());
  std::vector<bool>::const_iterator iter = used.begin(), end = used.end();
  int32 cur_index = 0;
  for (; iter != end; ++iter) {
    if (*iter) renumbering->push_back(cur_index++);
    else renumbering->push_back(-1);
  }
  return cur_index;
}

// static
void ComputationRenumberer::CreateRenumbering(
    int32 old_num_elements,
    const std::vector<int32> &to_remove,
    std::vector<int32> *renumbering) {
  KALDI_ASSERT(IsSortedAndUniq(to_remove) && old_num_elements > 0);
  renumbering->clear();
  renumbering->resize(old_num_elements, 0);
  int32 num_remove = to_remove.size();
  for (int32 r = 0; r < num_remove; r++) {
    int32 this_remove = to_remove[r];
    // the "> 0" would be ">= 0" in a more generic context, but zero is
    // not valid in this particular application.
    KALDI_ASSERT(this_remove > 0 && this_remove < old_num_elements);
    (*renumbering)[this_remove] = -1;
  }
  int32 cur_number = 0;
  for (int32 i = 0; i < old_num_elements; i++) {
    if ((*renumbering)[i] != -1)
      (*renumbering)[i] = cur_number++;
  }
  KALDI_ASSERT(cur_number == old_num_elements -
               static_cast<int32>(to_remove.size()));
}


void IdentifySubmatrixArgsInComputation(NnetComputation *computation,
                                        std::vector<int32*> *submatrix_args) {
  IdentifySubmatrixArgs(&(computation->commands), submatrix_args);

  size_t extra_size = 0;
  for (size_t i = 0; i < computation->indexes_multi.size(); i++)
    extra_size += computation->indexes_multi[i].size();
  submatrix_args->reserve(submatrix_args->size() + extra_size);

  for (size_t i = 0; i < computation->indexes_multi.size(); i++) {
    std::vector<std::pair<int32, int32> > &indexes_multi =
        computation->indexes_multi[i];
    std::vector<std::pair<int32, int32> >::iterator
        iter = indexes_multi.begin(), end = indexes_multi.end();
    for (; iter != end; ++iter)
      if (iter->first != -1)
        submatrix_args->push_back(&(iter->first));
  }
}


void ComputationRenumberer::ComputeSubmatrixIsUsed() {
  int32 num_submatrices = computation_->submatrices.size();
  submatrix_is_used_.clear();
  submatrix_is_used_.resize(num_submatrices, false);
  submatrix_is_used_[0] = true;
  // the zeroth element of the array is 'special', it refers to the
  // zero submatrix, and we don't want to renumber it.
  std::vector<int32*> submatrix_args;
  IdentifySubmatrixArgsInComputation(computation_, &submatrix_args);
  std::vector<int32*>::iterator iter = submatrix_args.begin(),
      end = submatrix_args.end();
  int32 cur_submatrix_index = -1;  // an optimization to avoid too many
                                   // indexings of the bool vector
                                   // submatrix_is_used_.
  for (; iter != end; ++iter) {
    int32 submatrix_index = **iter;
    if (submatrix_index > 0 && submatrix_index != cur_submatrix_index) {
      cur_submatrix_index = submatrix_index;
      KALDI_ASSERT(submatrix_index < num_submatrices);
      submatrix_is_used_[submatrix_index] = true;
    }
  }
}

void ComputationRenumberer::ComputeMatrixIsUsed() {
  matrix_is_used_.clear();
  matrix_is_used_.resize(computation_->matrices.size(), false);
  matrix_is_used_[0] = true;

  std::vector<int32*> matrix_args;
  IdentifyMatrixArgsInComputation(computation_, &matrix_args);
  std::vector<int32*>::iterator iter = matrix_args.begin(),
      end = matrix_args.end();
  for (; iter != end; ++iter) {
    int32 matrix_index = **iter;
    if (matrix_index > 0)
      matrix_is_used_[matrix_index] = true;
  }
  // We also need to take into account when matrices are used indirectly via
  // submatrices (which is actually the main way they are accessed).
  int32 num_submatrices_orig = computation_->submatrices.size();
  for (int32 s = 1; s < num_submatrices_orig; s++) {
    int32 matrix_index = computation_->submatrices[s].matrix_index;
    if (submatrix_is_used_[s])
      matrix_is_used_[matrix_index] = true;
  }
}



void ComputationRenumberer::SetUpMappings() {
  num_matrices_new_ = CreateRenumbering(matrix_is_used_, &old_to_new_matrix_);

  unordered_map<NnetComputation::SubMatrixInfo, int32,
                SubMatrixHasher> submat_map;
  int32 cur_index = 1, num_submatrices_orig =
      computation_->submatrices.size();
  // the old_to_new_submatrix_ map will remove duplicates.
  // -1's will appear wherever a particular submatrix was never used.
  submatrix_is_kept_ = submatrix_is_used_;
  old_to_new_submatrix_.resize(num_submatrices_orig, -1);
  old_to_new_submatrix_[0] = 0;
  for (int32 s = 1; s < num_submatrices_orig; s++) {
    if (submatrix_is_used_[s]) {
      const NnetComputation::SubMatrixInfo &info =
          computation_->submatrices[s];
      if (submat_map.count(info) > 0) {  // a duplicate...
        old_to_new_submatrix_[s] = submat_map[info];
        submatrix_is_kept_[s] = false;
      } else {
        old_to_new_submatrix_[s] = (submat_map[info] = cur_index++);
      }
    }
  }
  num_submatrices_new_ = cur_index;
}

void ComputationRenumberer::RenumberSubmatrices() {
  std::vector<int32*> submatrix_args;
  IdentifySubmatrixArgsInComputation(computation_, &submatrix_args);
  std::vector<int32*>::iterator iter = submatrix_args.begin(),
      end = submatrix_args.end();
  for (; iter != end; ++iter) {
    if (**iter > 0) {
      int32 new_submatrix_index = old_to_new_submatrix_[**iter];
      // old_to_new_submatrix_[s] for s > 0 is only <= 0 (actually, -1) for
      // submatrices that are never accessed, and these should never appear
      // in this list.
      KALDI_ASSERT(new_submatrix_index > 0);
      **iter = new_submatrix_index;
    }
  }
  std::vector<NnetComputation::SubMatrixInfo> new_submatrices;
  int32 num_submatrices_old = computation_->submatrices.size();
  new_submatrices.reserve(num_submatrices_old);
  for (int32 s = 0; s < num_submatrices_old; s++)
    if (submatrix_is_kept_[s])
      new_submatrices.push_back(computation_->submatrices[s]);
  computation_->submatrices.swap(new_submatrices);
  // We'll map the matrix indexes inside computation_->submatrices
  // when we call RenumberMatrices().
}

void ComputationRenumberer::RenumberMatrices() {
  std::vector<int32*> matrix_args;
  IdentifyMatrixArgsInComputation(computation_, &matrix_args);
  std::vector<int32*>::iterator iter = matrix_args.begin(),
      end = matrix_args.end();
  for (; iter != end; ++iter) {
    if (**iter > 0) {
      int32 new_matrix_index = old_to_new_matrix_[**iter];
      // old_to_new_matrix_[s] for s > 0 is only <= 0 (actually, -1) for
      // submatrices that are never accessed, and these should never appear
      // in this list.
      KALDI_ASSERT(new_matrix_index > 0);
      **iter = new_matrix_index;
    }
  }

  std::vector<NnetComputation::MatrixInfo> new_matrices;
  int32 num_matrices_old = computation_->matrices.size();
  new_matrices.reserve(num_matrices_old);
  for (int32 m = 0; m < num_matrices_old; m++)
    if (matrix_is_used_[m])
      new_matrices.push_back(computation_->matrices[m]);
  computation_->matrices.swap(new_matrices);

  std::vector<NnetComputation::MatrixDebugInfo> new_debug_info;
  int32 debug_info_size = computation_->matrix_debug_info.size();
  KALDI_ASSERT(debug_info_size == 0 || debug_info_size == num_matrices_old);
  new_debug_info.reserve(debug_info_size);
  for (int32 m = 0; m < debug_info_size; m++) {
    if (matrix_is_used_[m]) {
      new_debug_info.push_back(NnetComputation::MatrixDebugInfo());
      new_debug_info.back().Swap(&(computation_->matrix_debug_info[m]));
    }
  }
  computation_->matrix_debug_info.swap(new_debug_info);
}


void ComputationRenumberer::Renumber() {
  RemoveUnusedIndexesMulti();
  ComputeSubmatrixIsUsed();
  ComputeMatrixIsUsed();
  SetUpMappings();
  RenumberSubmatrices();
  RenumberMatrices();
  RemoveIndexesMultiDuplicates();
  RenumberIndexes();
}

void ComputationRenumberer::RemoveUnusedIndexesMulti() {
  int32 num_indexes_multi = computation_->indexes_multi.size();
  if (num_indexes_multi == 0)
    return;  // Nothing to do.  An optimization.
  std::vector<bool> indexes_multi_used(num_indexes_multi, false);
  std::vector<int32*> indexes_multi_args;
  IdentifyIndexesMultiArgs(&(computation_->commands), &indexes_multi_args);
  std::vector<int32*>::iterator iter = indexes_multi_args.begin(),
      end = indexes_multi_args.end();
  for (; iter != end; ++iter) {
    int32 indexes_multi_index = **iter;
    KALDI_ASSERT(indexes_multi_index >= 0 &&
                 indexes_multi_index < num_indexes_multi);
    indexes_multi_used[indexes_multi_index] = 1;
  }
  // old->new mapping for the indexes_multi arrays.  will remain -1 for
  // ones that are unused.
  std::vector<int32> old_to_new(num_indexes_multi, -1);
  int32 new_num_indexes_multi = CreateRenumbering(indexes_multi_used,
                                                  &old_to_new);
  if (new_num_indexes_multi == num_indexes_multi)
    return;  // Nothing to do.  An optimization.
  std::vector<std::vector<std::pair<int32, int32> > >
      new_indexes_multi(new_num_indexes_multi);
  for (int32 i = 0; i < num_indexes_multi; i++) {
    if (old_to_new[i] != -1)
      new_indexes_multi[old_to_new[i]].swap(computation_->indexes_multi[i]);
  }
  computation_->indexes_multi.swap(new_indexes_multi);
  // renumber within the commands.
  for (iter = indexes_multi_args.begin(); iter != end; ++iter)
    **iter = old_to_new[**iter];
}


// removes duplicates within the indexes_multi_ array itself.
void ComputationRenumberer::RemoveIndexesMultiDuplicates() {
  int32 cur_index = 0,
      old_indexes_multi_size = computation_->indexes_multi.size();
  if (old_indexes_multi_size == 0)
    return;
  // create index mapping from old to new.  the use of map is generally not that
  // efficient, but the idea here is that we can do most of the comparisons just
  // based on the size of the vectors, and avoid even visiting most of their
  // contents.
  std::vector<int32> indexes_multi_old_to_new(old_indexes_multi_size);
  typedef std::vector<std::pair<int32,int32> > PairVectorType;
  typedef std::map<const PairVectorType*, int32,
                   PointerCompare<std::pair<int32,int32> > > MapType;
  MapType indexes_multi_map;
  for (int32 i = 0; i < computation_->indexes_multi.size(); i++) {
    std::pair<MapType::iterator, bool> p =
        indexes_multi_map.insert(std::pair<const PairVectorType*, int32>(
            &(computation_->indexes_multi[i]), cur_index));
    if (p.second) {  // was inserted-- was not there already.
      indexes_multi_old_to_new[i] = cur_index++;
    } else {
      int32 index_from_map = p.first->second;
      indexes_multi_old_to_new[i] = index_from_map;
    }
  }
  if (cur_index == old_indexes_multi_size)
    return;  // An optimization.  No duplicates were found.
  std::vector<PairVectorType> new_indexes_multi(cur_index);
  for (int32 i = 0; i < old_indexes_multi_size; i++) {
    int32 new_index = indexes_multi_old_to_new[i];
    computation_->indexes_multi[i].swap(new_indexes_multi[new_index]);
  }
  computation_->indexes_multi.swap(new_indexes_multi);

  std::vector<int32*> indexes_multi_args;
  IdentifyIndexesMultiArgs(&(computation_->commands), &indexes_multi_args);
  std::vector<int32*>::const_iterator iter = indexes_multi_args.begin(),
      end = indexes_multi_args.end();
  for (; iter != end; ++iter)
    **iter = indexes_multi_old_to_new[**iter];
}


void ComputationRenumberer::RenumberIndexes() {
  int32 old_num_indexes = computation_->indexes.size();
  if (old_num_indexes == 0)
    return;
  std::vector<int32*> indexes_args;
  IdentifyIndexesArgs(&(computation_->commands), &indexes_args);

  std::vector<bool> indexes_seen(old_num_indexes, false);
  std::vector<int32*>::const_iterator iter = indexes_args.begin(),
      end = indexes_args.end();
  for (; iter != end; ++iter)
    indexes_seen[**iter] = true;

  std::map<std::vector<int32>*, int32> vector_to_new_index;
  std::vector<int32> indexes_old_to_new(old_num_indexes);
  typedef std::map<const std::vector<int32>*, int32,
                   PointerCompare<int32> > MapType;
  MapType indexes_map;

  int32 cur_index = 0;
  for (int32 i = 0; i < old_num_indexes; i++) {
    if (!indexes_seen[i]) {
      indexes_old_to_new[i] = -1;
    } else {
      std::pair<MapType::iterator, bool> p =
          indexes_map.insert(std::pair<const std::vector<int32>*, int32>(
              &(computation_->indexes[i]), cur_index));
      if (p.second) {  // was inserted-- was not there already.
        indexes_old_to_new[i] = cur_index++;
      } else {
        int32 index_from_map = p.first->second;
        indexes_old_to_new[i] = index_from_map;
      }
    }
  }
  if (cur_index == old_num_indexes)
    return;  // An optimization.  No changes to the numbering are made.
  std::vector<std::vector<int32> > new_indexes(cur_index);
  for (int32 i = 0; i < old_num_indexes; i++) {
    int32 new_index = indexes_old_to_new[i];
    if (new_index != -1)
      computation_->indexes[i].swap(new_indexes[new_index]);
  }
  computation_->indexes.swap(new_indexes);

  // renumber the indexes inside the commmands.
  for (iter = indexes_args.begin(); iter != end; ++iter) {
    int32 old_index = **iter;
    KALDI_ASSERT(old_index >= 0 && old_index < old_num_indexes);
    int32 new_index = indexes_old_to_new[old_index];
    KALDI_ASSERT(new_index >= 0);
    **iter = new_index;
  }
}



void RenumberComputation(NnetComputation *computation) {
  ComputationRenumberer renumberer(computation);
  renumberer.Renumber();
}

void RemoveNoOps(NnetComputation *computation) {
  std::vector<NnetComputation::Command>::iterator
      input_iter = computation->commands.begin(),
      input_end = computation->commands.end(),
      output_iter = computation->commands.begin();
  for (; input_iter != input_end; ++input_iter) {
    if (input_iter->command_type != kNoOperation) {
      *output_iter = *input_iter;
      ++output_iter;
    }
  }
  computation->commands.resize(output_iter - computation->commands.begin());
}

/// Wherever matrix orig_matrix_index appears in the input of the network
/// (i.e. in computation->input_output_info), replaces it with new_matrix_index.
/// Returns true if it did replace it.
bool ReplaceInInput(
    const Nnet &nnet,
    int32 orig_matrix_index, int32 new_matrix_index,
    NnetComputation *computation) {
  bool ans = false;
  int32 num_matrices = computation->matrices.size();
  KALDI_ASSERT(orig_matrix_index > 0 && orig_matrix_index < num_matrices &&
               new_matrix_index > 0 && new_matrix_index < num_matrices);
  unordered_map<int32, std::pair<int32, int32> >::iterator
      iter = computation->input_output_info.begin(),
      end = computation->input_output_info.end();
  for (; iter != end; ++iter) {
    int32 network_node = iter->first,
        &value_matrix_index = iter->second.first,
        &deriv_matrix_index = iter->second.second;
    if (nnet.IsOutputNode(network_node)) {
      // deriv_matrix_index would be an input to the computation.
      if (deriv_matrix_index == orig_matrix_index) {
        deriv_matrix_index = new_matrix_index;
        ans = true;
      }
    } else {
      // value_matrix_index would be an input to the computation.
      if (value_matrix_index == orig_matrix_index) {
        value_matrix_index = new_matrix_index;
        ans = true;
      }
    }
  }
  return ans;
}


/// Wherever matrix orig_matrix_index appears in the output of the network
/// (i.e. in computation->input_output_info), replaces it with new_matrix_index.
/// Returns true if it did replace it.
bool ReplaceInOutput(
    const Nnet &nnet, int32 orig_matrix_index, int32 new_matrix_index,
    NnetComputation *computation) {
  bool ans = false;
  int32 num_matrices = computation->matrices.size();
  KALDI_ASSERT(orig_matrix_index > 0 && orig_matrix_index < num_matrices &&
               new_matrix_index > 0 && new_matrix_index < num_matrices);
  unordered_map<int32, std::pair<int32, int32> >::iterator
      iter = computation->input_output_info.begin(),
      end = computation->input_output_info.end();
  for (; iter != end; ++iter) {
    int32 network_node = iter->first,
        &value_matrix_index = iter->second.first,
        &deriv_matrix_index = iter->second.second;
    if (nnet.IsOutputNode(network_node)) {
      // value_matrix_index would be an output of the computation.
      if (value_matrix_index == orig_matrix_index) {
        value_matrix_index = new_matrix_index;
        ans = true;
      }
    } else {
      // deriv_matrix_index would be an output of the computation.
      if (deriv_matrix_index == orig_matrix_index) {
        // we'd only have derivatives for actual inputs. [note: we also allow
        // users to provide inputs for component nodes, but these would not have
        // derivatives.]
        KALDI_ASSERT(nnet.IsInputNode(network_node));
        deriv_matrix_index = new_matrix_index;
        ans = true;
      }
    }
  }
  return ans;
}


VariableMergingOptimizer::VariableMergingOptimizer(
    const NnetOptimizeOptions &config,
    const Nnet &nnet,
    const ComputationRequest &request,
    NnetComputation *computation):
    config_(config), nnet_(nnet), request_(request),
    computation_(computation),
    already_called_merge_variables_(false) {
  analyzer_.Init(nnet, *computation);
  ComputeMatrixToSubmatrix(*computation_, &matrix_to_submatrix_);
  variable_dirty_.resize(analyzer_.variables.NumVariables(), false);
}

bool VariableMergingOptimizer::MergeVariables() {
  KALDI_ASSERT(!already_called_merge_variables_);
  already_called_merge_variables_ = true;
  if (!config_.optimize)
    return false;
  bool merged = false;
  int32 num_commands = computation_->commands.size();
  for (int32 command_index = 0; command_index < num_commands;
       command_index++) {
    // This loop looks for pairs of sub-matrix indexes s1,s2 that we could
    // potentially merge into a single variable.
    const NnetComputation::Command &c =
        computation_->commands[command_index];
    int32 s1 = -1, s2 = -1;
    if (c.command_type == kMatrixCopy &&
        config_.remove_assignments) {
      s2 = c.arg1;  // s2 is the written-to matrix.
      s1 = c.arg2;
    } else if (c.command_type == kPropagate &&
               config_.propagate_in_place) {
      const Component *component = nnet_.GetComponent(c.arg1);
      if (component->Properties() & kPropagateInPlace) {
        s1 = c.arg3;
        s2 = c.arg4;  // s2 is the written-to matrix.
      }
    } else if ((c.command_type == kBackprop ||
                c.command_type == kBackpropNoModelUpdate) &&
               config_.backprop_in_place) {
      const Component *component = nnet_.GetComponent(c.arg1);
      if (component->Properties() & kBackpropInPlace) {
        s1 = c.arg5;
        s2 = c.arg6;  // s2 is the written-to matrix.
        if (s1 == c.arg3 || s2 == c.arg3 || s1 == c.arg4 || s2 == c.arg4) {
          // we don't think this should ever happen, but just out of an
          // abundance of caution: if either of these submatrix indexes are the
          // input-value or output-value args to Backprop, don't do the optimization.
          s1 = -1;
          s2 = -1;
        }
      }
    }
    if (s1 > 0 && s2 > 0) {
      std::pair<bool,bool> p = MayBeMerged(command_index, s1, s2);
      if (p.first) {
        DoLeftMerge(command_index, s1, s2);
        merged = true;
      } else if (p.second) {
        DoRightMerge(command_index, s1, s2);
        merged = true;
      }
    }
  }
  if (merged) {
    RenumberComputation(computation_);
    RemoveNoOps(computation_);
  }
  return merged;
}

/**
   This static function returns a SubMatrixInfo corresponding to
   replacing the matrix-index in a's "matrix_index" with, essentially, sub-matrix b.
   Of course the matrix_index will be b's "matrix_index", but we may
   have to modify the row and column offsets.  The idea is that sub-matrix
   submat_b should have the same dimensions as the matrix underlying
   submat_a.
 */
static NnetComputation::SubMatrixInfo GetSubMatrixOfSubMatrix(
    const NnetComputation &computation, int32 submat_a, int32 submat_b) {
  KALDI_ASSERT(static_cast<size_t>(submat_a) < computation.submatrices.size());
  KALDI_ASSERT(static_cast<size_t>(submat_b) < computation.submatrices.size());
  const NnetComputation::SubMatrixInfo &a = computation.submatrices[submat_a],
                                       &b = computation.submatrices[submat_b];
  const NnetComputation::MatrixInfo &a_mat =
      computation.matrices[a.matrix_index];
  KALDI_ASSERT(a_mat.num_rows == b.num_rows && a_mat.num_cols == b.num_cols);
  NnetComputation::SubMatrixInfo ans;
  ans.matrix_index = b.matrix_index;
  ans.row_offset = a.row_offset + b.row_offset;
  ans.num_rows = a.num_rows;
  ans.col_offset = a.col_offset + b.col_offset;
  ans.num_cols = a.num_cols;
  return ans;
}

void VariableMergingOptimizer::MarkAsDirty(int32 s) {
  std::vector<int32> variable_indexes;
  analyzer_.variables.AppendVariablesForSubmatrix(s, &variable_indexes);
  std::vector<int32>::const_iterator iter = variable_indexes.begin(),
      end = variable_indexes.end();
  for (; iter != end; ++iter) {
    int32 v = *iter;
    KALDI_ASSERT(static_cast<size_t>(v) < variable_dirty_.size());
    variable_dirty_[v] = true;
  }
}

void VariableMergingOptimizer::DoRightMerge(int32 command_index,
                                            int32 s1, int32 s2) {
  // Prevent further optimizations touching s1 or s2 (we can
  // try again in a later round of optimization, with a new
  // instance of this class).
  MarkAsDirty(s1);
  MarkAsDirty(s2);

  int32 m1 = computation_->submatrices[s1].matrix_index,
      m2 = computation_->submatrices[s2].matrix_index;
  KALDI_ASSERT(m1 != m2 && m1 > 0 && m2 > 0);
  { // modify submatrices for submatrices of m1 to effectively be sub-matrices of
    // s2 instead (they will refer to m2 as the matrix_index).
    std::vector<int32>::const_iterator iter = matrix_to_submatrix_[m1].begin(),
        end = matrix_to_submatrix_[m1].end();
    for (; iter != end; ++iter) {
      int32 submatrix_index = *iter;
      KALDI_ASSERT(computation_->submatrices[submatrix_index].matrix_index==m1);
      computation_->submatrices[submatrix_index] =
          GetSubMatrixOfSubMatrix(*computation_, submatrix_index, s2);
    }
  }
  const std::vector<MatrixAccesses> &matrix_accesses = analyzer_.matrix_accesses;
  // - If m1 was an input, replace it as an input with m2
  bool replaced = ReplaceInInput(nnet_, m1, m2, computation_);
  KALDI_ASSERT(replaced == matrix_accesses[m1].is_input);
  if (replaced) {  // Remove the command that allocates m2.
    int32 alloc_command = matrix_accesses[m2].allocate_command;
    KALDI_ASSERT(alloc_command != -1);
    computation_->commands[alloc_command].command_type =
        kNoOperation;
  }
  // we keep matrix m2 (so m2 is m_to_keep, m1 is m_to_discard).
  DoMergeCommon(command_index, m2, m1);
}

void VariableMergingOptimizer::DoMergeCommon(int32 command_index,
                                             int32 m_to_keep,
                                             int32 m_to_discard) {
  NnetComputation::Command &c = computation_->commands[command_index];
  const std::vector<MatrixAccesses> &matrix_accesses =
      analyzer_.matrix_accesses;

  //  - If it was case (a), replace the assignment command with a no-op.
  if (c.command_type == kMatrixCopy) {
    // remove the command.
    c.command_type = kNoOperation;
    c.arg1 = -1;
    c.arg2 = -1;
  }

  //   - If both m_to_keep and m_to_discard have commands that deallocate them, keep only the
  //    later of the two and make it refer to m_to_keep (otherwise delete any
  //     deallocation command).
  int32 dealloc1 = matrix_accesses[m_to_keep].deallocate_command,
      dealloc2 = matrix_accesses[m_to_discard].deallocate_command;
  if (dealloc1 != -1 && dealloc2 != -1) {
    int32 earlier_index = std::min(dealloc1, dealloc2),
            later_index = std::max(dealloc1, dealloc2);
    NnetComputation::Command
        &earlier_command = computation_->commands[earlier_index],
        &later_command = computation_->commands[later_index];
    earlier_command.command_type = kNoOperation;
    later_command.arg1 = m_to_keep;
  } else {
    if (dealloc1 != -1)
      computation_->commands[dealloc1].command_type =
          kNoOperation;
    if (dealloc2 != -1)
      computation_->commands[dealloc2].command_type =
          kNoOperation;
  }

  //   - If both m_to_keep and m_to_discard have commands that allocate them,
  //     keep only the earlier of the two and make it refer to m_to_keep
  //     (otherwise delete any allocation command).
  int32 alloc1 = matrix_accesses[m_to_keep].allocate_command,
      alloc2 = matrix_accesses[m_to_discard].allocate_command;
  if (alloc1 != -1 && alloc2 != -1) {
    int32 earlier_index = std::min(alloc1, alloc2),
        later_index = std::max(alloc1, alloc2);
    NnetComputation::Command
        &earlier_command = computation_->commands[earlier_index],
        &later_command = computation_->commands[later_index];
    later_command.command_type = kNoOperation;
    earlier_command.arg1 = m_to_keep;
    // Make sure we don't initialize as undefined- checking that
    // that is correct would require some analysis.  We'll deal with
    // that in a later optimization pass.
    if (earlier_command.command_type == kAllocMatrixUndefined) {
      earlier_command.command_type = kAllocMatrixZeroed;
    } else if (earlier_command.command_type == kAllocMatrixFromOther) {
      earlier_command.command_type = kAllocMatrixFromOtherZeroed;
    }
  } else {
    if (alloc1 != -1)
      computation_->commands[alloc1].command_type =
          kNoOperation;
    if (alloc2 != -1)
      computation_->commands[alloc2].command_type =
          kNoOperation;
  }
}

void VariableMergingOptimizer::DoLeftMerge(int32 command_index,
                                           int32 s1, int32 s2) {
  // Prevent further optimizations touching s1 or s2 (we can
  // try again in a later round of optimization, with a new
  // instance of this class).
  MarkAsDirty(s1);
  MarkAsDirty(s2);

  int32 m1 = computation_->submatrices[s1].matrix_index,
      m2 = computation_->submatrices[s2].matrix_index;
  KALDI_ASSERT(m1 != m2 && m1 > 0 && m2 > 0);
  { // modify submatrices for submatrices of m2 to effectively be sub-matrices of
    // s1 instead (they will refer to m1 as the matrix_index).
    std::vector<int32>::const_iterator iter = matrix_to_submatrix_[m2].begin(),
        end = matrix_to_submatrix_[m2].end();
    for (; iter != end; ++iter) {
      int32 submatrix_index = *iter;
      KALDI_ASSERT(computation_->submatrices[submatrix_index].matrix_index==m2);
      computation_->submatrices[submatrix_index] =
          GetSubMatrixOfSubMatrix(*computation_, submatrix_index, s1);
    }
  }
  const std::vector<MatrixAccesses> &matrix_accesses = analyzer_.matrix_accesses;
  // - If m2 was an output, replace it as an input with m1.
  bool replaced = ReplaceInOutput(nnet_, m2, m1, computation_);
  KALDI_ASSERT(replaced == matrix_accesses[m2].is_output);
  if (replaced) {  // Remove the command that deallocates m1.
    int32 dealloc_command = matrix_accesses[m1].deallocate_command;
    KALDI_ASSERT(dealloc_command != -1);
    computation_->commands[dealloc_command].command_type =
        kNoOperation;
  }
  // we keep matrix m1 (so m1 is m_to_keep, m2 is m_to_discard).
  DoMergeCommon(command_index, m1, m2);
}




std::pair<bool,bool> VariableMergingOptimizer::MayBeMerged(
    int32 command_index, int32 s1, int32 s2) const {
  KALDI_ASSERT(s1 > 0 && s2 > 0 && static_cast<size_t>(command_index) <
               computation_->commands.size());
  if (!config_.allow_left_merge && !config_.allow_right_merge)
    return std::pair<bool,bool>(false,false);
  int32 m1 = computation_->submatrices[s1].matrix_index,
      m2 = computation_->submatrices[s2].matrix_index;
  // we can't merge two different submatrices of the same matrix.
  if (m1 == m2) return std::pair<bool,bool>(false,false);
  std::vector<int32> variable_indexes;
  analyzer_.variables.AppendVariablesForSubmatrix(s1, &variable_indexes);
  analyzer_.variables.AppendVariablesForSubmatrix(s2, &variable_indexes);
  std::vector<int32>::iterator iter = variable_indexes.begin(),
      end = variable_indexes.end();
  // condition c5:
  for (; iter != end; ++iter)
    if (variable_dirty_[*iter])
      return std::pair<bool,bool>(false,false);
  const std::vector<MatrixAccesses> &matrix_accesses = analyzer_.matrix_accesses;
  const MatrixAccesses &m1_access = matrix_accesses[m1],
      &m2_access = matrix_accesses[m2];
  // condition c1:
  if ((m1_access.is_input && m2_access.is_input) ||
      (m1_access.is_output && m2_access.is_output))
    return std::pair<bool,bool>(false,false);
  // condition c2:
  if ((m1_access.is_input || m1_access.is_output ||
       m2_access.is_input || m2_access.is_output) &&
      (!computation_->IsWholeMatrix(s1) ||
       !computation_->IsWholeMatrix(s2)))
    return std::pair<bool,bool>(false,false);
  bool left = config_.allow_left_merge,
      right = config_.allow_right_merge;
  // condition c3:
  if (!computation_->IsWholeMatrix(s2)) left = false;
  if (!computation_->IsWholeMatrix(s1)) right = false;
  if (!left && !right)  // save some time.
    return std::pair<bool,bool>(false,false);
  bool is_assignment = (computation_->commands[command_index].command_type ==
                        kMatrixCopy);
  ComputationAnalysis analysis(*computation_, analyzer_);
  if (is_assignment) {
    if (analysis.FirstAccess(s2) == command_index &&
        analysis.LastWriteAccess(s1) < command_index &&
        analysis.LastAccess(s1) <
        analysis.DataInvalidatedCommand(command_index, s2)) {
      return std::pair<bool,bool>(left, right);  // possible success.
    }
  } else {
    if (analysis.FirstAccess(s2) == command_index &&
        analysis.LastAccess(s1) == command_index) {
      return std::pair<bool,bool>(left, right);  // possible success.
    }
  }
  // failure.
  return std::pair<bool,bool>(false,false);
}

void ModelUpdateConsolidator::AppendDebugInfoForSubmatrix(
    int32 submatrix_index,
    NnetComputation::MatrixDebugInfo *debug_info) const {
  KALDI_ASSERT(!computation_->matrix_debug_info.empty());
  KALDI_ASSERT(static_cast<size_t>(submatrix_index) <
               computation_->submatrices.size());
  NnetComputation::SubMatrixInfo submatrix_info =
      computation_->submatrices[submatrix_index];
  int32 matrix_index = submatrix_info.matrix_index;
  KALDI_ASSERT(matrix_index > 0 && static_cast<size_t>(matrix_index) <
               computation_->matrix_debug_info.size());
  const NnetComputation::MatrixDebugInfo &src_info =
      computation_->matrix_debug_info[matrix_index];
  debug_info->is_deriv = src_info.is_deriv;
  KALDI_ASSERT(src_info.cindexes.size() ==
               computation_->matrices[matrix_index].num_rows);
  int32 row_begin = submatrix_info.row_offset,
      row_end = row_begin + submatrix_info.num_rows;
  debug_info->cindexes.insert(debug_info->cindexes.end(),
                             src_info.cindexes.begin() + row_begin,
                             src_info.cindexes.begin() + row_end);
}


int32 ModelUpdateConsolidator::ConsolidateSubmatrices(
    const std::vector<int32> &commands,
    const std::vector<int32> &submatrices) {
  int32 num_submatrices = submatrices.size();
  KALDI_ASSERT(num_submatrices > 1 && commands.size() == submatrices.size());
  int32 first_submatrix = submatrices[0];
  int32 num_cols = computation_->submatrices[first_submatrix].num_cols,
      num_rows = 0;
  NnetComputation::MatrixDebugInfo debug_info;
  for (int32 i = 0; i < num_submatrices; i++) {
    int32 submatrix = submatrices[i];
    num_rows += computation_->submatrices[submatrix].num_rows;
    KALDI_ASSERT(computation_->submatrices[submatrix].num_cols == num_cols);
    if (!computation_->matrix_debug_info.empty())
      AppendDebugInfoForSubmatrix(submatrix, &debug_info);
  }
  // new_whole_submatrix is a new submatrix index corresponding to the whole
  // of a new matrix that we are creating.
  int32 new_whole_submatrix = computation_->NewMatrix(num_rows, num_cols);
  // Add a command at the very start, to initialize this new matrix.
  int32 new_matrix_index =
      computation_->submatrices[new_whole_submatrix].matrix_index;
  extra_commands_[0].push_back(
      NnetComputation::Command(kAllocMatrixUndefined, new_matrix_index));
  final_deallocate_commands_.push_back(
      NnetComputation::Command(kDeallocMatrix, new_matrix_index));
  if (!computation_->matrix_debug_info.empty())
    computation_->matrix_debug_info[new_matrix_index].Swap(&debug_info);

  int32 row_offset = 0;
  for (int32 i = 0; i < num_submatrices; i++) {
    int32 submatrix_index = submatrices[i];
    int32 this_num_rows = computation_->submatrices[submatrix_index].num_rows;
    // submatrix corresponding to the part of the new matrix corresponding
    // to 'submatrices[i]'.
    int32 new_submatrix = computation_->NewSubMatrix(new_whole_submatrix,
                                                     row_offset, this_num_rows,
                                                     0, num_cols);
    // Just before command 'commands[i]', add a command that assigns to the
    // submatrix numbered 'new_submatrix' the contents of the submatrix numbered
    // 'submatrices[i]'.  Note: we hope that a later pass of optimization
    // (VariableMergingOptimization) will remove this redundant copy by
    // having the operation that created it right directly to the location
    // we want it to be.
    NnetComputation::Command c(kMatrixCopy, new_submatrix, submatrices[i]);
    extra_commands_[commands[i]].push_back(c);
    row_offset += this_num_rows;
  }
  KALDI_ASSERT(row_offset == num_rows);
  return new_whole_submatrix;
}

void ModelUpdateConsolidator::AddCommandsToComputation() {
  KALDI_ASSERT(computation_->commands.size() == extra_commands_.size());
  int32 old_num_commands = computation_->commands.size(),
      new_num_commands = old_num_commands +
      static_cast<int32>(final_commands_.size() +
                         final_deallocate_commands_.size());
  for (size_t i = 0; i < extra_commands_.size(); i++)
    new_num_commands += static_cast<int32>(extra_commands_[i].size());
  std::vector<NnetComputation::Command> new_commands;
  new_commands.reserve(new_num_commands);
  for (int32 c = 0; c < old_num_commands; c++) {
    new_commands.insert(new_commands.end(),
                        extra_commands_[c].begin(), extra_commands_[c].end());
    new_commands.push_back(computation_->commands[c]);
  }
  new_commands.insert(new_commands.end(),
                      final_commands_.begin(), final_commands_.end());
  new_commands.insert(new_commands.end(),
                      final_deallocate_commands_.begin(),
                      final_deallocate_commands_.end());
  computation_->commands.swap(new_commands);
}

/** This function, called from ConsolidateModelUpdate, is passed a list of
    commands that are all backprops for the same component, and it consolidates
    them into a single model-update command. */
void ModelUpdateConsolidator::ConsolidateUpdateForComponent(
    int32 component_index,
    const std::vector<int32> &backprop_commands) {
  const Component *component = nnet_.GetComponent(component_index);
  int32 num_backprop_commands = backprop_commands.size();

  bool need_input = (component->Properties() & kBackpropNeedsInput) != 0,
      need_output = (component->Properties() & kBackpropNeedsOutput) != 0;

  std::vector<int32>  input_submatrices(num_backprop_commands),
      output_submatrices(num_backprop_commands),
      output_deriv_submatrices(num_backprop_commands);

  for (int32 i = 0; i < num_backprop_commands; i++) {
    int32 command_index = backprop_commands[i];
    NnetComputation::Command &command =
        computation_->commands[command_index];
    // arg2 must be 0 because simple components don't use precomputed indexes.
    KALDI_ASSERT(command.command_type == kBackprop && command.arg2 == 0);
    command.command_type = kBackpropNoModelUpdate;
    int32 input_submatrix = command.arg3,
        output_submatrix = command.arg4,
        output_deriv_submatrix = command.arg5;
    KALDI_ASSERT((input_submatrix != 0) == need_input &&
                 (output_submatrix != 0) == need_output);
    input_submatrices[i] = input_submatrix;
    output_submatrices[i] = output_submatrix;
    output_deriv_submatrices[i] = output_deriv_submatrix;
  }
  // Get the sub-matrix indexes of whichever of the consolidated matrices we
  // need (will usually be input_submatrix and output_deriv_submatrix).
  int32 input_submatrix = (need_input ?
                           ConsolidateSubmatrices(backprop_commands,
                                                  input_submatrices) : 0),
      output_submatrix = (need_output ?
                         ConsolidateSubmatrices(backprop_commands,
                                                output_submatrices) : 0),
      output_deriv_submatrix = ConsolidateSubmatrices(backprop_commands,
                                                      output_deriv_submatrices);
  int32 precomputed_indexes_index = 0,  // unused since simple component
      input_deriv_submatrix = 0;  // we don't need the input-deriv, so this is
                                  // zero.
  NnetComputation::Command c(kBackprop, component_index, precomputed_indexes_index,
                             input_submatrix, output_submatrix,
                             output_deriv_submatrix, input_deriv_submatrix);
  final_commands_.push_back(c);
}

ModelUpdateConsolidator::ModelUpdateConsolidator(
    const Nnet &nnet,
    NnetComputation *computation):
    nnet_(nnet), computation_(computation),
    extra_commands_(computation->commands.size()) { }

void ModelUpdateConsolidator::ConsolidateModelUpdate() {
  int32 num_components = nnet_.NumComponents(),
      num_commands = computation_->commands.size();
  // 'backprop_commands' is a list, for each component (but nonempty only for
  // updatable components), of the command indexes for the backprop commands.
  std::vector<std::vector<int32> > backprop_commands(num_components);
  std::vector<NnetComputation::Command>::const_iterator iter =
      computation_->commands.begin(), end = computation_->commands.end();
  for (int32 command_index = 0;
       command_index < num_commands; command_index++) {
    const NnetComputation::Command &c = computation_->commands[command_index];
    if (c.command_type == kBackprop) {
      int32 component_index = c.arg1;
      const Component *component = nnet_.GetComponent(component_index);
      if (component->Properties() & kUpdatableComponent)
        backprop_commands[component_index].push_back(command_index);
    }
  }
  bool consolidated = false;
  for (int32 component = 0; component < num_components; component++) {
    if (backprop_commands[component].size() > 1) {
      ConsolidateUpdateForComponent(component,
                                    backprop_commands[component]);
      consolidated = true;
    }
  }
  if (!consolidated)  // This is an optimization to avoid redundant computation
    return;           // if there is nothing to do.
  // the following function call commits all the commands we stored in member
  // variables, to computation_->commands.
  AddCommandsToComputation();
}



} // namespace nnet3
} // namespace kaldi
