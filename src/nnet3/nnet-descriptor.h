// nnet3/nnet-descriptor.h

// Copyright   2012-2015  Johns Hopkins University (author: Daniel Povey)

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

#ifndef KALDI_NNET3_NNET_DESCRIPTOR_H_
#define KALDI_NNET3_NNET_DESCRIPTOR_H_

#include "base/kaldi-common.h"
#include "util/kaldi-io.h"
#include "matrix/matrix-lib.h"
#include "nnet3/nnet-common.h"
#include "nnet3/nnet-component-itf.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <map>


namespace kaldi {
namespace nnet3 {

/**
   \file nnet-descriptor.h

   This file contains class definitions for classes ForwardingDescriptor,
   SumDescriptor and InputDescriptor.  Basically this is code that specifies how
   we glue together the outputs of possibly several other network-nodes, as the
   input of a particular network node (or as an output of the network).  In the
   neural-network code we refer to the top-level descriptor which is
   InputDescriptor.  The InputDescriptor is a concatenation features; each part
   is a SumDescriptor.  The SumDescriptor is a summation over a set of features
   of all the same dimension, each of which is represented by a
   ForwardingDescriptor.  A ForwardingDescriptor in the simplest case just
   takes just points you to a particular network node, but in general can do
   things like adding time offsets, and selecting different rows of its matrix
   from different inputs.  Unlike the other descriptors, a ForwardingDescriptor
   is in general a bit like a parse tree, in that it can in general contain
   other ForwardingDescriptors.
 */


// forward declaration
class ForwardingDescriptorImpl;

// A ForwardingDescriptor describes how we copy data from another NetworkNode,
// or from multiple other NetworkNodes.  In the simplest case this can just be
// equivalent to giving the name of another NetworkNode, but we also support
// things like time-offsets, selecting depending on the index from multiple
// different inputs, and things like that.  Internally we use pointer to an
// implementation class; we do it this way so that we can make a std::vector of
// this class and things like copying and assignment will work correctly without
// having to worry about memory allocation: we can't make a std::vector of an
// abstract base-class.
// note: nodes of type kOutput (i.e. output nodes of the network) cannot appear
// as inputs in any descriptor.  This is to simplify compilation.
class ForwardingDescriptor {
 public:
  // Given an Index that's requested at the input of this Component (or at
  // an output of the network), maps it to a (node_index, Index) pair that
  // says where we are to get the data from.
  Cindex MapToInput(const Index &output) const;
  int32 Dim(const Nnet &nnet) const;

  ForwardingDescriptor & operator= (const ForwardingDescriptor &other);
  ForwardingDescriptor (const ForwardingDescriptor &other);
  ForwardingDescriptor(): impl_(NULL) { }

  // This function appends to "node_indexes" a list (not necessarily sorted or
  // unique) of all the node indexes that this descriptor may forward data from.
  void ComputeDependencies(std::vector<int32> *node_indexes) const;

  // This function is for use in things like clockwork RNNs, where shifting the
  // time of the inputs and outputs of the network by some multiple integer n
  // would leave things the same, but shifting by non-multiples would change the
  // network structure.  It returns the smallest modulus of the time-index t to
  // which this descriptor is invariant; the lowest common multiple of all
  // descriptors in the network gives you the modulus for the whole network.
  int32 Modulus() const;

  // The Parse method is used for reading a config-file-style represenation.
  // the is will correspond to one line of the config file (one NetworkNode), as
  // we need to figure out all node names before we read any of them.
  static void Parse(std::istringstream &is,
                    const std::vector<std::string> &node_names);
  // Write in config-file format.
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);
 private:
  // Everything happens in the implementation class.  We structure it this way
  // so we can make ForwardingDescriptor part of a std::vector (so it can't
  // be an abstract class).
  ForwardingDescriptorImpl *impl_;
};


// Abstract base-class for implementation of class ForwardingDescriptor (see
// its declaration for an explanation of what it is).
class ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &output) const;
  virtual int32 Dim(const Nnet &nnet) const;
  virtual ForwardingDescriptorImpl *Copy() const;

  // The Parse method is used for reading a config-file-style represenation.
  // the "is" will correspond to one line of the config file (one NetworkNode),
  // as we need to figure out all node names before we read any of them.
  static ForwardingDescriptorImpl *Parse(std::istringstream &is,
                                         const std::vector<std::string> &node_names);

  /// This function is for use in things like clockwork RNNs, where shifting the
  /// time of the inputs and outputs of the network by some multiple integer n
  /// would leave things the same, but shifting by non-multiples would change the
  /// network structure.  It returns the smallest modulus to which this
  /// descriptor is invariant; the lowest common multiple of all descriptors in
  /// the network gives you the modulus for the whole network.
  virtual int32 Modulus() const { return 1; }

  // Write to string that will be one line of a config-file-like format.  The
  // opposite of Parse.
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  /// This function appends to "node_indexes" all the node indexes
  // that this descriptor may access.
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;
  
  virtual ~ForwardingDescriptorImpl();
  ForwardingDescriptorImpl() { }
 private:
  KALDI_DISALLOW_COPY_AND_ASSIGN(ForwardingDescriptorImpl);
};

// SimpleForwardingDescriptor is the base-case of ForwardingDescriptor,
class SimpleForwardingDescriptorImpl: public ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &ind) { return Cindex(src_node_, ind); }
  virtual int32 Dim(const Nnet &nnet) const;
  virtual ForwardingDescriptorImpl *Copy() const;
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;

  
  // Write to string that will be one line of a config-file-like format.  The
  // opposite of Parse.
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  SimpleForwardingDescriptorImpl(int32 src_node): src_node_(src_node) {
    KALDI_ASSERT(src_node >= 0);
  }
  virtual ~SimpleForwardingDescriptorImpl();
 private:
  int32 src_node_;  // index of the source NetworkNode.
};

class OffsetForwardingDescriptorImpl: public ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &ind) {
    Cindex answer = src_->MapToInput(ind);
    answer.second = answer.second + offset_;
    return answer;
  }
  virtual int32 Dim(const Nnet &nnet) const { return src_->Dim(nnet); }
  virtual ForwardingDescriptorImpl *Copy() const;

  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  virtual int32 Modulus() const { return src_->Modulus(); }
  
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;
  
  // takes ownership of src.
  OffsetForwardingDescriptorImpl(ForwardingDescriptorImpl *src,
                                 Index offset): src_(src), offset_(offset) { }
  
  virtual ~OffsetForwardingDescriptorImpl();
 private:
  ForwardingDescriptorImpl *src_;  // Owned here.
  Index offset_;  // The index-offset to be added to the index.
};

// Chooses from different inputs based on the the time index modulo
// (the number of ForwardingDescriptors given as inputs).
class SwitchingForwardingDescriptorImpl: public ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &ind) {
    KALDI_ASSERT(!src_.empty());
    int32 size = src_.size(), mod = ind.t % size;
    // next line gets "mathematical" modulus, not broken "C" modulus.
    if (mod < 0) mod += size;
    return src_[mod]->MapToInput(ind);
  }
  virtual int32 Dim(const Nnet &nnet) const { return src_[0]->Dim(nnet); }
  virtual ForwardingDescriptorImpl *Copy() const;
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  virtual int32 Modulus() const;
  
  /// This function appends to "node_indexes" all the node indexes
  // that this descriptor may access.
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;

  // takes ownership of items in src.
  SwitchingForwardingDescriptorImpl(std::vector<ForwardingDescriptorImpl*> &src):
      src_(src) { }
  virtual ~SwitchingForwardingDescriptorImpl();
 private:
  // Pointers are owned here.
  std::vector<ForwardingDescriptorImpl*> src_; 
};


// This forwarding-descriptor uses the x value of the Index to choose from one
// of multiple argument ForwardingDescriptorImpl's.  That is, suppose it is
// initialized with two ForwardingDescriptorImpl's, it treats the x value as an
// index into that array of ForwardingDescriptorImpl's; and it sets that x value
// to zero before passing it to the chosen ForwardingDescriptorImpl.  This can
// be used as a mechanism for the Component to switch between multiple different
// inputs.
// We chose the x index for this purpose as it's not used in most setups.
class SelectForwardingDescriptorImpl: public ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &ind_in) {
    Index ind(ind_in);
    KALDI_ASSERT(!src_.empty());
    int32 size = src_.size(), x = ind.x;
    KALDI_ASSERT(x >= 0 && x < size);
    ind.x = 0;
    return src_[x]->MapToInput(ind);
  }
  virtual int32 Dim(const Nnet &nnet) const { return src_[0]->Dim(nnet); }
  virtual ForwardingDescriptorImpl *Copy() const;
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  /// This function appends to "node_indexes" all the node indexes
  // that this descriptor may access.
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;
  
  // takes ownership of items in src.
  SelectForwardingDescriptorImpl(std::vector<ForwardingDescriptorImpl*> &src):
      src_(src) { }
  virtual ~SelectForwardingDescriptorImpl();
 private:
  // Pointers are owned here.
  std::vector<ForwardingDescriptorImpl*> src_; 
};



/// For use in clockwork RNNs and the like, this forwarding-descriptor
/// rounds the time-index t down to the the closest t' <= t that is
/// an exact multiple of t_modulus_.
class RoundingForwardingDescriptorImpl: public ForwardingDescriptorImpl {
 public:
  virtual Cindex MapToInput(const Index &ind) {
    KALDI_ASSERT(t_modulus_ >= 1);
    Cindex ans = src_->MapToInput(ind);
    int32 mod = ans.second.t % t_modulus_;
    if (mod < 0)
      mod += t_modulus_;
    ans.second.t -= mod;
    return ans;
  }
  virtual int32 Dim(const Nnet &nnet) const { return src_->Dim(nnet); }
  virtual ForwardingDescriptorImpl *Copy() const;
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  virtual int32 Modulus() const { return t_modulus_; }

  /// This function appends to "node_indexes" all the node indexes
  // that this descriptor may access.
  virtual void ComputeDependencies(std::vector<int32> *node_indexes) const;

  // takes ownership of items in src.
  RoundingForwardingDescriptorImpl(int32 t_modulus,
                                   ForwardingDescriptorImpl *src):
      t_modulus_(t_modulus), src_(src) { }

  virtual ~RoundingForwardingDescriptorImpl() { delete src_; }
 private:
  int32 t_modulus_;
  ForwardingDescriptorImpl *src_;
};


// A SumDescriptor sums over its terms.  In a valid SumDescriptor,
// "terms" will be nonempty and they will all have the same dimension.
struct SumDescriptor {
  int32 Dim(const Nnet &nnet) const;

  // The Parse method is used for reading a config-file-style represenation.
  // the is will correspond to one line of the config file (one NetworkNode), as
  // we need to figure out all node names before we read any of them.
  static void Parse(std::istringstream &is,
                    const std::vector<std::string> &node_names);
  // Write in config-file format.
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  // This function appends to "node_indexes" a list (not necessarily sorted or
  // unique) of all the node indexes that this descriptor may forward data from.
  void ComputeDependencies(std::vector<int32> *node_indexes) const;

  // see Modulus function of ForwardingDescriptor for explanation.
  int32 Modulus() const;
  
  std::vector<ForwardingDescriptor> terms;
};

// A Descriptor concatenates over its parts, so its feature-dimension will
// be the sum of the feature-dimensions of its parts.  In a valid Descriptor,
// "parts" will be nonempty.  Each part may be (in general) a summation, but
// usually a summation with just one term.
struct Descriptor {
  int32 Dim(const Nnet &nnet) const;

  // The Parse method is used for reading a config-file-style represenation.
  // the is will correspond to one line of the config file (one NetworkNode), as
  // we need to figure out all node names before we read any of them.
  static void Parse(std::istringstream &is,
                    const std::vector<std::string> &node_names);
  // Write in config-file format.
  static void WriteConfig(std::ostream &is,
                          const std::vector<std::string> &node_names);

  // This function appends to "node_indexes" a list (not necessarily sorted or
  // unique) of all the node indexes that this descriptor may forward data from.
  void ComputeDependencies(std::vector<int32> *node_indexes) const;

  // see Modulus function of ForwardingDescriptor for explanation.
  int32 Modulus() const;
  
  // This function gets all Cindexes that are required to compute this index.
  // Used for computing dependencies when constructing ComputationGraph.
  // This list is *not guaranteed unique*, i.e. it may contain repeats.
  // The caller has to deal with this.
  void GetInputCindexes(const Index &indexes,
                        std::vector<Cindex> *required_indexes) const;
  
  std::vector<SumDescriptor> parts;
};




} // namespace nnet3
} // namespace kaldi

#endif
