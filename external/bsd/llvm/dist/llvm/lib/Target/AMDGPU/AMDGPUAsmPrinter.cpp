//===-- AMDGPUAsmPrinter.cpp - AMDGPU Assebly printer  --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
///
/// The AMDGPUAsmPrinter is used to print both assembly string and also binary
/// code.  When passed an MCAsmStreamer it prints assembly and when passed
/// an MCObjectStreamer it outputs binary code.
//
//===----------------------------------------------------------------------===//
//

#include "AMDGPUAsmPrinter.h"
#include "MCTargetDesc/AMDGPUTargetStreamer.h"
#include "InstPrinter/AMDGPUInstPrinter.h"
#include "Utils/AMDGPUBaseInfo.h"
#include "AMDGPU.h"
#include "AMDKernelCodeT.h"
#include "AMDGPUSubtarget.h"
#include "R600Defines.h"
#include "R600MachineFunctionInfo.h"
#include "R600RegisterInfo.h"
#include "SIDefines.h"
#include "SIMachineFunctionInfo.h"
#include "SIInstrInfo.h"
#include "SIRegisterInfo.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/ELF.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

using namespace llvm;

// TODO: This should get the default rounding mode from the kernel. We just set
// the default here, but this could change if the OpenCL rounding mode pragmas
// are used.
//
// The denormal mode here should match what is reported by the OpenCL runtime
// for the CL_FP_DENORM bit from CL_DEVICE_{HALF|SINGLE|DOUBLE}_FP_CONFIG, but
// can also be override to flush with the -cl-denorms-are-zero compiler flag.
//
// AMD OpenCL only sets flush none and reports CL_FP_DENORM for double
// precision, and leaves single precision to flush all and does not report
// CL_FP_DENORM for CL_DEVICE_SINGLE_FP_CONFIG. Mesa's OpenCL currently reports
// CL_FP_DENORM for both.
//
// FIXME: It seems some instructions do not support single precision denormals
// regardless of the mode (exp_*_f32, rcp_*_f32, rsq_*_f32, rsq_*f32, sqrt_f32,
// and sin_f32, cos_f32 on most parts).

// We want to use these instructions, and using fp32 denormals also causes
// instructions to run at the double precision rate for the device so it's
// probably best to just report no single precision denormals.
static uint32_t getFPMode(const MachineFunction &F) {
  const SISubtarget& ST = F.getSubtarget<SISubtarget>();
  // TODO: Is there any real use for the flush in only / flush out only modes?

  uint32_t FP32Denormals =
    ST.hasFP32Denormals() ? FP_DENORM_FLUSH_NONE : FP_DENORM_FLUSH_IN_FLUSH_OUT;

  uint32_t FP64Denormals =
    ST.hasFP64Denormals() ? FP_DENORM_FLUSH_NONE : FP_DENORM_FLUSH_IN_FLUSH_OUT;

  return FP_ROUND_MODE_SP(FP_ROUND_ROUND_TO_NEAREST) |
         FP_ROUND_MODE_DP(FP_ROUND_ROUND_TO_NEAREST) |
         FP_DENORM_MODE_SP(FP32Denormals) |
         FP_DENORM_MODE_DP(FP64Denormals);
}

static AsmPrinter *
createAMDGPUAsmPrinterPass(TargetMachine &tm,
                           std::unique_ptr<MCStreamer> &&Streamer) {
  return new AMDGPUAsmPrinter(tm, std::move(Streamer));
}

extern "C" void LLVMInitializeAMDGPUAsmPrinter() {
  TargetRegistry::RegisterAsmPrinter(getTheAMDGPUTarget(),
                                     createAMDGPUAsmPrinterPass);
  TargetRegistry::RegisterAsmPrinter(getTheGCNTarget(),
                                     createAMDGPUAsmPrinterPass);
}

AMDGPUAsmPrinter::AMDGPUAsmPrinter(TargetMachine &TM,
                                   std::unique_ptr<MCStreamer> Streamer)
  : AsmPrinter(TM, std::move(Streamer)) {}

StringRef AMDGPUAsmPrinter::getPassName() const {
  return "AMDGPU Assembly Printer";
}

void AMDGPUAsmPrinter::EmitStartOfAsmFile(Module &M) {
  if (TM.getTargetTriple().getOS() != Triple::AMDHSA)
    return;

  // Need to construct an MCSubtargetInfo here in case we have no functions
  // in the module.
  std::unique_ptr<MCSubtargetInfo> STI(TM.getTarget().createMCSubtargetInfo(
        TM.getTargetTriple().str(), TM.getTargetCPU(),
        TM.getTargetFeatureString()));

  AMDGPUTargetStreamer *TS =
      static_cast<AMDGPUTargetStreamer *>(OutStreamer->getTargetStreamer());

  TS->EmitDirectiveHSACodeObjectVersion(2, 1);

  AMDGPU::IsaVersion ISA = AMDGPU::getIsaVersion(STI->getFeatureBits());
  TS->EmitDirectiveHSACodeObjectISA(ISA.Major, ISA.Minor, ISA.Stepping,
                                    "AMD", "AMDGPU");

  // Emit runtime metadata.
  TS->EmitRuntimeMetadata(M);
}

bool AMDGPUAsmPrinter::isBlockOnlyReachableByFallthrough(
  const MachineBasicBlock *MBB) const {
  if (!AsmPrinter::isBlockOnlyReachableByFallthrough(MBB))
    return false;

  if (MBB->empty())
    return true;

  // If this is a block implementing a long branch, an expression relative to
  // the start of the block is needed.  to the start of the block.
  // XXX - Is there a smarter way to check this?
  return (MBB->back().getOpcode() != AMDGPU::S_SETPC_B64);
}


void AMDGPUAsmPrinter::EmitFunctionBodyStart() {
  const AMDGPUSubtarget &STM = MF->getSubtarget<AMDGPUSubtarget>();
  SIProgramInfo KernelInfo;
  if (STM.isAmdCodeObjectV2(*MF)) {
    getSIProgramInfo(KernelInfo, *MF);
    EmitAmdKernelCodeT(*MF, KernelInfo);
  }
}

void AMDGPUAsmPrinter::EmitFunctionEntryLabel() {
  const SIMachineFunctionInfo *MFI = MF->getInfo<SIMachineFunctionInfo>();
  const AMDGPUSubtarget &STM = MF->getSubtarget<AMDGPUSubtarget>();
  if (MFI->isKernel() && STM.isAmdCodeObjectV2(*MF)) {
    AMDGPUTargetStreamer *TS =
        static_cast<AMDGPUTargetStreamer *>(OutStreamer->getTargetStreamer());
    SmallString<128> SymbolName;
    getNameWithPrefix(SymbolName, MF->getFunction()),
    TS->EmitAMDGPUSymbolType(SymbolName, ELF::STT_AMDGPU_HSA_KERNEL);
  }

  AsmPrinter::EmitFunctionEntryLabel();
}

void AMDGPUAsmPrinter::EmitGlobalVariable(const GlobalVariable *GV) {

  // Group segment variables aren't emitted in HSA.
  if (AMDGPU::isGroupSegment(GV))
    return;

  AsmPrinter::EmitGlobalVariable(GV);
}

bool AMDGPUAsmPrinter::runOnMachineFunction(MachineFunction &MF) {

  // The starting address of all shader programs must be 256 bytes aligned.
  MF.setAlignment(8);

  SetupMachineFunction(MF);

  const AMDGPUSubtarget &STM = MF.getSubtarget<AMDGPUSubtarget>();
  MCContext &Context = getObjFileLowering().getContext();
  if (!STM.isAmdHsaOS()) {
    MCSectionELF *ConfigSection =
        Context.getELFSection(".AMDGPU.config", ELF::SHT_PROGBITS, 0);
    OutStreamer->SwitchSection(ConfigSection);
  }

  SIProgramInfo KernelInfo;
  if (STM.getGeneration() >= AMDGPUSubtarget::SOUTHERN_ISLANDS) {
    getSIProgramInfo(KernelInfo, MF);
    if (!STM.isAmdHsaOS()) {
      EmitProgramInfoSI(MF, KernelInfo);
    }
  } else {
    EmitProgramInfoR600(MF);
  }

  DisasmLines.clear();
  HexLines.clear();
  DisasmLineMaxLen = 0;

  EmitFunctionBody();

  if (isVerbose()) {
    MCSectionELF *CommentSection =
        Context.getELFSection(".AMDGPU.csdata", ELF::SHT_PROGBITS, 0);
    OutStreamer->SwitchSection(CommentSection);

    if (STM.getGeneration() >= AMDGPUSubtarget::SOUTHERN_ISLANDS) {
      OutStreamer->emitRawComment(" Kernel info:", false);
      OutStreamer->emitRawComment(" codeLenInByte = " + Twine(KernelInfo.CodeLen),
                                  false);
      OutStreamer->emitRawComment(" NumSgprs: " + Twine(KernelInfo.NumSGPR),
                                  false);
      OutStreamer->emitRawComment(" NumVgprs: " + Twine(KernelInfo.NumVGPR),
                                  false);
      OutStreamer->emitRawComment(" FloatMode: " + Twine(KernelInfo.FloatMode),
                                  false);
      OutStreamer->emitRawComment(" IeeeMode: " + Twine(KernelInfo.IEEEMode),
                                  false);
      OutStreamer->emitRawComment(" ScratchSize: " + Twine(KernelInfo.ScratchSize),
                                  false);
      OutStreamer->emitRawComment(" LDSByteSize: " + Twine(KernelInfo.LDSSize) +
                                  " bytes/workgroup (compile time only)", false);

      OutStreamer->emitRawComment(" SGPRBlocks: " +
                                  Twine(KernelInfo.SGPRBlocks), false);
      OutStreamer->emitRawComment(" VGPRBlocks: " +
                                  Twine(KernelInfo.VGPRBlocks), false);

      OutStreamer->emitRawComment(" NumSGPRsForWavesPerEU: " +
                                  Twine(KernelInfo.NumSGPRsForWavesPerEU), false);
      OutStreamer->emitRawComment(" NumVGPRsForWavesPerEU: " +
                                  Twine(KernelInfo.NumVGPRsForWavesPerEU), false);

      OutStreamer->emitRawComment(" ReservedVGPRFirst: " + Twine(KernelInfo.ReservedVGPRFirst),
                                  false);
      OutStreamer->emitRawComment(" ReservedVGPRCount: " + Twine(KernelInfo.ReservedVGPRCount),
                                  false);

      if (MF.getSubtarget<SISubtarget>().debuggerEmitPrologue()) {
        OutStreamer->emitRawComment(" DebuggerWavefrontPrivateSegmentOffsetSGPR: s" +
                                    Twine(KernelInfo.DebuggerWavefrontPrivateSegmentOffsetSGPR), false);
        OutStreamer->emitRawComment(" DebuggerPrivateSegmentBufferSGPR: s" +
                                    Twine(KernelInfo.DebuggerPrivateSegmentBufferSGPR), false);
      }

      OutStreamer->emitRawComment(" COMPUTE_PGM_RSRC2:USER_SGPR: " +
                                  Twine(G_00B84C_USER_SGPR(KernelInfo.ComputePGMRSrc2)),
                                  false);
      OutStreamer->emitRawComment(" COMPUTE_PGM_RSRC2:TGID_X_EN: " +
                                  Twine(G_00B84C_TGID_X_EN(KernelInfo.ComputePGMRSrc2)),
                                  false);
      OutStreamer->emitRawComment(" COMPUTE_PGM_RSRC2:TGID_Y_EN: " +
                                  Twine(G_00B84C_TGID_Y_EN(KernelInfo.ComputePGMRSrc2)),
                                  false);
      OutStreamer->emitRawComment(" COMPUTE_PGM_RSRC2:TGID_Z_EN: " +
                                  Twine(G_00B84C_TGID_Z_EN(KernelInfo.ComputePGMRSrc2)),
                                  false);
      OutStreamer->emitRawComment(" COMPUTE_PGM_RSRC2:TIDIG_COMP_CNT: " +
                                  Twine(G_00B84C_TIDIG_COMP_CNT(KernelInfo.ComputePGMRSrc2)),
                                  false);

    } else {
      R600MachineFunctionInfo *MFI = MF.getInfo<R600MachineFunctionInfo>();
      OutStreamer->emitRawComment(
        Twine("SQ_PGM_RESOURCES:STACK_SIZE = " + Twine(MFI->CFStackSize)));
    }
  }

  if (STM.dumpCode()) {

    OutStreamer->SwitchSection(
        Context.getELFSection(".AMDGPU.disasm", ELF::SHT_NOTE, 0));

    for (size_t i = 0; i < DisasmLines.size(); ++i) {
      std::string Comment(DisasmLineMaxLen - DisasmLines[i].size(), ' ');
      Comment += " ; " + HexLines[i] + "\n";

      OutStreamer->EmitBytes(StringRef(DisasmLines[i]));
      OutStreamer->EmitBytes(StringRef(Comment));
    }
  }

  return false;
}

void AMDGPUAsmPrinter::EmitProgramInfoR600(const MachineFunction &MF) {
  unsigned MaxGPR = 0;
  bool killPixel = false;
  const R600Subtarget &STM = MF.getSubtarget<R600Subtarget>();
  const R600RegisterInfo *RI = STM.getRegisterInfo();
  const R600MachineFunctionInfo *MFI = MF.getInfo<R600MachineFunctionInfo>();

  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      if (MI.getOpcode() == AMDGPU::KILLGT)
        killPixel = true;
      unsigned numOperands = MI.getNumOperands();
      for (unsigned op_idx = 0; op_idx < numOperands; op_idx++) {
        const MachineOperand &MO = MI.getOperand(op_idx);
        if (!MO.isReg())
          continue;
        unsigned HWReg = RI->getEncodingValue(MO.getReg()) & 0xff;

        // Register with value > 127 aren't GPR
        if (HWReg > 127)
          continue;
        MaxGPR = std::max(MaxGPR, HWReg);
      }
    }
  }

  unsigned RsrcReg;
  if (STM.getGeneration() >= R600Subtarget::EVERGREEN) {
    // Evergreen / Northern Islands
    switch (MF.getFunction()->getCallingConv()) {
    default: LLVM_FALLTHROUGH;
    case CallingConv::AMDGPU_CS: RsrcReg = R_0288D4_SQ_PGM_RESOURCES_LS; break;
    case CallingConv::AMDGPU_GS: RsrcReg = R_028878_SQ_PGM_RESOURCES_GS; break;
    case CallingConv::AMDGPU_PS: RsrcReg = R_028844_SQ_PGM_RESOURCES_PS; break;
    case CallingConv::AMDGPU_VS: RsrcReg = R_028860_SQ_PGM_RESOURCES_VS; break;
    }
  } else {
    // R600 / R700
    switch (MF.getFunction()->getCallingConv()) {
    default: LLVM_FALLTHROUGH;
    case CallingConv::AMDGPU_GS: LLVM_FALLTHROUGH;
    case CallingConv::AMDGPU_CS: LLVM_FALLTHROUGH;
    case CallingConv::AMDGPU_VS: RsrcReg = R_028868_SQ_PGM_RESOURCES_VS; break;
    case CallingConv::AMDGPU_PS: RsrcReg = R_028850_SQ_PGM_RESOURCES_PS; break;
    }
  }

  OutStreamer->EmitIntValue(RsrcReg, 4);
  OutStreamer->EmitIntValue(S_NUM_GPRS(MaxGPR + 1) |
                           S_STACK_SIZE(MFI->CFStackSize), 4);
  OutStreamer->EmitIntValue(R_02880C_DB_SHADER_CONTROL, 4);
  OutStreamer->EmitIntValue(S_02880C_KILL_ENABLE(killPixel), 4);

  if (AMDGPU::isCompute(MF.getFunction()->getCallingConv())) {
    OutStreamer->EmitIntValue(R_0288E8_SQ_LDS_ALLOC, 4);
    OutStreamer->EmitIntValue(alignTo(MFI->getLDSSize(), 4) >> 2, 4);
  }
}

void AMDGPUAsmPrinter::getSIProgramInfo(SIProgramInfo &ProgInfo,
                                        const MachineFunction &MF) const {
  const SISubtarget &STM = MF.getSubtarget<SISubtarget>();
  const SIMachineFunctionInfo *MFI = MF.getInfo<SIMachineFunctionInfo>();
  uint64_t CodeSize = 0;
  unsigned MaxSGPR = 0;
  unsigned MaxVGPR = 0;
  bool VCCUsed = false;
  bool FlatUsed = false;
  const SIRegisterInfo *RI = STM.getRegisterInfo();
  const SIInstrInfo *TII = STM.getInstrInfo();

  for (const MachineBasicBlock &MBB : MF) {
    for (const MachineInstr &MI : MBB) {
      // TODO: CodeSize should account for multiple functions.

      // TODO: Should we count size of debug info?
      if (MI.isDebugValue())
        continue;

      if (isVerbose())
        CodeSize += TII->getInstSizeInBytes(MI);

      unsigned numOperands = MI.getNumOperands();
      for (unsigned op_idx = 0; op_idx < numOperands; op_idx++) {
        const MachineOperand &MO = MI.getOperand(op_idx);
        unsigned width = 0;
        bool isSGPR = false;

        if (!MO.isReg())
          continue;

        unsigned reg = MO.getReg();
        switch (reg) {
        case AMDGPU::EXEC:
        case AMDGPU::EXEC_LO:
        case AMDGPU::EXEC_HI:
        case AMDGPU::SCC:
        case AMDGPU::M0:
          continue;

        case AMDGPU::VCC:
        case AMDGPU::VCC_LO:
        case AMDGPU::VCC_HI:
          VCCUsed = true;
          continue;

        case AMDGPU::FLAT_SCR:
        case AMDGPU::FLAT_SCR_LO:
        case AMDGPU::FLAT_SCR_HI:
          // Even if FLAT_SCRATCH is implicitly used, it has no effect if flat
          // instructions aren't used to access the scratch buffer.
          if (MFI->hasFlatScratchInit())
            FlatUsed = true;
          continue;

        case AMDGPU::TBA:
        case AMDGPU::TBA_LO:
        case AMDGPU::TBA_HI:
        case AMDGPU::TMA:
        case AMDGPU::TMA_LO:
        case AMDGPU::TMA_HI:
          llvm_unreachable("trap handler registers should not be used");

        default:
          break;
        }

        if (AMDGPU::SReg_32RegClass.contains(reg)) {
          assert(!AMDGPU::TTMP_32RegClass.contains(reg) &&
                 "trap handler registers should not be used");
          isSGPR = true;
          width = 1;
        } else if (AMDGPU::VGPR_32RegClass.contains(reg)) {
          isSGPR = false;
          width = 1;
        } else if (AMDGPU::SReg_64RegClass.contains(reg)) {
          assert(!AMDGPU::TTMP_64RegClass.contains(reg) &&
                 "trap handler registers should not be used");
          isSGPR = true;
          width = 2;
        } else if (AMDGPU::VReg_64RegClass.contains(reg)) {
          isSGPR = false;
          width = 2;
        } else if (AMDGPU::VReg_96RegClass.contains(reg)) {
          isSGPR = false;
          width = 3;
        } else if (AMDGPU::SReg_128RegClass.contains(reg)) {
          isSGPR = true;
          width = 4;
        } else if (AMDGPU::VReg_128RegClass.contains(reg)) {
          isSGPR = false;
          width = 4;
        } else if (AMDGPU::SReg_256RegClass.contains(reg)) {
          isSGPR = true;
          width = 8;
        } else if (AMDGPU::VReg_256RegClass.contains(reg)) {
          isSGPR = false;
          width = 8;
        } else if (AMDGPU::SReg_512RegClass.contains(reg)) {
          isSGPR = true;
          width = 16;
        } else if (AMDGPU::VReg_512RegClass.contains(reg)) {
          isSGPR = false;
          width = 16;
        } else {
          llvm_unreachable("Unknown register class");
        }
        unsigned hwReg = RI->getEncodingValue(reg) & 0xff;
        unsigned maxUsed = hwReg + width - 1;
        if (isSGPR) {
          MaxSGPR = maxUsed > MaxSGPR ? maxUsed : MaxSGPR;
        } else {
          MaxVGPR = maxUsed > MaxVGPR ? maxUsed : MaxVGPR;
        }
      }
    }
  }

  unsigned ExtraSGPRs = 0;

  if (VCCUsed)
    ExtraSGPRs = 2;

  if (STM.getGeneration() < SISubtarget::VOLCANIC_ISLANDS) {
    if (FlatUsed)
      ExtraSGPRs = 4;
  } else {
    if (STM.isXNACKEnabled())
      ExtraSGPRs = 4;

    if (FlatUsed)
      ExtraSGPRs = 6;
  }

  // Record first reserved register and reserved register count fields, and
  // update max register counts if "amdgpu-debugger-reserve-regs" attribute was
  // requested.
  ProgInfo.ReservedVGPRFirst = STM.debuggerReserveRegs() ? MaxVGPR + 1 : 0;
  ProgInfo.ReservedVGPRCount = RI->getNumDebuggerReservedVGPRs(STM);

  // Update DebuggerWavefrontPrivateSegmentOffsetSGPR and
  // DebuggerPrivateSegmentBufferSGPR fields if "amdgpu-debugger-emit-prologue"
  // attribute was requested.
  if (STM.debuggerEmitPrologue()) {
    ProgInfo.DebuggerWavefrontPrivateSegmentOffsetSGPR =
      RI->getHWRegIndex(MFI->getScratchWaveOffsetReg());
    ProgInfo.DebuggerPrivateSegmentBufferSGPR =
      RI->getHWRegIndex(MFI->getScratchRSrcReg());
  }

  // Check the addressable register limit before we add ExtraSGPRs.
  if (STM.getGeneration() >= AMDGPUSubtarget::VOLCANIC_ISLANDS &&
      !STM.hasSGPRInitBug()) {
    unsigned MaxAddressableNumSGPRs = STM.getMaxNumSGPRs();
    if (MaxSGPR + 1 > MaxAddressableNumSGPRs) {
      // This can happen due to a compiler bug or when using inline asm.
      LLVMContext &Ctx = MF.getFunction()->getContext();
      DiagnosticInfoResourceLimit Diag(*MF.getFunction(),
                                       "addressable scalar registers",
                                       MaxSGPR + 1, DS_Error,
                                       DK_ResourceLimit, MaxAddressableNumSGPRs);
      Ctx.diagnose(Diag);
      MaxSGPR = MaxAddressableNumSGPRs - 1;
    }
  }

  // Account for extra SGPRs and VGPRs reserved for debugger use.
  MaxSGPR += ExtraSGPRs;
  MaxVGPR += RI->getNumDebuggerReservedVGPRs(STM);

  // We found the maximum register index. They start at 0, so add one to get the
  // number of registers.
  ProgInfo.NumVGPR = MaxVGPR + 1;
  ProgInfo.NumSGPR = MaxSGPR + 1;

  // Adjust number of registers used to meet default/requested minimum/maximum
  // number of waves per execution unit request.
  ProgInfo.NumSGPRsForWavesPerEU = std::max(
    ProgInfo.NumSGPR, RI->getMinNumSGPRs(STM, MFI->getMaxWavesPerEU()));
  ProgInfo.NumVGPRsForWavesPerEU = std::max(
    ProgInfo.NumVGPR, RI->getMinNumVGPRs(MFI->getMaxWavesPerEU()));

  if (STM.getGeneration() <= AMDGPUSubtarget::SEA_ISLANDS ||
      STM.hasSGPRInitBug()) {
    unsigned MaxNumSGPRs = STM.getMaxNumSGPRs();
    if (ProgInfo.NumSGPR > MaxNumSGPRs) {
      // This can happen due to a compiler bug or when using inline asm to use the
      // registers which are usually reserved for vcc etc.

      LLVMContext &Ctx = MF.getFunction()->getContext();
      DiagnosticInfoResourceLimit Diag(*MF.getFunction(),
                                       "scalar registers",
                                       ProgInfo.NumSGPR, DS_Error,
                                       DK_ResourceLimit, MaxNumSGPRs);
      Ctx.diagnose(Diag);
      ProgInfo.NumSGPR = MaxNumSGPRs;
      ProgInfo.NumSGPRsForWavesPerEU = MaxNumSGPRs;
    }
  }

  if (STM.hasSGPRInitBug()) {
    ProgInfo.NumSGPR = SISubtarget::FIXED_SGPR_COUNT_FOR_INIT_BUG;
    ProgInfo.NumSGPRsForWavesPerEU = SISubtarget::FIXED_SGPR_COUNT_FOR_INIT_BUG;
  }

  if (MFI->NumUserSGPRs > STM.getMaxNumUserSGPRs()) {
    LLVMContext &Ctx = MF.getFunction()->getContext();
    DiagnosticInfoResourceLimit Diag(*MF.getFunction(), "user SGPRs",
                                     MFI->NumUserSGPRs, DS_Error);
    Ctx.diagnose(Diag);
  }

  if (MFI->getLDSSize() > static_cast<unsigned>(STM.getLocalMemorySize())) {
    LLVMContext &Ctx = MF.getFunction()->getContext();
    DiagnosticInfoResourceLimit Diag(*MF.getFunction(), "local memory",
                                     MFI->getLDSSize(), DS_Error);
    Ctx.diagnose(Diag);
  }

  // SGPRBlocks is actual number of SGPR blocks minus 1.
  ProgInfo.SGPRBlocks = alignTo(ProgInfo.NumSGPRsForWavesPerEU,
                                RI->getSGPRAllocGranule());
  ProgInfo.SGPRBlocks = ProgInfo.SGPRBlocks / RI->getSGPRAllocGranule() - 1;

  // VGPRBlocks is actual number of VGPR blocks minus 1.
  ProgInfo.VGPRBlocks = alignTo(ProgInfo.NumVGPRsForWavesPerEU,
                                RI->getVGPRAllocGranule());
  ProgInfo.VGPRBlocks = ProgInfo.VGPRBlocks / RI->getVGPRAllocGranule() - 1;

  // Set the value to initialize FP_ROUND and FP_DENORM parts of the mode
  // register.
  ProgInfo.FloatMode = getFPMode(MF);

  ProgInfo.IEEEMode = STM.enableIEEEBit(MF);

  // Make clamp modifier on NaN input returns 0.
  ProgInfo.DX10Clamp = 1;

  const MachineFrameInfo &FrameInfo = MF.getFrameInfo();
  ProgInfo.ScratchSize = FrameInfo.getStackSize();

  ProgInfo.FlatUsed = FlatUsed;
  ProgInfo.VCCUsed = VCCUsed;
  ProgInfo.CodeLen = CodeSize;

  unsigned LDSAlignShift;
  if (STM.getGeneration() < SISubtarget::SEA_ISLANDS) {
    // LDS is allocated in 64 dword blocks.
    LDSAlignShift = 8;
  } else {
    // LDS is allocated in 128 dword blocks.
    LDSAlignShift = 9;
  }

  unsigned LDSSpillSize =
    MFI->LDSWaveSpillSize * MFI->getMaxFlatWorkGroupSize();

  ProgInfo.LDSSize = MFI->getLDSSize() + LDSSpillSize;
  ProgInfo.LDSBlocks =
      alignTo(ProgInfo.LDSSize, 1ULL << LDSAlignShift) >> LDSAlignShift;

  // Scratch is allocated in 256 dword blocks.
  unsigned ScratchAlignShift = 10;
  // We need to program the hardware with the amount of scratch memory that
  // is used by the entire wave.  ProgInfo.ScratchSize is the amount of
  // scratch memory used per thread.
  ProgInfo.ScratchBlocks =
      alignTo(ProgInfo.ScratchSize * STM.getWavefrontSize(),
              1ULL << ScratchAlignShift) >>
      ScratchAlignShift;

  ProgInfo.ComputePGMRSrc1 =
      S_00B848_VGPRS(ProgInfo.VGPRBlocks) |
      S_00B848_SGPRS(ProgInfo.SGPRBlocks) |
      S_00B848_PRIORITY(ProgInfo.Priority) |
      S_00B848_FLOAT_MODE(ProgInfo.FloatMode) |
      S_00B848_PRIV(ProgInfo.Priv) |
      S_00B848_DX10_CLAMP(ProgInfo.DX10Clamp) |
      S_00B848_DEBUG_MODE(ProgInfo.DebugMode) |
      S_00B848_IEEE_MODE(ProgInfo.IEEEMode);

  // 0 = X, 1 = XY, 2 = XYZ
  unsigned TIDIGCompCnt = 0;
  if (MFI->hasWorkItemIDZ())
    TIDIGCompCnt = 2;
  else if (MFI->hasWorkItemIDY())
    TIDIGCompCnt = 1;

  ProgInfo.ComputePGMRSrc2 =
      S_00B84C_SCRATCH_EN(ProgInfo.ScratchBlocks > 0) |
      S_00B84C_USER_SGPR(MFI->getNumUserSGPRs()) |
      S_00B84C_TGID_X_EN(MFI->hasWorkGroupIDX()) |
      S_00B84C_TGID_Y_EN(MFI->hasWorkGroupIDY()) |
      S_00B84C_TGID_Z_EN(MFI->hasWorkGroupIDZ()) |
      S_00B84C_TG_SIZE_EN(MFI->hasWorkGroupInfo()) |
      S_00B84C_TIDIG_COMP_CNT(TIDIGCompCnt) |
      S_00B84C_EXCP_EN_MSB(0) |
      S_00B84C_LDS_SIZE(ProgInfo.LDSBlocks) |
      S_00B84C_EXCP_EN(0);
}

static unsigned getRsrcReg(CallingConv::ID CallConv) {
  switch (CallConv) {
  default: LLVM_FALLTHROUGH;
  case CallingConv::AMDGPU_CS: return R_00B848_COMPUTE_PGM_RSRC1;
  case CallingConv::AMDGPU_GS: return R_00B228_SPI_SHADER_PGM_RSRC1_GS;
  case CallingConv::AMDGPU_PS: return R_00B028_SPI_SHADER_PGM_RSRC1_PS;
  case CallingConv::AMDGPU_VS: return R_00B128_SPI_SHADER_PGM_RSRC1_VS;
  }
}

void AMDGPUAsmPrinter::EmitProgramInfoSI(const MachineFunction &MF,
                                         const SIProgramInfo &KernelInfo) {
  const SISubtarget &STM = MF.getSubtarget<SISubtarget>();
  const SIMachineFunctionInfo *MFI = MF.getInfo<SIMachineFunctionInfo>();
  unsigned RsrcReg = getRsrcReg(MF.getFunction()->getCallingConv());

  if (AMDGPU::isCompute(MF.getFunction()->getCallingConv())) {
    OutStreamer->EmitIntValue(R_00B848_COMPUTE_PGM_RSRC1, 4);

    OutStreamer->EmitIntValue(KernelInfo.ComputePGMRSrc1, 4);

    OutStreamer->EmitIntValue(R_00B84C_COMPUTE_PGM_RSRC2, 4);
    OutStreamer->EmitIntValue(KernelInfo.ComputePGMRSrc2, 4);

    OutStreamer->EmitIntValue(R_00B860_COMPUTE_TMPRING_SIZE, 4);
    OutStreamer->EmitIntValue(S_00B860_WAVESIZE(KernelInfo.ScratchBlocks), 4);

    // TODO: Should probably note flat usage somewhere. SC emits a "FlatPtr32 =
    // 0" comment but I don't see a corresponding field in the register spec.
  } else {
    OutStreamer->EmitIntValue(RsrcReg, 4);
    OutStreamer->EmitIntValue(S_00B028_VGPRS(KernelInfo.VGPRBlocks) |
                              S_00B028_SGPRS(KernelInfo.SGPRBlocks), 4);
    if (STM.isVGPRSpillingEnabled(*MF.getFunction())) {
      OutStreamer->EmitIntValue(R_0286E8_SPI_TMPRING_SIZE, 4);
      OutStreamer->EmitIntValue(S_0286E8_WAVESIZE(KernelInfo.ScratchBlocks), 4);
    }
  }

  if (MF.getFunction()->getCallingConv() == CallingConv::AMDGPU_PS) {
    OutStreamer->EmitIntValue(R_00B02C_SPI_SHADER_PGM_RSRC2_PS, 4);
    OutStreamer->EmitIntValue(S_00B02C_EXTRA_LDS_SIZE(KernelInfo.LDSBlocks), 4);
    OutStreamer->EmitIntValue(R_0286CC_SPI_PS_INPUT_ENA, 4);
    OutStreamer->EmitIntValue(MFI->PSInputEna, 4);
    OutStreamer->EmitIntValue(R_0286D0_SPI_PS_INPUT_ADDR, 4);
    OutStreamer->EmitIntValue(MFI->getPSInputAddr(), 4);
  }

  OutStreamer->EmitIntValue(R_SPILLED_SGPRS, 4);
  OutStreamer->EmitIntValue(MFI->getNumSpilledSGPRs(), 4);
  OutStreamer->EmitIntValue(R_SPILLED_VGPRS, 4);
  OutStreamer->EmitIntValue(MFI->getNumSpilledVGPRs(), 4);
}

// This is supposed to be log2(Size)
static amd_element_byte_size_t getElementByteSizeValue(unsigned Size) {
  switch (Size) {
  case 4:
    return AMD_ELEMENT_4_BYTES;
  case 8:
    return AMD_ELEMENT_8_BYTES;
  case 16:
    return AMD_ELEMENT_16_BYTES;
  default:
    llvm_unreachable("invalid private_element_size");
  }
}

void AMDGPUAsmPrinter::EmitAmdKernelCodeT(const MachineFunction &MF,
                                         const SIProgramInfo &KernelInfo) const {
  const SIMachineFunctionInfo *MFI = MF.getInfo<SIMachineFunctionInfo>();
  const SISubtarget &STM = MF.getSubtarget<SISubtarget>();
  amd_kernel_code_t header;

  AMDGPU::initDefaultAMDKernelCodeT(header, STM.getFeatureBits());

  header.compute_pgm_resource_registers =
      KernelInfo.ComputePGMRSrc1 |
      (KernelInfo.ComputePGMRSrc2 << 32);
  header.code_properties = AMD_CODE_PROPERTY_IS_PTR64;


  AMD_HSA_BITS_SET(header.code_properties,
                   AMD_CODE_PROPERTY_PRIVATE_ELEMENT_SIZE,
                   getElementByteSizeValue(STM.getMaxPrivateElementSize()));

  if (MFI->hasPrivateSegmentBuffer()) {
    header.code_properties |=
      AMD_CODE_PROPERTY_ENABLE_SGPR_PRIVATE_SEGMENT_BUFFER;
  }

  if (MFI->hasDispatchPtr())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_DISPATCH_PTR;

  if (MFI->hasQueuePtr())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_QUEUE_PTR;

  if (MFI->hasKernargSegmentPtr())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_KERNARG_SEGMENT_PTR;

  if (MFI->hasDispatchID())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_DISPATCH_ID;

  if (MFI->hasFlatScratchInit())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_FLAT_SCRATCH_INIT;

  // TODO: Private segment size

  if (MFI->hasGridWorkgroupCountX()) {
    header.code_properties |=
      AMD_CODE_PROPERTY_ENABLE_SGPR_GRID_WORKGROUP_COUNT_X;
  }

  if (MFI->hasGridWorkgroupCountY()) {
    header.code_properties |=
      AMD_CODE_PROPERTY_ENABLE_SGPR_GRID_WORKGROUP_COUNT_Y;
  }

  if (MFI->hasGridWorkgroupCountZ()) {
    header.code_properties |=
      AMD_CODE_PROPERTY_ENABLE_SGPR_GRID_WORKGROUP_COUNT_Z;
  }

  if (MFI->hasDispatchPtr())
    header.code_properties |= AMD_CODE_PROPERTY_ENABLE_SGPR_DISPATCH_PTR;

  if (STM.debuggerSupported())
    header.code_properties |= AMD_CODE_PROPERTY_IS_DEBUG_SUPPORTED;

  if (STM.isXNACKEnabled())
    header.code_properties |= AMD_CODE_PROPERTY_IS_XNACK_SUPPORTED;

  // FIXME: Should use getKernArgSize
  header.kernarg_segment_byte_size =
    STM.getKernArgSegmentSize(MF, MFI->getABIArgOffset());
  header.wavefront_sgpr_count = KernelInfo.NumSGPR;
  header.workitem_vgpr_count = KernelInfo.NumVGPR;
  header.workitem_private_segment_byte_size = KernelInfo.ScratchSize;
  header.workgroup_group_segment_byte_size = KernelInfo.LDSSize;
  header.reserved_vgpr_first = KernelInfo.ReservedVGPRFirst;
  header.reserved_vgpr_count = KernelInfo.ReservedVGPRCount;

  // These alignment values are specified in powers of two, so alignment =
  // 2^n.  The minimum alignment is 2^4 = 16.
  header.kernarg_segment_alignment = std::max((size_t)4,
      countTrailingZeros(MFI->getMaxKernArgAlign()));

  if (STM.debuggerEmitPrologue()) {
    header.debug_wavefront_private_segment_offset_sgpr =
      KernelInfo.DebuggerWavefrontPrivateSegmentOffsetSGPR;
    header.debug_private_segment_buffer_sgpr =
      KernelInfo.DebuggerPrivateSegmentBufferSGPR;
  }

  AMDGPUTargetStreamer *TS =
      static_cast<AMDGPUTargetStreamer *>(OutStreamer->getTargetStreamer());

  OutStreamer->SwitchSection(getObjFileLowering().getTextSection());
  TS->EmitAMDKernelCodeT(header);
}

bool AMDGPUAsmPrinter::PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                                       unsigned AsmVariant,
                                       const char *ExtraCode, raw_ostream &O) {
  if (ExtraCode && ExtraCode[0]) {
    if (ExtraCode[1] != 0)
      return true; // Unknown modifier.

    switch (ExtraCode[0]) {
    default:
      // See if this is a generic print operand
      return AsmPrinter::PrintAsmOperand(MI, OpNo, AsmVariant, ExtraCode, O);
    case 'r':
      break;
    }
  }

  AMDGPUInstPrinter::printRegOperand(MI->getOperand(OpNo).getReg(), O,
                   *TM.getSubtargetImpl(*MF->getFunction())->getRegisterInfo());
  return false;
}
