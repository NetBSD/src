//===-- X86TargetFrameLowering.h - Define frame lowering for X86 -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This class implements X86-specific bits of TargetFrameLowering class.
//
//===----------------------------------------------------------------------===//

#ifndef X86_FRAMELOWERING_H
#define X86_FRAMELOWERING_H

#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class MCSymbol;
class X86TargetMachine;

class X86FrameLowering : public TargetFrameLowering {
public:
  explicit X86FrameLowering(StackDirection D, unsigned StackAl, int LAO)
    : TargetFrameLowering(StackGrowsDown, StackAl, LAO) {}

  void emitCalleeSavedFrameMoves(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 DebugLoc DL) const;

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF) const override;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const override;

  void adjustForSegmentedStacks(MachineFunction &MF) const override;

  void adjustForHiPEPrologue(MachineFunction &MF) const override;

  void processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                     RegScavenger *RS = nullptr) const override;

  bool
  assignCalleeSavedSpillSlots(MachineFunction &MF,
                              const TargetRegisterInfo *TRI,
                              std::vector<CalleeSavedInfo> &CSI) const override;

  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const override;

  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MI,
                                  const std::vector<CalleeSavedInfo> &CSI,
                                  const TargetRegisterInfo *TRI) const override;

  bool hasFP(const MachineFunction &MF) const override;
  bool hasReservedCallFrame(const MachineFunction &MF) const override;

  int getFrameIndexOffset(const MachineFunction &MF, int FI) const override;
  int getFrameIndexReference(const MachineFunction &MF, int FI,
                             unsigned &FrameReg) const override;

  void eliminateCallFramePseudoInstr(MachineFunction &MF,
                                 MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI) const override;
};

} // End llvm namespace

#endif
