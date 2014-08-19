//===-- SIInstrInfo.h - SI Instruction Info Interface -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Interface definition for SIInstrInfo.
//
//===----------------------------------------------------------------------===//


#ifndef SIINSTRINFO_H
#define SIINSTRINFO_H

#include "AMDGPUInstrInfo.h"
#include "SIRegisterInfo.h"

namespace llvm {

class SIInstrInfo : public AMDGPUInstrInfo {
private:
  const SIRegisterInfo RI;

  unsigned buildExtractSubReg(MachineBasicBlock::iterator MI,
                              MachineRegisterInfo &MRI,
                              MachineOperand &SuperReg,
                              const TargetRegisterClass *SuperRC,
                              unsigned SubIdx,
                              const TargetRegisterClass *SubRC) const;
  MachineOperand buildExtractSubRegOrImm(MachineBasicBlock::iterator MI,
                                         MachineRegisterInfo &MRI,
                                         MachineOperand &SuperReg,
                                         const TargetRegisterClass *SuperRC,
                                         unsigned SubIdx,
                                         const TargetRegisterClass *SubRC) const;

  unsigned split64BitImm(SmallVectorImpl<MachineInstr *> &Worklist,
                         MachineBasicBlock::iterator MI,
                         MachineRegisterInfo &MRI,
                         const TargetRegisterClass *RC,
                         const MachineOperand &Op) const;

  void splitScalar64BitUnaryOp(SmallVectorImpl<MachineInstr *> &Worklist,
                               MachineInstr *Inst, unsigned Opcode) const;

  void splitScalar64BitBinaryOp(SmallVectorImpl<MachineInstr *> &Worklist,
                                MachineInstr *Inst, unsigned Opcode) const;

  void splitScalar64BitBCNT(SmallVectorImpl<MachineInstr *> &Worklist,
                            MachineInstr *Inst) const;

  void addDescImplicitUseDef(const MCInstrDesc &Desc, MachineInstr *MI) const;

public:
  explicit SIInstrInfo(const AMDGPUSubtarget &st);

  const SIRegisterInfo &getRegisterInfo() const override {
    return RI;
  }

  bool areLoadsFromSameBasePtr(SDNode *Load1, SDNode *Load2,
                               int64_t &Offset1,
                               int64_t &Offset2) const override;

  bool getLdStBaseRegImmOfs(MachineInstr *LdSt,
                            unsigned &BaseReg, unsigned &Offset,
                            const TargetRegisterInfo *TRI) const final;

  void copyPhysReg(MachineBasicBlock &MBB,
                   MachineBasicBlock::iterator MI, DebugLoc DL,
                   unsigned DestReg, unsigned SrcReg,
                   bool KillSrc) const override;

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

  virtual bool expandPostRAPseudo(MachineBasicBlock::iterator MI) const;

  unsigned commuteOpcode(unsigned Opcode) const;

  MachineInstr *commuteInstruction(MachineInstr *MI,
                                   bool NewMI=false) const override;

  bool isTriviallyReMaterializable(const MachineInstr *MI,
                                   AliasAnalysis *AA = nullptr) const;

  MachineInstr *buildMovInstr(MachineBasicBlock *MBB,
                              MachineBasicBlock::iterator I,
                              unsigned DstReg, unsigned SrcReg) const override;
  bool isMov(unsigned Opcode) const override;

  bool isSafeToMoveRegClassDefs(const TargetRegisterClass *RC) const override;
  bool isDS(uint16_t Opcode) const;
  bool isMIMG(uint16_t Opcode) const;
  bool isSMRD(uint16_t Opcode) const;
  bool isMUBUF(uint16_t Opcode) const;
  bool isMTBUF(uint16_t Opcode) const;
  bool isVOP1(uint16_t Opcode) const;
  bool isVOP2(uint16_t Opcode) const;
  bool isVOP3(uint16_t Opcode) const;
  bool isVOPC(uint16_t Opcode) const;
  bool isInlineConstant(const APInt &Imm) const;
  bool isInlineConstant(const MachineOperand &MO) const;
  bool isLiteralConstant(const MachineOperand &MO) const;

  bool isImmOperandLegal(const MachineInstr *MI, unsigned OpNo,
                         const MachineOperand &MO) const;

  /// \brief Return true if this 64-bit VALU instruction has a 32-bit encoding.
  /// This function will return false if you pass it a 32-bit instruction.
  bool hasVALU32BitEncoding(unsigned Opcode) const;

  /// \brief Return true if this instruction has any modifiers.
  ///  e.g. src[012]_mod, omod, clamp.
  bool hasModifiers(unsigned Opcode) const;
  bool verifyInstruction(const MachineInstr *MI,
                         StringRef &ErrInfo) const override;

  bool isSALUInstr(const MachineInstr &MI) const;
  static unsigned getVALUOp(const MachineInstr &MI);

  bool isSALUOpSupportedOnVALU(const MachineInstr &MI) const;

  /// \brief Return the correct register class for \p OpNo.  For target-specific
  /// instructions, this will return the register class that has been defined
  /// in tablegen.  For generic instructions, like REG_SEQUENCE it will return
  /// the register class of its machine operand.
  /// to infer the correct register class base on the other operands.
  const TargetRegisterClass *getOpRegClass(const MachineInstr &MI,
                                           unsigned OpNo) const;\

  /// \returns true if it is legal for the operand at index \p OpNo
  /// to read a VGPR.
  bool canReadVGPR(const MachineInstr &MI, unsigned OpNo) const;

  /// \brief Legalize the \p OpIndex operand of this instruction by inserting
  /// a MOV.  For example:
  /// ADD_I32_e32 VGPR0, 15
  /// to
  /// MOV VGPR1, 15
  /// ADD_I32_e32 VGPR0, VGPR1
  ///
  /// If the operand being legalized is a register, then a COPY will be used
  /// instead of MOV.
  void legalizeOpWithMove(MachineInstr *MI, unsigned OpIdx) const;

  /// \brief Check if \p MO is a legal operand if it was the \p OpIdx Operand
  /// for \p MI.
  bool isOperandLegal(const MachineInstr *MI, unsigned OpIdx,
                      const MachineOperand *MO = nullptr) const;

  /// \brief Legalize all operands in this instruction.  This function may
  /// create new instruction and insert them before \p MI.
  void legalizeOperands(MachineInstr *MI) const;

  void moveSMRDToVALU(MachineInstr *MI, MachineRegisterInfo &MRI) const;

  /// \brief Replace this instruction's opcode with the equivalent VALU
  /// opcode.  This function will also move the users of \p MI to the
  /// VALU if necessary.
  void moveToVALU(MachineInstr &MI) const;

  unsigned calculateIndirectAddress(unsigned RegIndex,
                                    unsigned Channel) const override;

  const TargetRegisterClass *getIndirectAddrRegClass() const override;

  MachineInstrBuilder buildIndirectWrite(MachineBasicBlock *MBB,
                                         MachineBasicBlock::iterator I,
                                         unsigned ValueReg,
                                         unsigned Address,
                                         unsigned OffsetReg) const override;

  MachineInstrBuilder buildIndirectRead(MachineBasicBlock *MBB,
                                        MachineBasicBlock::iterator I,
                                        unsigned ValueReg,
                                        unsigned Address,
                                        unsigned OffsetReg) const override;
  void reserveIndirectRegisters(BitVector &Reserved,
                                const MachineFunction &MF) const;

  void LoadM0(MachineInstr *MoveRel, MachineBasicBlock::iterator I,
              unsigned SavReg, unsigned IndexReg) const;

  void insertNOPs(MachineBasicBlock::iterator MI, int Count) const;

  /// \brief Returns the operand named \p Op.  If \p MI does not have an
  /// operand named \c Op, this function returns nullptr.
  MachineOperand *getNamedOperand(MachineInstr &MI, unsigned OperandName) const;
};

namespace AMDGPU {

  int getVOPe64(uint16_t Opcode);
  int getVOPe32(uint16_t Opcode);
  int getCommuteRev(uint16_t Opcode);
  int getCommuteOrig(uint16_t Opcode);
  int getMCOpcode(uint16_t Opcode, unsigned Gen);

  const uint64_t RSRC_DATA_FORMAT = 0xf00000000000LL;
  const uint64_t RSRC_TID_ENABLE = 1LL << 55;

} // End namespace AMDGPU

} // End namespace llvm

namespace SIInstrFlags {
  enum Flags {
    // First 4 bits are the instruction encoding
    VM_CNT = 1 << 0,
    EXP_CNT = 1 << 1,
    LGKM_CNT = 1 << 2
  };
}

namespace SISrcMods {
  enum {
   NEG = 1 << 0,
   ABS = 1 << 1
  };
}

#endif //SIINSTRINFO_H
