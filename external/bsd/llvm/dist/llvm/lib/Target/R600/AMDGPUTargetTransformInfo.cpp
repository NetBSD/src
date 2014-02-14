//===-- AMDGPUTargetTransformInfo.cpp - AMDGPU specific TTI pass ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// \file
// This file implements a TargetTransformInfo analysis pass specific to the
// AMDGPU target machine. It uses the target's detailed information to provide
// more precise answers to certain TTI queries, while letting the target
// independent and default TTI implementations handle the rest.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "AMDGPUtti"
#include "AMDGPU.h"
#include "AMDGPUTargetMachine.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/CostTable.h"
#include "llvm/Target/TargetLowering.h"
using namespace llvm;

// Declare the pass initialization routine locally as target-specific passes
// don't have a target-wide initialization entry point, and so we rely on the
// pass constructor initialization.
namespace llvm {
void initializeAMDGPUTTIPass(PassRegistry &);
}

namespace {

class AMDGPUTTI LLVM_FINAL : public ImmutablePass, public TargetTransformInfo {
  const AMDGPUTargetMachine *TM;
  const AMDGPUSubtarget *ST;
  const AMDGPUTargetLowering *TLI;

  /// Estimate the overhead of scalarizing an instruction. Insert and Extract
  /// are set if the result needs to be inserted and/or extracted from vectors.
  unsigned getScalarizationOverhead(Type *Ty, bool Insert, bool Extract) const;

public:
  AMDGPUTTI() : ImmutablePass(ID), TM(0), ST(0), TLI(0) {
    llvm_unreachable("This pass cannot be directly constructed");
  }

  AMDGPUTTI(const AMDGPUTargetMachine *TM)
      : ImmutablePass(ID), TM(TM), ST(TM->getSubtargetImpl()),
        TLI(TM->getTargetLowering()) {
    initializeAMDGPUTTIPass(*PassRegistry::getPassRegistry());
  }

  virtual void initializePass() LLVM_OVERRIDE { pushTTIStack(this); }

  virtual void finalizePass() { popTTIStack(); }

  virtual void getAnalysisUsage(AnalysisUsage &AU) const LLVM_OVERRIDE {
    TargetTransformInfo::getAnalysisUsage(AU);
  }

  /// Pass identification.
  static char ID;

  /// Provide necessary pointer adjustments for the two base classes.
  virtual void *getAdjustedAnalysisPointer(const void *ID) LLVM_OVERRIDE {
    if (ID == &TargetTransformInfo::ID)
      return (TargetTransformInfo *)this;
    return this;
  }

  virtual bool hasBranchDivergence() const LLVM_OVERRIDE;

  virtual void getUnrollingPreferences(Loop *L, UnrollingPreferences &UP) const;

  /// @}
};

} // end anonymous namespace

INITIALIZE_AG_PASS(AMDGPUTTI, TargetTransformInfo, "AMDGPUtti",
                   "AMDGPU Target Transform Info", true, true, false)
char AMDGPUTTI::ID = 0;

ImmutablePass *
llvm::createAMDGPUTargetTransformInfoPass(const AMDGPUTargetMachine *TM) {
  return new AMDGPUTTI(TM);
}

bool AMDGPUTTI::hasBranchDivergence() const { return true; }

void AMDGPUTTI::getUnrollingPreferences(Loop *L,
                                        UnrollingPreferences &UP) const {
  for (Loop::block_iterator BI = L->block_begin(), BE = L->block_end();
                                                  BI != BE; ++BI) {
    BasicBlock *BB = *BI;
    for (BasicBlock::const_iterator I = BB->begin(), E = BB->end();
                                                      I != E; ++I) {
      const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(I);
      if (!GEP)
        continue;
      const Value *Ptr = GEP->getPointerOperand();
      const AllocaInst *Alloca = dyn_cast<AllocaInst>(GetUnderlyingObject(Ptr));
      if (Alloca) {
        // We want to do whatever we can to limit the number of alloca
        // instructions that make it through to the code generator.  allocas
        // require us to use indirect addressing, which is slow and prone to
        // compiler bugs.  If this loop does an address calculation on an
        // alloca ptr, then we want to unconditionally unroll the loop.  In most
        // cases, this will make it possible for SROA to eliminate these allocas.
        UP.Threshold = UINT_MAX;
      }
    }
  }
}
