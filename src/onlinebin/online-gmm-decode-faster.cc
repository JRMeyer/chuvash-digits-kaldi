// onlinebin/online-gmm-decode-faster.cc

// Copyright 2012 Cisco Systems (author: Matthias Paulik)

//   Modifications to the original contribution by Cisco Systems made by:
//   Vassil Panayotov

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

#include "feat/feature-mfcc.h"
#include "online/online-audio-source.h"
#include "online/online-feat-input.h"
#include "online/online-decodable.h"
#include "online/online-faster-decoder.h"
#include "online/onlinebin-util.h"


int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    using namespace fst;

    typedef kaldi::int32 int32;
    typedef OnlineFeInput<OnlinePaSource, Mfcc> FeInput;

    // Up to delta-delta derivative features are calculated (unless LDA is used)
    const int32 kDeltaOrder = 2;
    // Input sampling frequency is fixed to 16KHz
    const int32 kSampleFreq = 16000;
    // PortAudio's internal ring buffer size in bytes
    const int32 kPaRingSize = 32768;
    // Report interval for PortAudio buffer overflows in number of feat. batches
    const int32 kPaReportInt = 4;

    const char *usage =
        "Decode speech, using microphone input(PortAudio)\n\n"
        "Utterance segmentation is done on-the-fly.\n"
        "Feature splicing/LDA transform is used, if the optional(last) argument "
        "is given.\n"
        "Otherwise delta/delta-delta(2-nd order) features are produced.\n\n"
        "Usage: ./online-wav-gmm-decode-faster [options] model-in"
        "fst-in word-symbol-table silence-phones [lda-matrix-in]\n\n"
        "Example: ./online-wav-gmm-decode-faster --rt-min=0.3 --rt-max=0.5 "
        "--max-active=4000 --beam=12.0 --acoustic-scale=0.0769 "
        "model HCLG.fst words.txt '1:2:3:4:5' lda-matrix";
    ParseOptions po(usage);
    BaseFloat acoustic_scale = 0.1;
    int32 cmn_window = 600;
    int32 right_context = 4, left_context = 4;

    OnlineFasterDecoderOpts decoder_opts;
    decoder_opts.Register(&po, true);
    po.Register("left-context", &left_context, "Number of frames of left context");
    po.Register("right-context", &right_context, "Number of frames of right context");
    po.Register("acoustic-scale", &acoustic_scale,
                "Scaling factor for acoustic likelihoods");
    po.Register("cmn-window", &cmn_window,
        "Number of feat. vectors used in the running average CMN calculation");
    po.Read(argc, argv);
    if (po.NumArgs() != 4 && po.NumArgs() != 5) {
      po.PrintUsage();
      return 1;
    }
    if (po.NumArgs() == 4)
      if (left_context % kDeltaOrder != 0 || left_context != right_context)
        KALDI_ERR << "Invalid left/right context parameters!";

    std::string model_rxfilename = po.GetArg(1),
        fst_rxfilename = po.GetArg(2),
        word_syms_filename = po.GetArg(3),
        silence_phones_str = po.GetArg(4),
        lda_mat_rspecifier = po.GetOptArg(5);

    Matrix<BaseFloat> lda_transform;
    if (lda_mat_rspecifier != "") {
      bool binary_in;
      Input ki(lda_mat_rspecifier, &binary_in);
      lda_transform.Read(ki.Stream(), binary_in);
    }

    std::vector<int32> silence_phones;
    if (!SplitStringToIntegers(silence_phones_str, ":", false, &silence_phones))
        KALDI_ERR << "Invalid silence-phones string " << silence_phones_str;
    if (silence_phones.empty())
        KALDI_ERR << "No silence phones given!";

    TransitionModel trans_model;
    AmDiagGmm am_gmm;
    {
        bool binary;
        Input ki(model_rxfilename, &binary);
        trans_model.Read(ki.Stream(), binary);
        am_gmm.Read(ki.Stream(), binary);
    }

    fst::SymbolTable *word_syms = NULL;
    if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "
                    << word_syms_filename;

    fst::Fst<fst::StdArc> *decode_fst = ReadDecodeGraph(fst_rxfilename);

    // We are not properly registering/exposing MFCC and frame extraction options,
    // because there are parts of the online decoding code, where some of these
    // options are hardwired(ToDo: we should fix this at some point)
    MfccOptions mfcc_opts;
    mfcc_opts.use_energy = false;
    int32 frame_length = mfcc_opts.frame_opts.frame_length_ms = 25;
    int32 frame_shift = mfcc_opts.frame_opts.frame_shift_ms = 10;

    int32 feat_dim;
    int32 window_size = right_context + left_context + 1;
    decoder_opts.batch_size = std::max(decoder_opts.batch_size, window_size);
    OnlineFasterDecoder decoder(*decode_fst, decoder_opts,
                                silence_phones, trans_model);
    VectorFst<LatticeArc> out_fst;
    OnlinePaSource au_src(kSampleFreq, kPaRingSize, kPaReportInt);
    Mfcc mfcc(mfcc_opts);
    FeInput fe_input(&au_src, &mfcc,
                     frame_length * (kSampleFreq / 1000),
                     frame_shift * (kSampleFreq / 1000));
    OnlineCmvnInput cmvn_input(&fe_input, mfcc_opts.num_ceps, cmn_window);
    OnlineFeatInputItf *feat_transform = 0;
    if (lda_mat_rspecifier != "") {
      feat_transform = new OnlineLdaInput(
                               &cmvn_input, mfcc_opts.num_ceps, lda_transform,
                               left_context, right_context);
      feat_dim = lda_transform.NumRows();
    } else {
      feat_transform = new OnlineDeltaInput(&cmvn_input, mfcc_opts.num_ceps,
                                            kDeltaOrder, left_context / 2);
      feat_dim = (kDeltaOrder + 1) * mfcc_opts.num_ceps;
    }
    OnlineDecodableDiagGmmScaled decodable(feat_transform, am_gmm, trans_model,
                                           acoustic_scale, decoder_opts.batch_size,
                                           feat_dim, -1);
    bool partial_res = false;
    while (1) {
      OnlineFasterDecoder::DecodeState dstate = decoder.Decode(&decodable);
      if (dstate & (decoder.kEndFeats | decoder.kEndUtt)) {
        std::vector<int32> word_ids;
        decoder.FinishTraceBack(&out_fst);
        fst::GetLinearSymbolSequence(out_fst,
                                     static_cast<vector<int32> *>(0),
                                     &word_ids,
                                     static_cast<LatticeArc::Weight*>(0));
        PrintPartialResult(word_ids, word_syms, partial_res || word_ids.size());
        partial_res = false;
      } else {
        std::vector<int32> word_ids;
        if (decoder.PartialTraceback(&out_fst)) {
          fst::GetLinearSymbolSequence(out_fst,
                                       static_cast<vector<int32> *>(0),
                                       &word_ids,
                                       static_cast<LatticeArc::Weight*>(0));
          PrintPartialResult(word_ids, word_syms, false);
          if (!partial_res)
            partial_res = (word_ids.size() > 0);
        }
      }
    }

    if (feat_transform) delete feat_transform;
    if (word_syms) delete word_syms;
    if (decode_fst) delete decode_fst;
    return 0;
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()
