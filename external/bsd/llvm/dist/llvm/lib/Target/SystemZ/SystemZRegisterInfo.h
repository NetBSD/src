//===-- SystemZRegisterInfo.h - SystemZ register information ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef SystemZREGISTERINFO_H
#define SystemZREGISTERINFO_H

#include "SystemZ.h"
#include "llvm/Target/TargetRegisterInfo.h"

#define GET_REGINFO_HEADER
#include "SystemZGenRegisterInfo.inc"

namespace llvm {

namespace SystemZ {
// Return the subreg to use for referring to the even and odd registers
// in a GR128 pair.  Is32Bit says whether we want a GR32 or GR64.
inline unsigned even128(bool Is32bit) {
  return Is32bit ? subreg_hl32 : subreg_h64;
}
inline unsigned odd128(bool Is32bit) {
  return Is32bit ? subreg_l32 : subreg_l64;
}
} // end namespace SystemZ

class SystemZSubtarget;
class SystemZInstrInfo;

struct SystemZRegisterInfo : public SystemZGenRegisterInfo {
private:
  SystemZTargetMachine &TM;

public:
  SystemZRegisterInfo(SystemZTargetMachine &tm);

  // Override TargetRegisterInfo.h.
  bool requiresRegisterScavenging(const MachineFunction &MF) const override {
    return true;
  }
  bool requiresFrameIndexScavenging(const MachineFunction &MF) const override {
    return true;
  }
  bool trackLivenessAfterRegAlloc(const MachineFunction &MF) const override {
    return true;
  }
  const MCPhysReg *getCalleeSavedRegs(const MachineFunction *MF = nullptr) const
    override;
  BitVector getReservedRegs(const MachineFunction &MF) const override;
  void eliminateFrameIndex(MachineBasicBlock::iterator MI,
                           int SPAdj, unsigned FIOperandNum,
                           RegScavenger *RS) const override;
  unsigned getFrameRegister(const MachineFunction &MF) const override;
};

} // end namespace llvm

#endif
