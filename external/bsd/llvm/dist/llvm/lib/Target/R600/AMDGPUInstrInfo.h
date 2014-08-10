//===-- AMDGPUInstrInfo.h - AMDGPU Instruction Information ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Contains the definition of a TargetInstrInfo class that is common
/// to all AMD GPUs.
//
//===----------------------------------------------------------------------===//

#ifndef AMDGPUINSTRUCTIONINFO_H
#define AMDGPUINSTRUCTIONINFO_H

#include "AMDGPUInstrInfo.h"
#include "AMDGPURegisterInfo.h"
#include "llvm/Target/TargetInstrInfo.h"
#include <map>

#define GET_INSTRINFO_HEADER
#define GET_INSTRINFO_ENUM
#define GET_INSTRINFO_OPERAND_ENUM
#include "AMDGPUGenInstrInfo.inc"

#define OPCODE_IS_ZERO_INT AMDGPU::PRED_SETE_INT
#define OPCODE_IS_NOT_ZERO_INT AMDGPU::PRED_SETNE_INT
#define OPCODE_IS_ZERO AMDGPU::PRED_SETE
#define OPCODE_IS_NOT_ZERO AMDGPU::PRED_SETNE

namespace llvm {

class AMDGPUTargetMachine;
class MachineFunction;
class MachineInstr;
class MachineInstrBuilder;

class AMDGPUInstrInfo : public AMDGPUGenInstrInfo {
private:
  const AMDGPURegisterInfo RI;
  bool getNextBranchInstr(MachineBasicBlock::iterator &iter,
                          MachineBasicBlock &MBB) const;
  virtual void anchor();
protected:
  TargetMachine &TM;
public:
  explicit AMDGPUInstrInfo(TargetMachine &tm);

  virtual const AMDGPURegisterInfo &getRegisterInfo() const = 0;

  bool isCoalescableExtInstr(const MachineInstr &MI, unsigned &SrcReg,
                             unsigned &DstReg, unsigned &SubIdx) const override;

  unsigned isLoadFromStackSlot(const MachineInstr *MI,
                               int &FrameIndex) const override;
  unsigned isLoadFromStackSlotPostFE(const MachineInstr *MI,
                                     int &FrameIndex) const override;
  bool hasLoadFromStackSlot(const MachineInstr *MI,
                            const MachineMemOperand *&MMO,
                            int &FrameIndex) const override;
  unsigned isStoreFromStackSlot(const MachineInstr *MI, int &FrameIndex) const;
  unsigned isStoreFromStackSlotPostFE(const MachineInstr *MI,
                                      int &FrameIndex) const;
  bool hasStoreFromStackSlot(const MachineInstr *MI,
                             const MachineMemOperand *&MMO,
                             int &FrameIndex) const;

  MachineInstr *
  convertToThreeAddress(MachineFunction::iterator &MFI,
                        MachineBasicBlock::iterator &MBBI,
                        LiveVariables *LV) const override;


  virtual void copyPhysReg(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI, DebugLoc DL,
                           unsigned DestReg, unsigned SrcReg,
                           bool KillSrc) const = 0;

  bool expandPostRAPseudo(MachineBasicBlock::iterator MI) const override;

  void storeRegToStackSlot(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MI,
                           unsigned SrcReg, bool isKill, int FrameIndex,
                           const TargetRegisterClass *RC,
                           const TargetRegisterInfo *TRI) const override;
  void loadRegFromStackSlot(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MI,
                            unsigned DestReg, int FrameIndex,
                            const TargetRegisterClass *RC,
                            const TargetRegisterInfo *TRI) const override;

protected:
  MachineInstr *foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      int FrameIndex) const override;
  MachineInstr *foldMemoryOperandImpl(MachineFunction &MF,
                                      MachineInstr *MI,
                                      const SmallVectorImpl<unsigned> &Ops,
                                      MachineInstr *LoadMI) const override;
  /// \returns the smallest register index that will be accessed by an indirect
  /// read or write or -1 if indirect addressing is not used by this program.
  int getIndirectIndexBegin(const MachineFunction &MF) const;

  /// \returns the largest register index that will be accessed by an indirect
  /// read or write or -1 if indirect addressing is not used by this program.
  int getIndirectIndexEnd(const MachineFunction &MF) const;

public:
  bool canFoldMemoryOperand(const MachineInstr *MI,
                           const SmallVectorImpl<unsigned> &Ops) const override;
  bool unfoldMemoryOperand(MachineFunction &MF, MachineInstr *MI,
                        unsigned Reg, bool UnfoldLoad, bool UnfoldStore,
                        SmallVectorImpl<MachineInstr *> &NewMIs) const override;
  bool unfoldMemoryOperand(SelectionDAG &DAG, SDNode *N,
                           SmallVectorImpl<SDNode *> &NewNodes) const override;
  unsigned getOpcodeAfterMemoryUnfold(unsigned Opc,
                               bool UnfoldLoad, bool UnfoldStore,
                               unsigned *LoadRegIndex = nullptr) const override;
  bool shouldScheduleLoadsNear(SDNode *Load1, SDNode *Load2,
                               int64_t Offset1, int64_t Offset2,
                               unsigned NumLoads) const override;

  bool
  ReverseBranchCondition(SmallVectorImpl<MachineOperand> &Cond) const override;
  void insertNoop(MachineBasicBlock &MBB,
                  MachineBasicBlock::iterator MI) const override;
  bool isPredicated(const MachineInstr *MI) const override;
  bool SubsumesPredicate(const SmallVectorImpl<MachineOperand> &Pred1,
                   const SmallVectorImpl<MachineOperand> &Pred2) const override;
  bool DefinesPredicate(MachineInstr *MI,
                        std::vector<MachineOperand> &Pred) const override;
  bool isPredicable(MachineInstr *MI) const override;
  bool isSafeToMoveRegClassDefs(const TargetRegisterClass *RC) const override;

  // Helper functions that check the opcode for status information
  bool isLoadInst(llvm::MachineInstr *MI) const;
  bool isExtLoadInst(llvm::MachineInstr *MI) const;
  bool isSWSExtLoadInst(llvm::MachineInstr *MI) const;
  bool isSExtLoadInst(llvm::MachineInstr *MI) const;
  bool isZExtLoadInst(llvm::MachineInstr *MI) const;
  bool isAExtLoadInst(llvm::MachineInstr *MI) const;
  bool isStoreInst(llvm::MachineInstr *MI) const;
  bool isTruncStoreInst(llvm::MachineInstr *MI) const;
  bool isRegisterStore(const MachineInstr &MI) const;
  bool isRegisterLoad(const MachineInstr &MI) const;

//===---------------------------------------------------------------------===//
// Pure virtual funtions to be implemented by sub-classes.
//===---------------------------------------------------------------------===//

  virtual unsigned getIEQOpcode() const = 0;
  virtual bool isMov(unsigned opcode) const = 0;

  /// \brief Calculate the "Indirect Address" for the given \p RegIndex and
  ///        \p Channel
  ///
  /// We model indirect addressing using a virtual address space that can be
  /// accesed with loads and stores.  The "Indirect Address" is the memory
  /// address in this virtual address space that maps to the given \p RegIndex
  /// and \p Channel.
  virtual unsigned calculateIndirectAddress(unsigned RegIndex,
                                            unsigned Channel) const = 0;

  /// \returns The register class to be used for loading and storing values
  /// from an "Indirect Address" .
  virtual const TargetRegisterClass *getIndirectAddrRegClass() const = 0;

  /// \brief Build instruction(s) for an indirect register write.
  ///
  /// \returns The instruction that performs the indirect register write
  virtual MachineInstrBuilder buildIndirectWrite(MachineBasicBlock *MBB,
                                    MachineBasicBlock::iterator I,
                                    unsigned ValueReg, unsigned Address,
                                    unsigned OffsetReg) const = 0;

  /// \brief Build instruction(s) for an indirect register read.
  ///
  /// \returns The instruction that performs the indirect register read
  virtual MachineInstrBuilder buildIndirectRead(MachineBasicBlock *MBB,
                                    MachineBasicBlock::iterator I,
                                    unsigned ValueReg, unsigned Address,
                                    unsigned OffsetReg) const = 0;


  /// \brief Convert the AMDIL MachineInstr to a supported ISA
  /// MachineInstr
  void convertToISA(MachineInstr & MI, MachineFunction &MF, DebugLoc DL) const;

  /// \brief Build a MOV instruction.
  virtual MachineInstr *buildMovInstr(MachineBasicBlock *MBB,
                                      MachineBasicBlock::iterator I,
                                      unsigned DstReg, unsigned SrcReg) const = 0;

  /// \brief Given a MIMG \p Opcode that writes all 4 channels, return the
  /// equivalent opcode that writes \p Channels Channels.
  int getMaskedMIMGOp(uint16_t Opcode, unsigned Channels) const;

};

namespace AMDGPU {
  int16_t getNamedOperandIdx(uint16_t Opcode, uint16_t NamedIndex);
}  // End namespace AMDGPU

} // End llvm namespace

#define AMDGPU_FLAG_REGISTER_LOAD  (UINT64_C(1) << 63)
#define AMDGPU_FLAG_REGISTER_STORE (UINT64_C(1) << 62)

#endif // AMDGPUINSTRINFO_H
