//===-- PPCTargetMachine.cpp - Define TargetMachine for PowerPC -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Top-level implementation for the PowerPC target.
//
//===----------------------------------------------------------------------===//

#include "PPCTargetMachine.h"
#include "PPC.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

static cl::
opt<bool> DisableCTRLoops("disable-ppc-ctrloops", cl::Hidden,
                        cl::desc("Disable CTR loops for PPC"));

static cl::opt<bool>
VSXFMAMutateEarly("schedule-ppc-vsx-fma-mutation-early",
  cl::Hidden, cl::desc("Schedule VSX FMA instruction mutation early"));

extern "C" void LLVMInitializePowerPCTarget() {
  // Register the targets
  RegisterTargetMachine<PPC32TargetMachine> A(ThePPC32Target);
  RegisterTargetMachine<PPC64TargetMachine> B(ThePPC64Target);
  RegisterTargetMachine<PPC64TargetMachine> C(ThePPC64LETarget);
}

/// Return the datalayout string of a subtarget.
static std::string getDataLayoutString(const PPCSubtarget &ST) {
  const Triple &T = ST.getTargetTriple();

  std::string Ret;

  // Most PPC* platforms are big endian, PPC64LE is little endian.
  if (ST.isLittleEndian())
    Ret = "e";
  else
    Ret = "E";

  Ret += DataLayout::getManglingComponent(T);

  // PPC32 has 32 bit pointers. The PS3 (OS Lv2) is a PPC64 machine with 32 bit
  // pointers.
  if (!ST.isPPC64() || T.getOS() == Triple::Lv2)
    Ret += "-p:32:32";

  // Note, the alignment values for f64 and i64 on ppc64 in Darwin
  // documentation are wrong; these are correct (i.e. "what gcc does").
  if (ST.isPPC64() || ST.isSVR4ABI())
    Ret += "-i64:64";
  else
    Ret += "-f64:32:64";

  // PPC64 has 32 and 64 bit registers, PPC32 has only 32 bit ones.
  if (ST.isPPC64())
    Ret += "-n32:64";
  else
    Ret += "-n32";

  return Ret;
}

PPCTargetMachine::PPCTargetMachine(const Target &T, StringRef TT,
                                   StringRef CPU, StringRef FS,
                                   const TargetOptions &Options,
                                   Reloc::Model RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL,
                                   bool is64Bit)
  : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
    Subtarget(TT, CPU, FS, is64Bit, OL),
    DL(getDataLayoutString(Subtarget)), InstrInfo(*this),
    FrameLowering(Subtarget), JITInfo(*this, is64Bit),
    TLInfo(*this), TSInfo(*this),
    InstrItins(Subtarget.getInstrItineraryData()) {
  initAsmInfo();
}

void PPC32TargetMachine::anchor() { }

PPC32TargetMachine::PPC32TargetMachine(const Target &T, StringRef TT,
                                       StringRef CPU, StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL)
  : PPCTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, false) {
}

void PPC64TargetMachine::anchor() { }

PPC64TargetMachine::PPC64TargetMachine(const Target &T, StringRef TT,
                                       StringRef CPU,  StringRef FS,
                                       const TargetOptions &Options,
                                       Reloc::Model RM, CodeModel::Model CM,
                                       CodeGenOpt::Level OL)
  : PPCTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL, true) {
}


//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

namespace {
/// PPC Code Generator Pass Configuration Options.
class PPCPassConfig : public TargetPassConfig {
public:
  PPCPassConfig(PPCTargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  PPCTargetMachine &getPPCTargetMachine() const {
    return getTM<PPCTargetMachine>();
  }

  const PPCSubtarget &getPPCSubtarget() const {
    return *getPPCTargetMachine().getSubtargetImpl();
  }

  bool addPreISel() override;
  bool addILPOpts() override;
  bool addInstSelector() override;
  bool addPreRegAlloc() override;
  bool addPreSched2() override;
  bool addPreEmitPass() override;
};
} // namespace

TargetPassConfig *PPCTargetMachine::createPassConfig(PassManagerBase &PM) {
  return new PPCPassConfig(this, PM);
}

bool PPCPassConfig::addPreISel() {
  if (!DisableCTRLoops && getOptLevel() != CodeGenOpt::None)
    addPass(createPPCCTRLoops(getPPCTargetMachine()));

  return false;
}

bool PPCPassConfig::addILPOpts() {
  addPass(&EarlyIfConverterID);
  return true;
}

bool PPCPassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createPPCISelDag(getPPCTargetMachine()));

#ifndef NDEBUG
  if (!DisableCTRLoops && getOptLevel() != CodeGenOpt::None)
    addPass(createPPCCTRLoopsVerify());
#endif

  addPass(createPPCVSXCopyPass());
  return false;
}

bool PPCPassConfig::addPreRegAlloc() {
  initializePPCVSXFMAMutatePass(*PassRegistry::getPassRegistry());
  insertPass(VSXFMAMutateEarly ? &RegisterCoalescerID : &MachineSchedulerID,
             &PPCVSXFMAMutateID);
  return false;
}

bool PPCPassConfig::addPreSched2() {
  addPass(createPPCVSXCopyCleanupPass());

  if (getOptLevel() != CodeGenOpt::None)
    addPass(&IfConverterID);

  return true;
}

bool PPCPassConfig::addPreEmitPass() {
  if (getOptLevel() != CodeGenOpt::None)
    addPass(createPPCEarlyReturnPass());
  // Must run branch selection immediately preceding the asm printer.
  addPass(createPPCBranchSelectionPass());
  return false;
}

bool PPCTargetMachine::addCodeEmitter(PassManagerBase &PM,
                                      JITCodeEmitter &JCE) {
  // Inform the subtarget that we are in JIT mode.  FIXME: does this break macho
  // writing?
  Subtarget.SetJITMode();

  // Machine code emitter pass for PowerPC.
  PM.add(createPPCJITCodeEmitterPass(*this, JCE));

  return false;
}

void PPCTargetMachine::addAnalysisPasses(PassManagerBase &PM) {
  // Add first the target-independent BasicTTI pass, then our PPC pass. This
  // allows the PPC pass to delegate to the target independent layer when
  // appropriate.
  PM.add(createBasicTargetTransformInfoPass(this));
  PM.add(createPPCTargetTransformInfoPass(this));
}

