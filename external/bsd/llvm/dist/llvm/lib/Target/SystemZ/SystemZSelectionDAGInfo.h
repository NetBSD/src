//===-- SystemZSelectionDAGInfo.h - SystemZ SelectionDAG Info ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the SystemZ subclass for TargetSelectionDAGInfo.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTEMZSELECTIONDAGINFO_H
#define SYSTEMZSELECTIONDAGINFO_H

#include "llvm/Target/TargetSelectionDAGInfo.h"

namespace llvm {

class SystemZTargetMachine;

class SystemZSelectionDAGInfo : public TargetSelectionDAGInfo {
public:
  explicit SystemZSelectionDAGInfo(const SystemZTargetMachine &TM);
  ~SystemZSelectionDAGInfo();

  virtual
  SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                                  SDValue Dst, SDValue Src,
                                  SDValue Size, unsigned Align,
                                  bool IsVolatile, bool AlwaysInline,
                                  MachinePointerInfo DstPtrInfo,
                                  MachinePointerInfo SrcPtrInfo) const
    LLVM_OVERRIDE;

  virtual SDValue
  EmitTargetCodeForMemset(SelectionDAG &DAG, SDLoc DL,
                          SDValue Chain, SDValue Dst, SDValue Byte,
                          SDValue Size, unsigned Align, bool IsVolatile,
                          MachinePointerInfo DstPtrInfo) const LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForMemcmp(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                          SDValue Src1, SDValue Src2, SDValue Size,
                          MachinePointerInfo Op1PtrInfo,
                          MachinePointerInfo Op2PtrInfo) const LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForMemchr(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                          SDValue Src, SDValue Char, SDValue Length,
                          MachinePointerInfo SrcPtrInfo) const LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForStrcpy(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                          SDValue Dest, SDValue Src,
                          MachinePointerInfo DestPtrInfo,
                          MachinePointerInfo SrcPtrInfo,
                          bool isStpcpy) const LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForStrcmp(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                          SDValue Src1, SDValue Src2,
                          MachinePointerInfo Op1PtrInfo,
                          MachinePointerInfo Op2PtrInfo) const LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForStrlen(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                          SDValue Src, MachinePointerInfo SrcPtrInfo) const
    LLVM_OVERRIDE;

  virtual std::pair<SDValue, SDValue>
  EmitTargetCodeForStrnlen(SelectionDAG &DAG, SDLoc DL, SDValue Chain,
                           SDValue Src, SDValue MaxLength,
                           MachinePointerInfo SrcPtrInfo) const LLVM_OVERRIDE;
};

}

#endif
