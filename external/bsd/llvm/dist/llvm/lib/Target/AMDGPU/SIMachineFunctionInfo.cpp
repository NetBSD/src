//===-- SIMachineFunctionInfo.cpp - SI Machine Function Info -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
/// \file
//===----------------------------------------------------------------------===//


#include "SIMachineFunctionInfo.h"
#include "AMDGPUSubtarget.h"
#include "SIInstrInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LLVMContext.h"

#define MAX_LANES 64

using namespace llvm;


// Pin the vtable to this file.
void SIMachineFunctionInfo::anchor() {}

SIMachineFunctionInfo::SIMachineFunctionInfo(const MachineFunction &MF)
  : AMDGPUMachineFunction(MF),
    TIDReg(AMDGPU::NoRegister),
    ScratchRSrcReg(AMDGPU::NoRegister),
    ScratchWaveOffsetReg(AMDGPU::NoRegister),
    PrivateSegmentBufferUserSGPR(AMDGPU::NoRegister),
    DispatchPtrUserSGPR(AMDGPU::NoRegister),
    QueuePtrUserSGPR(AMDGPU::NoRegister),
    KernargSegmentPtrUserSGPR(AMDGPU::NoRegister),
    DispatchIDUserSGPR(AMDGPU::NoRegister),
    FlatScratchInitUserSGPR(AMDGPU::NoRegister),
    PrivateSegmentSizeUserSGPR(AMDGPU::NoRegister),
    GridWorkGroupCountXUserSGPR(AMDGPU::NoRegister),
    GridWorkGroupCountYUserSGPR(AMDGPU::NoRegister),
    GridWorkGroupCountZUserSGPR(AMDGPU::NoRegister),
    WorkGroupIDXSystemSGPR(AMDGPU::NoRegister),
    WorkGroupIDYSystemSGPR(AMDGPU::NoRegister),
    WorkGroupIDZSystemSGPR(AMDGPU::NoRegister),
    WorkGroupInfoSystemSGPR(AMDGPU::NoRegister),
    PrivateSegmentWaveByteOffsetSystemSGPR(AMDGPU::NoRegister),
    PSInputAddr(0),
    ReturnsVoid(true),
    LDSWaveSpillSize(0),
    PSInputEna(0),
    NumUserSGPRs(0),
    NumSystemSGPRs(0),
    HasSpilledSGPRs(false),
    HasSpilledVGPRs(false),
    HasNonSpillStackObjects(false),
    HasFlatInstructions(false),
    PrivateSegmentBuffer(false),
    DispatchPtr(false),
    QueuePtr(false),
    DispatchID(false),
    KernargSegmentPtr(false),
    FlatScratchInit(false),
    GridWorkgroupCountX(false),
    GridWorkgroupCountY(false),
    GridWorkgroupCountZ(false),
    WorkGroupIDX(true),
    WorkGroupIDY(false),
    WorkGroupIDZ(false),
    WorkGroupInfo(false),
    PrivateSegmentWaveByteOffset(false),
    WorkItemIDX(true),
    WorkItemIDY(false),
    WorkItemIDZ(false) {
  const AMDGPUSubtarget &ST = MF.getSubtarget<AMDGPUSubtarget>();
  const Function *F = MF.getFunction();

  PSInputAddr = AMDGPU::getInitialPSInputAddr(*F);

  const MachineFrameInfo *FrameInfo = MF.getFrameInfo();

  if (getShaderType() == ShaderType::COMPUTE)
    KernargSegmentPtr = true;

  if (F->hasFnAttribute("amdgpu-work-group-id-y"))
    WorkGroupIDY = true;

  if (F->hasFnAttribute("amdgpu-work-group-id-z"))
    WorkGroupIDZ = true;

  if (F->hasFnAttribute("amdgpu-work-item-id-y"))
    WorkItemIDY = true;

  if (F->hasFnAttribute("amdgpu-work-item-id-z"))
    WorkItemIDZ = true;

  // X, XY, and XYZ are the only supported combinations, so make sure Y is
  // enabled if Z is.
  if (WorkItemIDZ)
    WorkItemIDY = true;

  bool MaySpill = ST.isVGPRSpillingEnabled(this);
  bool HasStackObjects = FrameInfo->hasStackObjects();

  if (HasStackObjects || MaySpill)
    PrivateSegmentWaveByteOffset = true;

  if (ST.isAmdHsaOS()) {
    if (HasStackObjects || MaySpill)
      PrivateSegmentBuffer = true;

    if (F->hasFnAttribute("amdgpu-dispatch-ptr"))
      DispatchPtr = true;
  }

  // We don't need to worry about accessing spills with flat instructions.
  // TODO: On VI where we must use flat for global, we should be able to omit
  // this if it is never used for generic access.
  if (HasStackObjects && ST.getGeneration() >= AMDGPUSubtarget::SEA_ISLANDS &&
      ST.isAmdHsaOS())
    FlatScratchInit = true;
}

unsigned SIMachineFunctionInfo::addPrivateSegmentBuffer(
  const SIRegisterInfo &TRI) {
  PrivateSegmentBufferUserSGPR = TRI.getMatchingSuperReg(
    getNextUserSGPR(), AMDGPU::sub0, &AMDGPU::SReg_128RegClass);
  NumUserSGPRs += 4;
  return PrivateSegmentBufferUserSGPR;
}

unsigned SIMachineFunctionInfo::addDispatchPtr(const SIRegisterInfo &TRI) {
  DispatchPtrUserSGPR = TRI.getMatchingSuperReg(
    getNextUserSGPR(), AMDGPU::sub0, &AMDGPU::SReg_64RegClass);
  NumUserSGPRs += 2;
  return DispatchPtrUserSGPR;
}

unsigned SIMachineFunctionInfo::addQueuePtr(const SIRegisterInfo &TRI) {
  QueuePtrUserSGPR = TRI.getMatchingSuperReg(
    getNextUserSGPR(), AMDGPU::sub0, &AMDGPU::SReg_64RegClass);
  NumUserSGPRs += 2;
  return QueuePtrUserSGPR;
}

unsigned SIMachineFunctionInfo::addKernargSegmentPtr(const SIRegisterInfo &TRI) {
  KernargSegmentPtrUserSGPR = TRI.getMatchingSuperReg(
    getNextUserSGPR(), AMDGPU::sub0, &AMDGPU::SReg_64RegClass);
  NumUserSGPRs += 2;
  return KernargSegmentPtrUserSGPR;
}

unsigned SIMachineFunctionInfo::addFlatScratchInit(const SIRegisterInfo &TRI) {
  FlatScratchInitUserSGPR = TRI.getMatchingSuperReg(
    getNextUserSGPR(), AMDGPU::sub0, &AMDGPU::SReg_64RegClass);
  NumUserSGPRs += 2;
  return FlatScratchInitUserSGPR;
}

SIMachineFunctionInfo::SpilledReg SIMachineFunctionInfo::getSpilledReg(
                                                       MachineFunction *MF,
                                                       unsigned FrameIndex,
                                                       unsigned SubIdx) {
  MachineFrameInfo *FrameInfo = MF->getFrameInfo();
  const SIRegisterInfo *TRI = static_cast<const SIRegisterInfo *>(
      MF->getSubtarget<AMDGPUSubtarget>().getRegisterInfo());
  MachineRegisterInfo &MRI = MF->getRegInfo();
  int64_t Offset = FrameInfo->getObjectOffset(FrameIndex);
  Offset += SubIdx * 4;

  unsigned LaneVGPRIdx = Offset / (64 * 4);
  unsigned Lane = (Offset / 4) % 64;

  struct SpilledReg Spill;
  Spill.Lane = Lane;

  if (!LaneVGPRs.count(LaneVGPRIdx)) {
    unsigned LaneVGPR = TRI->findUnusedRegister(MRI, &AMDGPU::VGPR_32RegClass);

    if (LaneVGPR == AMDGPU::NoRegister)
      // We have no VGPRs left for spilling SGPRs.
      return Spill;


    LaneVGPRs[LaneVGPRIdx] = LaneVGPR;

    // Add this register as live-in to all blocks to avoid machine verifer
    // complaining about use of an undefined physical register.
    for (MachineFunction::iterator BI = MF->begin(), BE = MF->end();
         BI != BE; ++BI) {
      BI->addLiveIn(LaneVGPR);
    }
  }

  Spill.VGPR = LaneVGPRs[LaneVGPRIdx];
  return Spill;
}

unsigned SIMachineFunctionInfo::getMaximumWorkGroupSize(
                                              const MachineFunction &MF) const {
  const AMDGPUSubtarget &ST = MF.getSubtarget<AMDGPUSubtarget>();
  // FIXME: We should get this information from kernel attributes if it
  // is available.
  return getShaderType() == ShaderType::COMPUTE ? 256 : ST.getWavefrontSize();
}
