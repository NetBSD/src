//===-- AMDGPUSubtarget.cpp - AMDGPU Subtarget Information ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Implements the AMDGPU specific subclass of TargetSubtarget.
//
//===----------------------------------------------------------------------===//

#include "AMDGPUSubtarget.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/Target/TargetFrameLowering.h"
#include <algorithm>

using namespace llvm;

#define DEBUG_TYPE "amdgpu-subtarget"

#define GET_SUBTARGETINFO_ENUM
#define GET_SUBTARGETINFO_TARGET_DESC
#define GET_SUBTARGETINFO_CTOR
#include "AMDGPUGenSubtargetInfo.inc"

AMDGPUSubtarget::~AMDGPUSubtarget() = default;

AMDGPUSubtarget &
AMDGPUSubtarget::initializeSubtargetDependencies(const Triple &TT,
                                                 StringRef GPU, StringRef FS) {
  // Determine default and user-specified characteristics
  // On SI+, we want FP64 denormals to be on by default. FP32 denormals can be
  // enabled, but some instructions do not respect them and they run at the
  // double precision rate, so don't enable by default.
  //
  // We want to be able to turn these off, but making this a subtarget feature
  // for SI has the unhelpful behavior that it unsets everything else if you
  // disable it.

  SmallString<256> FullFS("+promote-alloca,+fp64-denormals,+load-store-opt,");
  if (isAmdHsaOS()) // Turn on FlatForGlobal for HSA.
    FullFS += "+flat-for-global,+unaligned-buffer-access,";
  FullFS += FS;

  ParseSubtargetFeatures(GPU, FullFS);

  // Unless +-flat-for-global is specified, turn on FlatForGlobal for all OS-es
  // on VI and newer hardware to avoid assertion failures due to missing ADDR64
  // variants of MUBUF instructions.
  if (!hasAddr64() && !FS.contains("flat-for-global")) {
    FlatForGlobal = true;
  }

  // FIXME: I don't think think Evergreen has any useful support for
  // denormals, but should be checked. Should we issue a warning somewhere
  // if someone tries to enable these?
  if (getGeneration() <= AMDGPUSubtarget::NORTHERN_ISLANDS) {
    FP16Denormals = false;
    FP32Denormals = false;
    FP64Denormals = false;
  }

  // Set defaults if needed.
  if (MaxPrivateElementSize == 0)
    MaxPrivateElementSize = 4;

  return *this;
}

AMDGPUSubtarget::AMDGPUSubtarget(const Triple &TT, StringRef GPU, StringRef FS,
                                 const TargetMachine &TM)
  : AMDGPUGenSubtargetInfo(TT, GPU, FS),
    TargetTriple(TT),
    Gen(TT.getArch() == Triple::amdgcn ? SOUTHERN_ISLANDS : R600),
    IsaVersion(ISAVersion0_0_0),
    WavefrontSize(64),
    LocalMemorySize(0),
    LDSBankCount(0),
    MaxPrivateElementSize(0),

    FastFMAF32(false),
    HalfRate64Ops(false),

    FP16Denormals(false),
    FP32Denormals(false),
    FP64Denormals(false),
    FPExceptions(false),
    FlatForGlobal(false),
    UnalignedScratchAccess(false),
    UnalignedBufferAccess(false),

    EnableXNACK(false),
    DebuggerInsertNops(false),
    DebuggerReserveRegs(false),
    DebuggerEmitPrologue(false),

    EnableVGPRSpilling(false),
    EnablePromoteAlloca(false),
    EnableLoadStoreOpt(false),
    EnableUnsafeDSOffsetFolding(false),
    EnableSIScheduler(false),
    DumpCode(false),

    FP64(false),
    IsGCN(false),
    GCN1Encoding(false),
    GCN3Encoding(false),
    CIInsts(false),
    SGPRInitBug(false),
    HasSMemRealTime(false),
    Has16BitInsts(false),
    HasMovrel(false),
    HasVGPRIndexMode(false),
    HasScalarStores(false),
    HasInv2PiInlineImm(false),
    FlatAddressSpace(false),

    R600ALUInst(false),
    CaymanISA(false),
    CFALUBug(false),
    HasVertexCache(false),
    TexVTXClauseSize(0),
    ScalarizeGlobal(false),

    FeatureDisable(false),
    InstrItins(getInstrItineraryForCPU(GPU)) {
  initializeSubtargetDependencies(TT, GPU, FS);
}

// FIXME: These limits are for SI. Did they change with the larger maximum LDS
// size?
unsigned AMDGPUSubtarget::getMaxLocalMemSizeWithWaveCount(unsigned NWaves) const {
  switch (NWaves) {
  case 10:
    return 1638;
  case 9:
    return 1820;
  case 8:
    return 2048;
  case 7:
    return 2340;
  case 6:
    return 2730;
  case 5:
    return 3276;
  case 4:
    return 4096;
  case 3:
    return 5461;
  case 2:
    return 8192;
  default:
    return getLocalMemorySize();
  }
}

unsigned AMDGPUSubtarget::getOccupancyWithLocalMemSize(uint32_t Bytes) const {
  if (Bytes <= 1638)
    return 10;

  if (Bytes <= 1820)
    return 9;

  if (Bytes <= 2048)
    return 8;

  if (Bytes <= 2340)
    return 7;

  if (Bytes <= 2730)
    return 6;

  if (Bytes <= 3276)
    return 5;

  if (Bytes <= 4096)
    return 4;

  if (Bytes <= 5461)
    return 3;

  if (Bytes <= 8192)
    return 2;

  return 1;
}

std::pair<unsigned, unsigned> AMDGPUSubtarget::getFlatWorkGroupSizes(
  const Function &F) const {
  // Default minimum/maximum flat work group sizes.
  std::pair<unsigned, unsigned> Default =
    AMDGPU::isCompute(F.getCallingConv()) ?
      std::pair<unsigned, unsigned>(getWavefrontSize() * 2,
                                    getWavefrontSize() * 4) :
      std::pair<unsigned, unsigned>(1, getWavefrontSize());

  // TODO: Do not process "amdgpu-max-work-group-size" attribute once mesa
  // starts using "amdgpu-flat-work-group-size" attribute.
  Default.second = AMDGPU::getIntegerAttribute(
    F, "amdgpu-max-work-group-size", Default.second);
  Default.first = std::min(Default.first, Default.second);

  // Requested minimum/maximum flat work group sizes.
  std::pair<unsigned, unsigned> Requested = AMDGPU::getIntegerPairAttribute(
    F, "amdgpu-flat-work-group-size", Default);

  // Make sure requested minimum is less than requested maximum.
  if (Requested.first > Requested.second)
    return Default;

  // Make sure requested values do not violate subtarget's specifications.
  if (Requested.first < getMinFlatWorkGroupSize())
    return Default;
  if (Requested.second > getMaxFlatWorkGroupSize())
    return Default;

  return Requested;
}

std::pair<unsigned, unsigned> AMDGPUSubtarget::getWavesPerEU(
  const Function &F) const {
  // Default minimum/maximum number of waves per execution unit.
  std::pair<unsigned, unsigned> Default(1, 0);

  // Default/requested minimum/maximum flat work group sizes.
  std::pair<unsigned, unsigned> FlatWorkGroupSizes = getFlatWorkGroupSizes(F);

  // If minimum/maximum flat work group sizes were explicitly requested using
  // "amdgpu-flat-work-group-size" attribute, then set default minimum/maximum
  // number of waves per execution unit to values implied by requested
  // minimum/maximum flat work group sizes.
  unsigned MinImpliedByFlatWorkGroupSize =
    getMaxWavesPerEU(FlatWorkGroupSizes.second);
  bool RequestedFlatWorkGroupSize = false;

  // TODO: Do not process "amdgpu-max-work-group-size" attribute once mesa
  // starts using "amdgpu-flat-work-group-size" attribute.
  if (F.hasFnAttribute("amdgpu-max-work-group-size") ||
      F.hasFnAttribute("amdgpu-flat-work-group-size")) {
    Default.first = MinImpliedByFlatWorkGroupSize;
    RequestedFlatWorkGroupSize = true;
  }

  // Requested minimum/maximum number of waves per execution unit.
  std::pair<unsigned, unsigned> Requested = AMDGPU::getIntegerPairAttribute(
    F, "amdgpu-waves-per-eu", Default, true);

  // Make sure requested minimum is less than requested maximum.
  if (Requested.second && Requested.first > Requested.second)
    return Default;

  // Make sure requested values do not violate subtarget's specifications.
  if (Requested.first < getMinWavesPerEU() ||
      Requested.first > getMaxWavesPerEU())
    return Default;
  if (Requested.second > getMaxWavesPerEU())
    return Default;

  // Make sure requested values are compatible with values implied by requested
  // minimum/maximum flat work group sizes.
  if (RequestedFlatWorkGroupSize &&
      Requested.first > MinImpliedByFlatWorkGroupSize)
    return Default;

  return Requested;
}

R600Subtarget::R600Subtarget(const Triple &TT, StringRef GPU, StringRef FS,
                             const TargetMachine &TM) :
  AMDGPUSubtarget(TT, GPU, FS, TM),
  InstrInfo(*this),
  FrameLowering(TargetFrameLowering::StackGrowsUp, getStackAlignment(), 0),
  TLInfo(TM, *this) {}

SISubtarget::SISubtarget(const Triple &TT, StringRef GPU, StringRef FS,
                         const TargetMachine &TM) :
  AMDGPUSubtarget(TT, GPU, FS, TM),
  InstrInfo(*this),
  FrameLowering(TargetFrameLowering::StackGrowsUp, getStackAlignment(), 0),
  TLInfo(TM, *this) {}

void SISubtarget::overrideSchedPolicy(MachineSchedPolicy &Policy,
                                      unsigned NumRegionInstrs) const {
  // Track register pressure so the scheduler can try to decrease
  // pressure once register usage is above the threshold defined by
  // SIRegisterInfo::getRegPressureSetLimit()
  Policy.ShouldTrackPressure = true;

  // Enabling both top down and bottom up scheduling seems to give us less
  // register spills than just using one of these approaches on its own.
  Policy.OnlyTopDown = false;
  Policy.OnlyBottomUp = false;

  // Enabling ShouldTrackLaneMasks crashes the SI Machine Scheduler.
  if (!enableSIScheduler())
    Policy.ShouldTrackLaneMasks = true;
}

bool SISubtarget::isVGPRSpillingEnabled(const Function& F) const {
  return EnableVGPRSpilling || !AMDGPU::isShader(F.getCallingConv());
}

unsigned SISubtarget::getKernArgSegmentSize(const MachineFunction &MF,
					    unsigned ExplicitArgBytes) const {
  unsigned ImplicitBytes = getImplicitArgNumBytes(MF);
  if (ImplicitBytes == 0)
    return ExplicitArgBytes;

  unsigned Alignment = getAlignmentForImplicitArgPtr();
  return alignTo(ExplicitArgBytes, Alignment) + ImplicitBytes;
}

unsigned SISubtarget::getOccupancyWithNumSGPRs(unsigned SGPRs) const {
  if (getGeneration() >= SISubtarget::VOLCANIC_ISLANDS) {
    if (SGPRs <= 80)
      return 10;
    if (SGPRs <= 88)
      return 9;
    if (SGPRs <= 100)
      return 8;
    return 7;
  }
  if (SGPRs <= 48)
    return 10;
  if (SGPRs <= 56)
    return 9;
  if (SGPRs <= 64)
    return 8;
  if (SGPRs <= 72)
    return 7;
  if (SGPRs <= 80)
    return 6;
  return 5;
}

unsigned SISubtarget::getOccupancyWithNumVGPRs(unsigned VGPRs) const {
  if (VGPRs <= 24)
    return 10;
  if (VGPRs <= 28)
    return 9;
  if (VGPRs <= 32)
    return 8;
  if (VGPRs <= 36)
    return 7;
  if (VGPRs <= 40)
    return 6;
  if (VGPRs <= 48)
    return 5;
  if (VGPRs <= 64)
    return 4;
  if (VGPRs <= 84)
    return 3;
  if (VGPRs <= 128)
    return 2;
  return 1;
}

unsigned SISubtarget::getMaxNumSGPRs() const {
  if (hasSGPRInitBug())
    return SISubtarget::FIXED_SGPR_COUNT_FOR_INIT_BUG;

  if (getGeneration() >= VOLCANIC_ISLANDS)
    return 102;

  return 104;
}
