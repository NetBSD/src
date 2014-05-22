//===-- SIRegisterInfo.h - SI Register Info Interface ----------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Interface definition for SIRegisterInfo
//
//===----------------------------------------------------------------------===//


#ifndef SIREGISTERINFO_H_
#define SIREGISTERINFO_H_

#include "AMDGPURegisterInfo.h"

namespace llvm {

class AMDGPUTargetMachine;

struct SIRegisterInfo : public AMDGPURegisterInfo {
  AMDGPUTargetMachine &TM;

  SIRegisterInfo(AMDGPUTargetMachine &tm);

  virtual BitVector getReservedRegs(const MachineFunction &MF) const;

  virtual unsigned getRegPressureLimit(const TargetRegisterClass *RC,
                                       MachineFunction &MF) const;

  /// \param RC is an AMDIL reg class.
  ///
  /// \returns the SI register class that is equivalent to \p RC.
  virtual const TargetRegisterClass *
    getISARegClass(const TargetRegisterClass *RC) const;

  /// \brief get the register class of the specified type to use in the
  /// CFGStructurizer
  virtual const TargetRegisterClass * getCFGStructurizerRegClass(MVT VT) const;

  virtual unsigned getHWRegIndex(unsigned Reg) const;

  /// \brief Return the 'base' register class for this register.
  /// e.g. SGPR0 => SReg_32, VGPR => VReg_32 SGPR0_SGPR1 -> SReg_32, etc.
  const TargetRegisterClass *getPhysRegClass(unsigned Reg) const;

  /// \returns true if this class contains only SGPR registers
  bool isSGPRClass(const TargetRegisterClass *RC) const;

  /// \returns true if this class contains VGPR registers.
  bool hasVGPRs(const TargetRegisterClass *RC) const;

  /// \returns A VGPR reg class with the same width as \p SRC
  const TargetRegisterClass *getEquivalentVGPRClass(
                                          const TargetRegisterClass *SRC) const;

  /// \returns The register class that is used for a sub-register of \p RC for
  /// the given \p SubIdx.  If \p SubIdx equals NoSubRegister, \p RC will
  /// be returned.
  const TargetRegisterClass *getSubRegClass(const TargetRegisterClass *RC,
                                            unsigned SubIdx) const;
};

} // End namespace llvm

#endif // SIREGISTERINFO_H_
