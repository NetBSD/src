//===-- X86ShuffleDecodeConstantPool.h - X86 shuffle decode -----*-C++-*---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Define several functions to decode x86 specific shuffle semantics using
// constants from the constant pool.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_X86SHUFFLEDECODECONSTANTPOOL_H
#define LLVM_LIB_TARGET_X86_X86SHUFFLEDECODECONSTANTPOOL_H

#include "llvm/ADT/SmallVector.h"

//===----------------------------------------------------------------------===//
//  Vector Mask Decoding
//===----------------------------------------------------------------------===//

namespace llvm {
class Constant;
class MVT;

/// \brief Decode a PSHUFB mask from an IR-level vector constant.
void DecodePSHUFBMask(const Constant *C, SmallVectorImpl<int> &ShuffleMask);

/// \brief Decode a VPERMILP variable mask from an IR-level vector constant.
void DecodeVPERMILPMask(const Constant *C, unsigned ElSize,
                        SmallVectorImpl<int> &ShuffleMask);

/// \brief Decode a VPERM W/D/Q/PS/PD mask from an IR-level vector constant.
void DecodeVPERMVMask(const Constant *C, MVT VT,
                      SmallVectorImpl<int> &ShuffleMask);

/// \brief Decode a VPERMT2 W/D/Q/PS/PD mask from an IR-level vector constant.
void DecodeVPERMV3Mask(const Constant *C, MVT VT,
                       SmallVectorImpl<int> &ShuffleMask);

} // llvm namespace

#endif
