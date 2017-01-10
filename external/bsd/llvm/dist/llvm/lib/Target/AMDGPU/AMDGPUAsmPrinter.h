//===-- AMDGPUAsmPrinter.h - Print AMDGPU assembly code ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief AMDGPU Assembly printer class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_AMDGPU_AMDGPUASMPRINTER_H
#define LLVM_LIB_TARGET_AMDGPU_AMDGPUASMPRINTER_H

#include "AMDGPUMCInstLower.h"

#include "llvm/CodeGen/AsmPrinter.h"
#include <vector>

namespace llvm {
class MCOperand;

class AMDGPUAsmPrinter final : public AsmPrinter {
private:
  struct SIProgramInfo {
    SIProgramInfo() :
      VGPRBlocks(0),
      SGPRBlocks(0),
      Priority(0),
      FloatMode(0),
      Priv(0),
      DX10Clamp(0),
      DebugMode(0),
      IEEEMode(0),
      ScratchSize(0),
      ComputePGMRSrc1(0),
      LDSBlocks(0),
      ScratchBlocks(0),
      ComputePGMRSrc2(0),
      NumVGPR(0),
      NumSGPR(0),
      FlatUsed(false),
      NumSGPRsForWavesPerEU(0),
      NumVGPRsForWavesPerEU(0),
      ReservedVGPRFirst(0),
      ReservedVGPRCount(0),
      DebuggerWavefrontPrivateSegmentOffsetSGPR((uint16_t)-1),
      DebuggerPrivateSegmentBufferSGPR((uint16_t)-1),
      VCCUsed(false),
      CodeLen(0) {}

    // Fields set in PGM_RSRC1 pm4 packet.
    uint32_t VGPRBlocks;
    uint32_t SGPRBlocks;
    uint32_t Priority;
    uint32_t FloatMode;
    uint32_t Priv;
    uint32_t DX10Clamp;
    uint32_t DebugMode;
    uint32_t IEEEMode;
    uint32_t ScratchSize;

    uint64_t ComputePGMRSrc1;

    // Fields set in PGM_RSRC2 pm4 packet.
    uint32_t LDSBlocks;
    uint32_t ScratchBlocks;

    uint64_t ComputePGMRSrc2;

    uint32_t NumVGPR;
    uint32_t NumSGPR;
    uint32_t LDSSize;
    bool FlatUsed;

    // Number of SGPRs that meets number of waves per execution unit request.
    uint32_t NumSGPRsForWavesPerEU;

    // Number of VGPRs that meets number of waves per execution unit request.
    uint32_t NumVGPRsForWavesPerEU;

    // If ReservedVGPRCount is 0 then must be 0. Otherwise, this is the first
    // fixed VGPR number reserved.
    uint16_t ReservedVGPRFirst;

    // The number of consecutive VGPRs reserved.
    uint16_t ReservedVGPRCount;

    // Fixed SGPR number used to hold wave scratch offset for entire kernel
    // execution, or uint16_t(-1) if the register is not used or not known.
    uint16_t DebuggerWavefrontPrivateSegmentOffsetSGPR;

    // Fixed SGPR number of the first 4 SGPRs used to hold scratch V# for entire
    // kernel execution, or uint16_t(-1) if the register is not used or not
    // known.
    uint16_t DebuggerPrivateSegmentBufferSGPR;

    // Bonus information for debugging.
    bool VCCUsed;
    uint64_t CodeLen;
  };

  void getSIProgramInfo(SIProgramInfo &Out, const MachineFunction &MF) const;
  void findNumUsedRegistersSI(const MachineFunction &MF,
                              unsigned &NumSGPR,
                              unsigned &NumVGPR) const;

  /// \brief Emit register usage information so that the GPU driver
  /// can correctly setup the GPU state.
  void EmitProgramInfoR600(const MachineFunction &MF);
  void EmitProgramInfoSI(const MachineFunction &MF, const SIProgramInfo &KernelInfo);
  void EmitAmdKernelCodeT(const MachineFunction &MF,
                          const SIProgramInfo &KernelInfo) const;

public:
  explicit AMDGPUAsmPrinter(TargetMachine &TM,
                            std::unique_ptr<MCStreamer> Streamer);

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override;

  /// \brief Wrapper for MCInstLowering.lowerOperand() for the tblgen'erated
  /// pseudo lowering.
  bool lowerOperand(const MachineOperand &MO, MCOperand &MCOp) const;

  /// \brief tblgen'erated driver function for lowering simple MI->MC pseudo
  /// instructions.
  bool emitPseudoExpansionLowering(MCStreamer &OutStreamer,
                                   const MachineInstr *MI);

  /// Implemented in AMDGPUMCInstLower.cpp
  void EmitInstruction(const MachineInstr *MI) override;

  void EmitFunctionBodyStart() override;

  void EmitFunctionEntryLabel() override;

  void EmitGlobalVariable(const GlobalVariable *GV) override;

  void EmitStartOfAsmFile(Module &M) override;

  bool isBlockOnlyReachableByFallthrough(
    const MachineBasicBlock *MBB) const override;

  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       unsigned AsmVariant, const char *ExtraCode,
                       raw_ostream &O) override;

protected:
  std::vector<std::string> DisasmLines, HexLines;
  size_t DisasmLineMaxLen;
};

} // End anonymous llvm

#endif
