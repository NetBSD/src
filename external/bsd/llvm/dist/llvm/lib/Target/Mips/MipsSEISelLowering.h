//===-- MipsSEISelLowering.h - MipsSE DAG Lowering Interface ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Subclass of MipsTargetLowering specialized for mips32/64.
//
//===----------------------------------------------------------------------===//

#ifndef MipsSEISELLOWERING_H
#define MipsSEISELLOWERING_H

#include "MipsISelLowering.h"
#include "MipsRegisterInfo.h"

namespace llvm {
  class MipsSETargetLowering : public MipsTargetLowering  {
  public:
    explicit MipsSETargetLowering(MipsTargetMachine &TM);

    /// \brief Enable MSA support for the given integer type and Register
    /// class.
    void addMSAIntType(MVT::SimpleValueType Ty, const TargetRegisterClass *RC);
    /// \brief Enable MSA support for the given floating-point type and
    /// Register class.
    void addMSAFloatType(MVT::SimpleValueType Ty,
                         const TargetRegisterClass *RC);

    bool allowsUnalignedMemoryAccesses(EVT VT, unsigned AS = 0,
                                       bool *Fast = nullptr) const override;

    SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const override;

    SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const override;

    MachineBasicBlock *
    EmitInstrWithCustomInserter(MachineInstr *MI,
                                MachineBasicBlock *MBB) const override;

    bool isShuffleMaskLegal(const SmallVectorImpl<int> &Mask,
                            EVT VT) const override {
      return false;
    }

    const TargetRegisterClass *getRepRegClassFor(MVT VT) const override {
      if (VT == MVT::Untyped)
        return Subtarget->hasDSP() ? &Mips::ACC64DSPRegClass :
                                     &Mips::ACC64RegClass;

      return TargetLowering::getRepRegClassFor(VT);
    }

  private:
    bool isEligibleForTailCallOptimization(const MipsCC &MipsCCInfo,
                                     unsigned NextStackOffset,
                                     const MipsFunctionInfo& FI) const override;

    void
    getOpndList(SmallVectorImpl<SDValue> &Ops,
                std::deque< std::pair<unsigned, SDValue> > &RegsToPass,
                bool IsPICCall, bool GlobalOrExternal, bool InternalLinkage,
                CallLoweringInfo &CLI, SDValue Callee,
                SDValue Chain) const override;

    SDValue lowerLOAD(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerSTORE(SDValue Op, SelectionDAG &DAG) const;

    SDValue lowerMulDiv(SDValue Op, unsigned NewOpc, bool HasLo, bool HasHi,
                        SelectionDAG &DAG) const;

    SDValue lowerINTRINSIC_WO_CHAIN(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerINTRINSIC_W_CHAIN(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerINTRINSIC_VOID(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerEXTRACT_VECTOR_ELT(SDValue Op, SelectionDAG &DAG) const;
    SDValue lowerBUILD_VECTOR(SDValue Op, SelectionDAG &DAG) const;
    /// \brief Lower VECTOR_SHUFFLE into one of a number of instructions
    /// depending on the indices in the shuffle.
    SDValue lowerVECTOR_SHUFFLE(SDValue Op, SelectionDAG &DAG) const;

    MachineBasicBlock *emitBPOSGE32(MachineInstr *MI,
                                    MachineBasicBlock *BB) const;
    MachineBasicBlock *emitMSACBranchPseudo(MachineInstr *MI,
                                            MachineBasicBlock *BB,
                                            unsigned BranchOp) const;
    /// \brief Emit the COPY_FW pseudo instruction
    MachineBasicBlock *emitCOPY_FW(MachineInstr *MI,
                                   MachineBasicBlock *BB) const;
    /// \brief Emit the COPY_FD pseudo instruction
    MachineBasicBlock *emitCOPY_FD(MachineInstr *MI,
                                   MachineBasicBlock *BB) const;
    /// \brief Emit the INSERT_FW pseudo instruction
    MachineBasicBlock *emitINSERT_FW(MachineInstr *MI,
                                     MachineBasicBlock *BB) const;
    /// \brief Emit the INSERT_FD pseudo instruction
    MachineBasicBlock *emitINSERT_FD(MachineInstr *MI,
                                     MachineBasicBlock *BB) const;
    /// \brief Emit the INSERT_([BHWD]|F[WD])_VIDX pseudo instruction
    MachineBasicBlock *emitINSERT_DF_VIDX(MachineInstr *MI,
                                          MachineBasicBlock *BB,
                                          unsigned EltSizeInBytes,
                                          bool IsFP) const;
    /// \brief Emit the FILL_FW pseudo instruction
    MachineBasicBlock *emitFILL_FW(MachineInstr *MI,
                                   MachineBasicBlock *BB) const;
    /// \brief Emit the FILL_FD pseudo instruction
    MachineBasicBlock *emitFILL_FD(MachineInstr *MI,
                                   MachineBasicBlock *BB) const;
    /// \brief Emit the FEXP2_W_1 pseudo instructions.
    MachineBasicBlock *emitFEXP2_W_1(MachineInstr *MI,
                                     MachineBasicBlock *BB) const;
    /// \brief Emit the FEXP2_D_1 pseudo instructions.
    MachineBasicBlock *emitFEXP2_D_1(MachineInstr *MI,
                                     MachineBasicBlock *BB) const;
  };
}

#endif // MipsSEISELLOWERING_H
