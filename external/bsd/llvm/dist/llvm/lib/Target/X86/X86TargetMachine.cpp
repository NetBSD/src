//===-- X86TargetMachine.cpp - Define TargetMachine for the X86 -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the X86 specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#include "X86TargetMachine.h"
#include "X86.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/PassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

extern "C" void LLVMInitializeX86Target() {
  // Register the target.
  RegisterTargetMachine<X86TargetMachine> X(TheX86_32Target);
  RegisterTargetMachine<X86TargetMachine> Y(TheX86_64Target);
}

void X86TargetMachine::anchor() { }

/// X86TargetMachine ctor - Create an X86 target.
///
X86TargetMachine::X86TargetMachine(const Target &T, StringRef TT, StringRef CPU,
                                   StringRef FS, const TargetOptions &Options,
                                   Reloc::Model RM, CodeModel::Model CM,
                                   CodeGenOpt::Level OL)
    : LLVMTargetMachine(T, TT, CPU, FS, Options, RM, CM, OL),
      Subtarget(TT, CPU, FS, *this, Options.StackAlignmentOverride) {
  // default to hard float ABI
  if (Options.FloatABIType == FloatABI::Default)
    this->Options.FloatABIType = FloatABI::Hard;

  // Windows stack unwinder gets confused when execution flow "falls through"
  // after a call to 'noreturn' function.
  // To prevent that, we emit a trap for 'unreachable' IR instructions.
  // (which on X86, happens to be the 'ud2' instruction)
  if (Subtarget.isTargetWin64())
    this->Options.TrapUnreachable = true;

  initAsmInfo();
}

//===----------------------------------------------------------------------===//
// Command line options for x86
//===----------------------------------------------------------------------===//
static cl::opt<bool>
UseVZeroUpper("x86-use-vzeroupper", cl::Hidden,
  cl::desc("Minimize AVX to SSE transition penalty"),
  cl::init(true));

//===----------------------------------------------------------------------===//
// X86 Analysis Pass Setup
//===----------------------------------------------------------------------===//

void X86TargetMachine::addAnalysisPasses(PassManagerBase &PM) {
  // Add first the target-independent BasicTTI pass, then our X86 pass. This
  // allows the X86 pass to delegate to the target independent layer when
  // appropriate.
  PM.add(createBasicTargetTransformInfoPass(this));
  PM.add(createX86TargetTransformInfoPass(this));
}


//===----------------------------------------------------------------------===//
// Pass Pipeline Configuration
//===----------------------------------------------------------------------===//

namespace {
/// X86 Code Generator Pass Configuration Options.
class X86PassConfig : public TargetPassConfig {
public:
  X86PassConfig(X86TargetMachine *TM, PassManagerBase &PM)
    : TargetPassConfig(TM, PM) {}

  X86TargetMachine &getX86TargetMachine() const {
    return getTM<X86TargetMachine>();
  }

  const X86Subtarget &getX86Subtarget() const {
    return *getX86TargetMachine().getSubtargetImpl();
  }

  void addIRPasses() override;
  bool addInstSelector() override;
  bool addILPOpts() override;
  bool addPreRegAlloc() override;
  bool addPostRegAlloc() override;
  bool addPreEmitPass() override;
};
} // namespace

TargetPassConfig *X86TargetMachine::createPassConfig(PassManagerBase &PM) {
  return new X86PassConfig(this, PM);
}

void X86PassConfig::addIRPasses() {
  addPass(createX86AtomicExpandPass(&getX86TargetMachine()));

  TargetPassConfig::addIRPasses();
}

bool X86PassConfig::addInstSelector() {
  // Install an instruction selector.
  addPass(createX86ISelDag(getX86TargetMachine(), getOptLevel()));

  // For ELF, cleanup any local-dynamic TLS accesses.
  if (getX86Subtarget().isTargetELF() && getOptLevel() != CodeGenOpt::None)
    addPass(createCleanupLocalDynamicTLSPass());

  addPass(createX86GlobalBaseRegPass());

  return false;
}

bool X86PassConfig::addILPOpts() {
  addPass(&EarlyIfConverterID);
  return true;
}

bool X86PassConfig::addPreRegAlloc() {
  return false;  // -print-machineinstr shouldn't print after this.
}

bool X86PassConfig::addPostRegAlloc() {
  addPass(createX86FloatingPointStackifierPass());
  return true;  // -print-machineinstr should print after this.
}

bool X86PassConfig::addPreEmitPass() {
  bool ShouldPrint = false;
  if (getOptLevel() != CodeGenOpt::None && getX86Subtarget().hasSSE2()) {
    addPass(createExecutionDependencyFixPass(&X86::VR128RegClass));
    ShouldPrint = true;
  }

  if (UseVZeroUpper) {
    addPass(createX86IssueVZeroUpperPass());
    ShouldPrint = true;
  }

  if (getOptLevel() != CodeGenOpt::None) {
    addPass(createX86PadShortFunctions());
    addPass(createX86FixupLEAs());
    ShouldPrint = true;
  }

  return ShouldPrint;
}

bool X86TargetMachine::addCodeEmitter(PassManagerBase &PM,
                                      JITCodeEmitter &JCE) {
  PM.add(createX86JITCodeEmitterPass(*this, JCE));

  return false;
}
