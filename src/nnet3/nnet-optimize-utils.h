// nnet3/nnet-optimize-utils.h

// Copyright 2015    Johns Hopkins University (author: Daniel Povey)

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

#ifndef KALDI_NNET3_NNET_OPTIMIZE_UTILS_H_
#define KALDI_NNET3_NNET_OPTIMIZE_UTILS_H_

#include "nnet3/nnet-compile.h"
#include "nnet3/nnet-analyze.h"

namespace kaldi {
namespace nnet3 {


struct NnetOptimizeOptions;  // Forward declaration.

/**
   This class is responsible for merging matrices, although you probably want to
   access it via the the function VariableMergingOptimization().

   We identify pairs of submatrices which can potentially be merged into a single
   submatrix.

   Suppose there are two different submatrices s1 != s2 that are submatrices of
   different respective matrices m1 != m2, and somewhere in the computation we
   have a command C, which is one of:
      (a) the assignment command  "s2 = s1", or
      (b) a propagate command with s1 as input and s2 as output, with a component
          that supports propagate in place, or
      (c) a backprop command with s1 as output-deriv and s2 as input-deriv, with
          a component that supports backprop in place.

   Then the triple (C, s1, s2) is a candidate for merging.  We support two types
   of merging: 'right merging', in which we delete s1 and use s2 instead; and
   'left merging' in which we delete s2 and use s1 instead.  The two types of
   merging may seem to be the same thing, but remember that in general s1 and s2
   may be sub-matrices of larger matrices.

   Note: the following
     - Define last-access(submatrix) as:
       If matrix-of(submatrix) is an output, then num-commands, otherwise the
       last command that accesses that submatrix for either read or write.  [note:
       deallocation does not count as a read or write operation].
     - Define first-access(submatrix) as:
       If matrix-of(submatrix) is an input, then -1, otherwise the first command
       that is *not* an allocation command that accessed that submatrix for either
       read or write.
     - Define last-write-access(submatrix) as the last command-index that accessed
       the submatrix in a write operation, or -1 if there is no such command (this
       could happen for inputs).
     - Define data-invalidated-command(c, submatrix) as the first
       command-index after 'c' that 'submatrix' is written to; or if there is
       no such command, then the command index of the deallocation command
       for 'submatrix'; or if this does not exist, then num-commands.

   The conditions that must be satisfied for both left and right merges:
     - It cannot be the case that m1 and m2 are both inputs, or that they are
       both outputs.  [condition c1]
     - If either m1 or m2 is an input or an output, then s1 must be the entirety
       of m1 and s2 must be the entirety of m2 (this is because inputs and outputs
       must be whole matrices). [condition c2]
     - If we are left-merging (deleting s2,m2), then s2 must be the entirety of m2.
       [condition c3]
     - If we are right-merging (deleting s1,m1), then s1 must be the entirety of m1.
       [condition c4]
     - None of the the variables underlying s1 and s2 may be marked as 'dirty'
       (implying that they were the subjects of a previous merge during the lifetime of
       this class) [condition c5]

   If the command C is case (a), i.e. an assignment operation, then the following
   conditions must apply:
     - first-access(s2) == C
     - last-write-access(s1) < C
     - last-access(s1) < data-invalidated-command(C, s2)
   Otherwise (cases (b) and (c), in-place propagate or backprop), we insist that:
     - first-access(s2) == C
     - last-access(s1) == C


   The sequence of things we have to do for a right-merge (in which we delete
   s1,m1) is as follows:
     - All submatrices that reference m1, make them reference m2 instead.
       [later we'll renumber so that there are no duplicates.]
     - If m1 was an input, replace it as an input with m2 and remove the
       command that allocated m2.
     - If it was an assignment [case (a)], replace the assignment command with a
       no-op.
     - If both m1 and m2 have commands that allocate them, keep only the
       earlier of the two and make it refer to m1 (otherwise delete any
       allocation command, because m1 must be an input); and make sure
       it zeroes the new data (later we can change it to undefined
       initialization, if possible).
     - If both m1 and m2 have commands that deallocate them, keep only the
       later of the two and make it refer to m2 (otherwise delete any
       deallocation command, because m2 must be an output).


   The sequence of things we have to do for a right-merge (in which we delete
   s1,m1) is as follows:
     - All submatrices that reference m2, make them reference m1 instead.
       [later we'll renumber so that there are no duplicates.]
     - If m2 was an output, replace it as an output with m1 and remove the
       command that deallocated m1.
    ... the last three bullet-points, regarding removing the assignment
        command, and allocation and deallocation, are the same as for a
        left-merge.

   At the end when we call RemoveOrphanMatrices(), the renumbering code will
   automatically detect that there are duplicate submatrices, and will merge
   them, as well as removing the now-unused matrix indexes.  After merging, we
   will mark the variables (i.e. row-ranges) underlying s1 and s2 as being
   "dirty" so they can no longer be merged during the lifetime of this class.
 */
class VariableMergingOptimizer {
 public:
  VariableMergingOptimizer(const NnetOptimizeOptions &config,
                           const Nnet &nnet,
                           const ComputationRequest &request,
                           NnetComputation *computation);
  // Note: you can call this only once.  If it returns true, it means it has
  // merged variables.  In this case, you have the option to instantiate another
  // copy of the class and try again with that other copy.
  bool MergeVariables();

 private:
  /// @brief This function returns a pair of bools saying whether we can do a
  ///   (left and/or right) merge respectively, based on the conditions defined
  ///   in the header.
  ///
  /// Note: if one of the variables underlying s1 or s2 is marked as 'dirty' due
  /// to a previous merge, this function will return (false,false).  The terms
  /// left-merge and right-merge are defined in the extended comment above this
  /// class.  Note: left_merge will always be false if config.allow_left_merge
  /// == false, and the same respectively for right_merge.
  ///
  ///  @param command  [in] The command-index that assigns s2 := s1
  ///                        or does a forward or backprop with s1 as the
  ///                        input and s2 as the output
  ///  @param s1   [in]     A submatrix-index s1 > 0.
  ///  @param s2   [in]     A submatrix-index s2 > 0
  std::pair<bool,bool> MayBeMerged(int32 command, int32 s1, int32 s2) const;

  // performs the left merge.  Search for left-merge in the comment
  // above the class declaration for details.
  void DoLeftMerge(int32 command_index, int32 s1, int32 s2);

  // performs the right merge.  Search for right-merge in the comment
  // above the class declaration for details.
  void DoRightMerge(int32 command_index, int32 s1, int32 s2);

  // Performs the actions common to both left and right merges, regarding
  // removing the assignment command, and allocation and deallocation (called
  // from DoLeftMerge and DoRightMerge).  The m_to_keep and m_to_discard
  // are the matrix-indexes we will keep and discard respectively.
  void DoMergeCommon(int32 command_index, int32 m_to_keep,
                     int32 m_to_discard);

  /// Marks the variables underlying submatrix 's' as dirty
  void MarkAsDirty(int32 s);

  void Initialize();

  const NnetOptimizeOptions &config_;
  const Nnet &nnet_;
  const ComputationRequest &request_;
  NnetComputation *computation_;

  Analyzer analyzer_;

  // lists of submatrices that correspond to each matrix.
  std::vector<std::vector<int32> > matrix_to_submatrix_;

  // for each variable (as defined by analyzer_.variables), true if
  // we have already performed a merge on it.
  std::vector<bool> variable_dirty_;

  bool already_called_merge_variables_;
};


/** This class is responsible for consolidating the model-update part of
    backprop commands, for components in (e.g.) recurrent networks that need to
    have many separate backprop commands, into more efficient single commands
    operating on consolidated data in larger matrices.  This is useful for
    recurrent networks.  */
class ModelUpdateConsolidator {
 public:
  ModelUpdateConsolidator(const Nnet &nnet,
                          NnetComputation *computation);
  void ConsolidateModelUpdate();
 private:
  void ConsolidateUpdateForComponent(
      int32 component,
      const std::vector<int32> &backprop_commands);

  /// This function, called at the end of ConsolidateModelUpdate(), takes the
  /// commands that we have put in extra_commands_, final_commands_ and
  /// final_deallocate_commands_, and puts them in the appropriate place in
  /// computation->commands_.
  void AddCommandsToComputation();

  /// You call this function when you want to consolidate the values of a list
  /// of submatrices taken just prior particular commands.  The input 'commands'
  /// and 'submatrices' lists must be the same size, and size must be > 1.  This
  /// function will create a new matrix that is the row-wise concatentation of
  /// all these submatrices, with values taken just prior to the respective
  /// command indexes.  This function will will add to extra_commands_ the
  /// commands to do the copying at the appropriate places (at the supplied
  /// command indexes; they will be inserted just before).  The return value is
  /// the submatrix index of a submatrix that represents the whole of the
  /// consolidated matrix.  This command will insert, at the beginning of
  /// the computation (in extra_commands_[0]), a command to initialize the matrix;
  /// and will append to final_deallocate_commands_ the commands to deallocate
  /// the matrix.
  /// If computation_->matrix_debug_info is nonempty, this function will
  /// also update computation_->matrix_debug_info with suitable values
  /// for the newly added matrix
  int32 ConsolidateSubmatrices(
      const std::vector<int32> &commands,
      const std::vector<int32> &submatrices);

  /// This function, called from ConsolidateSubmatrices, will
  /// update 'debug_info' by appending the corresponding 'indexes' from
  /// the existing debug info for this submatrix.  It will also set
  /// the 'is_deriv' of '*debug_info' to the same value as the
  /// debug info for 'submatrix_index', and set the 'node_index' to the
  /// 'node_index' in the debug info for that submatrix-index.
  /// It requires that computation_->matrix_debug_info be nonempty.
  void AppendDebugInfoForSubmatrix(
      int32 submatrix_index,
      NnetComputation::MatrixDebugInfo *debug_info) const;

  const Nnet &nnet_;
  NnetComputation *computation_;

  // Indexed by the original command index in *computation_ (and sized to the
  // original number of commands in *computation_ before we added anything),
  // extra_commands_[c] contains a list of commands that need to be inserted
  // just before command c in the previously existing computation.
  std::vector<std::vector<NnetComputation::Command> > extra_commands_;

  // This is as list of kBackprop commands that will be placed after the
  // commands in 'computation_->commands' and 'extra_commands_', but before
  // the 'final_deallocate_commands_'.
  std::vector<NnetComputation::Command> final_commands_;
  // This is a list of commands to deallocate our 'consolidated' matrices; the
  // commands will be placed after the commands in 'final_commands_'.
  std::vector<NnetComputation::Command> final_deallocate_commands_;
};


// We declare this class in the .cc file, we don't need to export it.
// It's used inside RemoveOrphanMatrices. 
class ComputationRenumberer {
 public:
  ComputationRenumberer(NnetComputation *computation):
      computation_(computation) { }

  void Renumber();
 private:
  // this function removes unused vectors within the indexes_multi_ array, i.e.
  // ones that are not referenced in the computation.
  void RemoveUnusedIndexesMulti();
  // this function computes the submatrix_is_used_ vector, saying whether each
  // of the original submatrices is referenced somewhere.
  void ComputeSubmatrixIsUsed();
  // this function computes the matrix_is_used_ vector (from the
  // submatrix_is_used_ vector, from computation_->input_output_info, and from
  // computation_->commands, saying whether each of the original matrices is
  // referenced somewhere, directly or indirectly.
  void ComputeMatrixIsUsed();
  // This function sets up mappings from old to new matrix and submatrix indexes,
  // writing to num_{,sub}matrices_new_ and old_to_new_{,sub}matrix_.
  void SetUpMappings();
  // This function renumbers submatrix indexes appearing within commands and
  // indexes_multi_, and then removes unused submatrices from the list of
  // submatrices while leaving the matrix-indexes at their old values (they will
  // be mapped by RenumberMatrices()).
  void RenumberSubmatrices();
  // renumber matrix indexes appearing within 'commmands', within 'submatrices'
  // and 'input_output_info'; renumber 'matrices' and if applicable
  // 'debug_info'.
  void RenumberMatrices();
  // removes duplicates within the indexes_multi_ array itself.
  void RemoveIndexesMultiDuplicates();

  struct SubMatrixHasher {
    SubMatrixHasher() { }
    size_t operator () (const NnetComputation::SubMatrixInfo &submat) const {
      // these numbers are arbitrarily chosen primes.
      return submat.matrix_index +
          19553 * submat.row_offset +
          29297 * submat.num_rows +
          42209 * submat.col_offset +
          56527 * submat.num_cols;
    }
  };

  typedef std::vector<std::pair<int32, int32> > PairVectorType;
  struct PointersEqual {
    bool operator ()(const PairVectorType *ptr1,
                     const PairVectorType *ptr2) const {
      return *ptr1 == *ptr2;
    }
  };
  struct PairVectorHasher {
    size_t operator() (const std::vector<std::pair<int32, int32> > *arg) const {
      const size_t kPrime1 = 13, kPrime2 = 7853;
      size_t ans = arg->size();
      std::vector<std::pair<int32, int32> >::iterator
          iter = arg->begin(), end = arg->end();
      for (; iter != end; ++iter)
        ans = kPrime1 * ans + static_cast<size_t>(iter->first) +
            kPrime2 * static_cast<size_t>(iter->second);
      return ans;
    }
  };
  
  /// creates a renumbering that removes the elements in "to_remove",
  /// e.g. if old_num_elements = 3 and to_remove = [1], would output
  /// the vector [ 0, -1, 1 ].
  static void CreateRenumbering(int32 old_num_elements,
                                const std::vector<int32> &to_remove,
                                std::vector<int32> *renumbering);

  /// creates a renumbering from old to new index that removes the unused
  /// elements, e.g. if used == [ true, false, true, true], would output the
  /// vector [ 0, -1, 1, 2 ].  Returns number of new elements, i.e. the
  /// number of elements of 'used' that were true.
  static int32 CreateRenumbering(const std::vector<bool> &used,
                                 std::vector<int32> *renumbering);
  
  // vector of bool indexed by original submatrix-index, that is true if a
  // submatrix-index is used somewhere in the computation (always true for
  // the zeroth element).
  std::vector<bool> submatrix_is_used_;
  // vector of bool indexed by original submatrix-index, that is true if
  // a submatrix-index will be kept (like submatrix_is_used_; but for duplicate
  // submatrices, all but the first will be marked false).
  std::vector<bool> submatrix_is_used_;
  
  // vector of bool indexed by original-matrix-index > 0, that is true if a
  // matrix-index is used somewhere in the computation, directly or indirectly.
  // always true for the zeroth element.
  std::vector<bool> matrix_is_used_;
  NnetComputation *computation_;
  int32 num_matrices_new_;
  int32 num_submatrices_new_;
  std::vector<int32> old_to_new_matrix_; // numbered by orig-matrix-index, gives
                                         // new-matrix-index.  -1 for removed
                                         // ones.
  std::vector<int32> old_to_new_submatrix_; // numbered by orig-submatrix-index,
                                            // gives new-submatrix-index.  -1
                                            // for removed ones.
};


// We require that the computation have debug info set up
// (!matrix_debug_info.empty()) and that this be the first
// optimization you perform.  This means that the debug_info will
// be accurate and that all matrices will be initialized with
// zero contents.
class DeriativeTimeLimiter {

  DerivativeTimeLimiter(const Nnet &nnet,
                        NnetComputation *computation);


 private:
  const Nnet &nnet_;
  // the computation; we require it to have debug info set up
  // (otherwise you shouldn't be instantiating this class).
  NnetComputation *computation_;
  
  // is_nonzero_ is the same size as computation_->matrices, and contains a
  // nonempty vector for each row of each matrix m in the computation
  // (computation_->matrices) that (a) represents a derivative, (b) is not an
  // input to or output of the computation (i.e. which is not an output-deriv or
  // input-deriv), and (c) is
  struct MatrixPruneInfo {
    bool is_zero;  // True if the matrix is all zero (because it represents
                   // derivatives from outside the times we compute derivatives.
    bool is_limited;  // True if the matrix's rows are limited, i.e. nonzero
                      // only inside a range.  (if there are zeros within this
                      // range, which generally won't happen because we sort on
                      // the t value first, we just treat them as nonzero.
    int32 row_begin;  // if is_limited, the first row that's nonzero.
    int32 row_end;    // if is_limited, the last row that's nonzero, plus one.
  };

  std::vector<MatrixPruneInfo> prune_info_;

  // sets up prune_info_.
  void SetUpMatrixPruneInfo();

  // Modifies this command to take into account prune_info_.  At this point we
  // don't actually reduce the size of the matrices, we simply make the commands
  // operate on submatrices of the original matrices where possible- or
  // delete them completely if their output is all zeros.
  // Note: this calls computation_->NewSubMatrix, and will generate duplicates
  // of the same submatrix which we'll later remove in RemoveOrphanMatrices.
  void ModifyCommand(NnetComputation::Command *command);
};




/// This function detects submatrices, matrices and indexes-multi indexes that
/// are never used (e.g. due to changes made in other optimization code), and
/// removes them from the computation by way of suitable renumbering.
void RemoveOrphans(NnetComputation *computation);

/// Removes commands of type kNoOperation in the computation.
void RemoveNoOps(NnetComputation *computation);

/// Wherever matrix orig_matrix_index appears in the input of the network
/// (i.e. in computation->input_output_info), replaces it with new_matrix_index.
/// Returns true if it did replace it.
bool ReplaceInInput(
    const Nnet &nnet, int32 orig_matrix_index, int32 new_matrix_index,
    NnetComputation *computation);

/// A helper function used in some optimization functions.
/// Wherever matrix orig_matrix_index appears in the output of the network
/// (i.e. in computation->input_output_info), replaces it with new_matrix_index.
/// Returns true if it did replace it.
bool ReplaceInOutput(
    const Nnet &nnet, int32 orig_matrix_index, int32 new_matrix_index,
    NnetComputation *computation);

/// This function outputs to "submatrix_args" the addresses of a subset of
/// arguments arg1 through arg6 in "command", that correspond to the indexes of
/// submatrices.  This is useful in renumbering code.  Note: some of the
/// pointers may point to a zero value, for optional submatrix args.
void IdentifySubmatrixArgs(NnetComputation::Command *command,
                           std::vector<int32*> *submatrix_args);


/// This function outputs to "submatrix_args" the addresses of the args
/// (arguments arg1 through arg6) in the vector "commands", that correspond to
/// the indexes of submatrices.  This is useful in renumbering code.  Note: some
/// of the pointers may point to a zero value, for optional submatrix args.
void IdentifySubmatrixArgs(std::vector<NnetComputation::Command> *commands,
                           std::vector<int32*> *submatrix_args);


/// This function outputs to "submatrix_args" the addresses of integers in
/// 'computation' that correspond to submatrices.  These may be present in
/// 'commands', and in 'indexes_multi'.  This is useful in renumbering code.
/// Note: some of the pointers may point to a zero value, for optional submatrix
/// args in commands, but for efficiency we don't provide pointers for the -1's
/// in 'indexes_multi'.
void IdentifySubmatrixArgsInComputation(NnetComputation *computation,
                                        std::vector<int32*> *submatrix_args);


/// This function outputs to "matrix_args" the addresses of a subset of the
/// arguments arg1 through arg6 in "command", that correspond to the indexes of
/// matrices.  This is useful in renumbering code.  (Note: only a few types of
/// command use matrix indexes).
void IdentifyMatrixArgs(NnetComputation::Command *command,
                        std::vector<int32*> *matrix_args);

/// This function outputs to "matrix_args" the addresses of a subset of the
/// arguments arg1 through arg6 in commands in "commands", that correspond to
/// the indexes of matrices.  This is useful in renumbering code.  (Note: only a
/// few types of command use matrix indexes).
void IdentifyMatrixArgs(std::vector<NnetComputation::Command> *command,
                        std::vector<int32*> *matrix_args);

/// This function outputs to "matrix_args" the addresses of indexes inside
/// 'computation' that correspond to matrices.  These live inside
/// computation->commands, computation->submatrices, and
/// computation->input_output_info.  Zeros may be present if there were optional
/// arguments; we do include pointers to them, but you can just ignore them.
void IdentifyMatrixArgsInComputation(NnetComputation *computation,
                                     std::vector<int32*> *matrix_args);


/// Identifies in the vector of commands, arguments that correspond to indexes
/// into the computation's indexes_multi array, and outputs a list of pointers
/// to those arguments to 'indexes_multi_args'.  Useful in renumbering code.
void IdentifyIndexesMultiArgs(std::vector<NnetComputation::Command> *commands,
                              std::vector<int32*> *indexes_multi_args);




/*

   Possible TODO:
      optimizations to replace row-by-row copy and add commands with whole-matrix
      commands on smaller sub-matrices (if the row-by-row copy commands have certain
      regularities).  this is a minor issue, we can handle it later.  We have to be
      careful if this causes sub-matrices to overlap.

 */





} // namespace nnet3
} // namespace kaldi


#endif

