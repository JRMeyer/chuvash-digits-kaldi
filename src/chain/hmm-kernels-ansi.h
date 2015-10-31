// ctc/cctc-kernels-ansi.h

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


#ifndef KALDI_CTC_CCTC_KERNELS_ANSI_H_
#define KALDI_CTC_CCTC_KERNELS_ANSI_H_
#include "ctc/cctc-datastruct.h"

#if HAVE_CUDA == 1
extern "C" {

void cudaF_rearrange_3d_tensor(dim3 Gr, dim3 Bl, int32_cuda xdim,
                               int32_cuda xstride_in, int32_cuda ystride_in,
                               int32_cuda zstride_in, int32_cuda xstride_out,
                               int32_cuda ystride_out, int32_cuda zstride_out,
                               const float *src, float *dst);

void cudaD_rearrange_3d_tensor(dim3 Gr, dim3 Bl, int32_cuda xdim,
                               int32_cuda xstride_in, int32_cuda ystride_in,
                               int32_cuda zstride_in, int32_cuda xstride_out,
                               int32_cuda ystride_out, int32_cuda zstride_out,
                               const double *src, double *dst);


void cuda_ctc_hmm_backward(dim3 Gr, dim3 Bl,
                           const Int32Pair *forward_transitions,
                           const CctcHmmTransition *transitions,
                           int32_cuda t, int32_cuda num_sequences,
                           int32_cuda special_hmm_state,
                           const BaseFloat *num_probs, const BaseFloat *den_probs,
                           const BaseFloat *this_alpha, const BaseFloat *next_beta,
                           BaseFloat *this_beta,
                           BaseFloat *log_num_deriv, BaseFloat *den_deriv);

void cuda_ctc_hmm_forward(dim3 Gr, dim3 Bl,
                          const Int32Pair *backward_transitions,
                          const CctcHmmTransition *transitions,
                          int32_cuda t, int32_cuda num_sequences,
                          int32_cuda special_hmm_state,
                          const BaseFloat *num_probs,
                          const BaseFloat *den_probs,
                          const BaseFloat *prev_alpha,
                          BaseFloat *this_alpha);

} // extern "C"

#endif  // HAVE_CUDA


#endif  // KALDI_CTC_CCTC_KERNELS_ANSI_H_
