// latbin/lattice-determinize-non-compact.cc

// Copyright 2009-2012  Microsoft Corporation
//           2012-2013  Johns Hopkins University (Author: Daniel Povey)
//           2015  Vimal Manohar

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


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "util/stl-utils.h"
#include "fstext/fstext-lib.h"
#include "lat/kaldi-lattice.h"
#include "lat/lattice-functions.h"
#include "lat/push-lattice.h"
#include "lat/minimize-lattice.h"

namespace kaldi {

typedef Lattice::StateId StateId;
typedef Lattice::Arc Arc;

// This function is a copy of the function in the program lattice-determinize
bool DeterminizeLatticeWrapper(const Lattice &lat,
                               const std::string &key,
                               bool prune,
                               BaseFloat beam,
                               BaseFloat beam_ratio,
                               int32 max_mem,
                               int32 max_loop,
                               BaseFloat delta,
                               int32 num_loops,
                               CompactLattice *clat) {
  fst::DeterminizeLatticeOptions lat_opts;
  lat_opts.max_mem = max_mem;
  lat_opts.max_loop = max_loop;
  lat_opts.delta = delta;
  BaseFloat cur_beam = beam;
  for (int32 i = 0; i < num_loops;) { // we increment i below.

    if (lat.Start() == fst::kNoStateId) {
      KALDI_WARN << "Detected empty lattice, skipping " << key;
      return false;
    }
    
    // The work gets done in the next line.  
    if (DeterminizeLattice(lat, clat, lat_opts, NULL)) { 
      if (prune) PruneLattice(cur_beam, clat);
      return true;
    } else { // failed to determinize..
      KALDI_WARN << "Failed to determinize lattice (presumably max-states "
                 << "reached), reducing lattice-beam to "
                 << (cur_beam*beam_ratio) << " and re-trying.";
      for (; i < num_loops; i++) {
        cur_beam *= beam_ratio;
        Lattice pruned_lat(lat);
        PruneLattice(cur_beam, &pruned_lat);
        if (NumArcs(lat) == NumArcs(pruned_lat)) {
          cur_beam *= beam_ratio;
          KALDI_WARN << "Pruning did not have an effect on the original "
                     << "lattice size; reducing beam to "
                     << cur_beam << " and re-trying.";
        } else if (DeterminizeLattice(pruned_lat, clat, lat_opts, NULL)) {
          if (prune) PruneLattice(cur_beam, clat);
          return true;
        } else {
          KALDI_WARN << "Determinization failed again; reducing beam again to "
                     << (cur_beam*beam_ratio) << " and re-trying.";
        }
      }
    }
  }
  KALDI_WARN << "Decreased pruning beam --num-loops=" << num_loops
             << " times and was not able to determinize: failed for "
             << key;
  return false;
}

void ComputeAcousticScoresMap(
    const Lattice &lat, 
    unordered_map<std::pair<int32, int32>, std::pair<BaseFloat, int32>, 
                                        PairHasher<int32> > *acoustic_scores) {
  acoustic_scores->clear();

  std::vector<int32> state_times;
  LatticeStateTimes(lat, &state_times);
  
  KALDI_ASSERT(lat.Start() == 0);

  for (StateId s = 0; s < lat.NumStates(); s++) {
    int32 t = state_times[s];
    for (fst::ArcIterator<Lattice> aiter(lat, s); !aiter.Done();
          aiter.Next()) {
      const Arc &arc = aiter.Value();
      const LatticeWeight &weight = arc.weight;

      int32 tid = arc.ilabel;

      if (tid != 0) {
        unordered_map<std::pair<int32, int32>, std::pair<BaseFloat, int32>, 
          PairHasher<int32> >::iterator it = acoustic_scores->find(std::make_pair(t, tid));
        if (it == acoustic_scores->end()) {
          acoustic_scores->insert(std::make_pair(std::make_pair(t, tid), 
                                          std::make_pair(weight.Value2(), 1)));
        } else {
          if (it->second.second == 2 
                && it->second.first / it->second.second != weight.Value2()) {
            KALDI_VLOG(2) << "Transitions on the same frame have different "
                          << "acoustic costs for tid " << tid << "; " 
                          << it->second.first / it->second.second 
                          << " vs " << weight.Value2();
          }
          it->second.first += weight.Value2();
          it->second.second++;
        }
      } else {
        // Arcs with epsilon input label (tid) must have 0 acoustic cost
        KALDI_ASSERT(weight.Value2() == 0);
      }
    }

    LatticeWeight f = lat.Final(s);
    if (f != LatticeWeight::Zero()) {
      // Final acoustic cost must be 0 as we are reading from 
      // non-determinized, non-compact lattice
      KALDI_ASSERT(f.Value2() == 0.0);
    }
  }
}

void ReplaceAcousticScoresFromMap(
    const unordered_map<std::pair<int32, int32>, std::pair<BaseFloat, int32>, 
                                        PairHasher<int32> > &acoustic_scores,
    Lattice *lat) {
  fst::TopSort(lat);
  
  std::vector<int32> state_times;
  LatticeStateTimes(*lat, &state_times);
  
  KALDI_ASSERT(lat->Start() == 0);

  for (StateId s = 0; s < lat->NumStates(); s++) {
    int32 t = state_times[s];
    for (fst::MutableArcIterator<Lattice> aiter(lat, s); 
          !aiter.Done(); aiter.Next()) {
      Arc arc(aiter.Value());
 
      int32 tid = arc.ilabel;
      if (tid != 0) {
        unordered_map<std::pair<int32, int32>, std::pair<BaseFloat, int32>, 
          PairHasher<int32> >::const_iterator it = acoustic_scores.find(std::make_pair(t, tid));
        if (it == acoustic_scores.end()) {
          KALDI_ERR << "Could not find tid " << tid << " at time " << t
                    << " in the acoustic scores map.";
        } else {
          arc.weight.SetValue2(it->second.first / it->second.second);
        }
      } else {
        // For epsilon arcs, set acoustic cost to 0.0
        arc.weight.SetValue2(0.0);
      }
      aiter.SetValue(arc);
    }

    LatticeWeight f = lat->Final(s);
    if (f != LatticeWeight::Zero()) {
      // Set final acoustic cost to 0.0
      f.SetValue2(0.0);
      lat->SetFinal(s, f);
    }
  }
}

}

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;
    using fst::SymbolTable;
    using fst::VectorFst;
    using fst::StdArc;

    const char *usage =
        "lattice-determinize lattices (and apply a pruning beam)\n"
        " (see http://kaldi.sourceforge.net/lattices.html for more explanation)\n"
        "This version of the program retains the original "
        "acoustic scores of arcs in the determinized lattice and writes it "
        "as a normal (non-compact) lattice. \n"
        " note: this program is tyically only useful if you generated state-level\n"
        " lattices, e.g. called gmm-latgen-simple with --determinize=false\n"
        "\n"
        "Usage: lattice-determinize-non-compact [options] lattice-rspecifier lattice-wspecifier\n"
        " e.g.: lattice-determinize-non-compact --acoustic-scale=0.1 --beam=15.0 ark:1.lats ark:det.lats\n";
      
    ParseOptions po(usage);
    BaseFloat acoustic_scale = 1.0;
    BaseFloat beam = 10.0;
    BaseFloat beam_ratio = 0.9;
    int32 num_loops = 20;
    int32 max_mem = 50000000; // 50 MB
    int32 max_loop = 500000;
    BaseFloat delta = fst::kDelta;
    bool prune = false;
    bool minimize = false;
    
    po.Register("acoustic-scale", &acoustic_scale,
                "Scaling factor for acoustic likelihoods");
    po.Register("beam", &beam,
                "Pruning beam [applied after acoustic scaling]-- also used "
                "to handle determinization failures, set --prune=false to "
                "disable routine pruning");
    po.Register("delta", &delta, "Tolerance used in determinization");
    po.Register("prune", &prune, "If true, prune determinized lattices "
                "with the --beam option.");
    po.Register("max-mem", &max_mem, "Maximum approximate memory usage in "
                "determinization (real usage might be many times this)");
    po.Register("max-loop", &max_loop, "Option to detect a certain "
                "type of failure in lattice determinization (not critical)");
    po.Register("beam-ratio", &beam_ratio, "Ratio by which to "
                "decrease beam if we reach the max-arcs.");
    po.Register("num-loops", &num_loops, "Number of times to "
                "decrease beam by beam-ratio if determinization fails.");
    po.Register("minimize", &minimize,
                "If true, push and minimize after determinization");
    
    po.Read(argc, argv);

    if (po.NumArgs() != 2) {
      po.PrintUsage();
      exit(1);
    }

    std::string lats_rspecifier = po.GetArg(1),
        lats_wspecifier = po.GetArg(2);

    // Read as regular lattice-- this is the form we need it in for efficient
    // pruning.
    SequentialLatticeReader lattice_reader(lats_rspecifier);
    
    // Write as regular lattice.
    LatticeWriter lattice_writer(lats_wspecifier); 

    int32 n_done = 0, n_error = 0;

    if (acoustic_scale == 0.0)
      KALDI_ERR << "Do not use a zero acoustic scale (cannot be inverted)";
    LatticeWeight beam_weight(beam, static_cast<BaseFloat>(0.0));

    for (; !lattice_reader.Done(); lattice_reader.Next()) {
      std::string key = lattice_reader.Key();
      Lattice lat = lattice_reader.Value();
      
      lattice_reader.FreeCurrent();
      
      fst::TopSort(&lat);
      
      fst::ScaleLattice(fst::AcousticLatticeScale(acoustic_scale), &lat);


      // Compute a map from each (t, tid) to (sum_of_acoustic_scores, count) 
      unordered_map<std::pair<int32,int32>, std::pair<BaseFloat, int32>, 
                                          PairHasher<int32> > acoustic_scores;
      ComputeAcousticScoresMap(lat, &acoustic_scores);
      
      Invert(&lat); // make it so word labels are on the input.
      
      CompactLattice clat;
      if (DeterminizeLatticeWrapper(lat, key, prune,
                                    beam, beam_ratio, max_mem, max_loop,
                                    delta, num_loops, &clat)) {
        if (minimize) {
          PushCompactLatticeStrings(&clat);
          PushCompactLatticeWeights(&clat);
          MinimizeCompactLattice(&clat);
        }

        Lattice out_lat;
        fst::ConvertLattice(clat, &out_lat);
        fst::TopSort(&out_lat);

        // Replace each arc (t, tid) with the averaged acoustic score from
        // the computed map
        ReplaceAcousticScoresFromMap(acoustic_scores, &out_lat);

        fst::ScaleLattice(fst::AcousticLatticeScale(1.0/acoustic_scale), 
                          &out_lat);
        lattice_writer.Write(key, out_lat);
        n_done++;
      } else {
        n_error++; // will have already printed warning.
      }
    }

    KALDI_LOG << "Done " << n_done << " lattices, errors on " << n_error;
    return (n_done != 0 ? 0 : 1);
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return -1;
  }
}

