//===------- llvm/IR/OptBisect/Bisect.cpp - LLVM Bisect support --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements support for a bisecting optimizations based on a
/// command line option.
///
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/OptBisect.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

static cl::opt<int> OptBisectLimit("opt-bisect-limit", cl::Hidden,
                                   cl::init(INT_MAX), cl::Optional,
                                   cl::desc("Maximum optimization to perform"));

OptBisect::OptBisect() {
  BisectEnabled = OptBisectLimit != INT_MAX;
}

static void printPassMessage(const StringRef &Name, int PassNum,
                             StringRef TargetDesc, bool Running) {
  StringRef Status = Running ? "" : "NOT ";
  errs() << "BISECT: " << Status << "running pass "
         << "(" << PassNum << ") " << Name << " on " << TargetDesc << "\n";
}

static void printCaseMessage(int CaseNum, StringRef Msg, bool Running) {
  if (Running)
    errs() << "BISECT: running case (";
  else
    errs() << "BISECT: NOT running case (";
  errs() << CaseNum << "): " << Msg << "\n";
}

static std::string getDescription(const Module &M) {
  return "module (" + M.getName().str() + ")";
}

static std::string getDescription(const Function &F) {
  return "function (" + F.getName().str() + ")";
}

static std::string getDescription(const BasicBlock &BB) {
  return "basic block (" + BB.getName().str() + ") in function (" +
         BB.getParent()->getName().str() + ")";
}

static std::string getDescription(const Loop &L) {
  // FIXME: I'd like to be able to provide a better description here, but
  //        calling L->getHeader() would introduce a new dependency on the
  //        LLVMCore library.
  return "loop";
}

static std::string getDescription(const CallGraphSCC &SCC) {
  std::string Desc = "SCC (";
  bool First = true;
  for (CallGraphNode *CGN : SCC) {
    if (First)
      First = false;
    else
      Desc += ", ";
    Function *F = CGN->getFunction();
    if (F)
      Desc += F->getName();
    else
      Desc += "<<null function>>";
  }
  Desc += ")";
  return Desc;
}

// Force instantiations.
template bool OptBisect::shouldRunPass(const Pass *, const Module &);
template bool OptBisect::shouldRunPass(const Pass *, const Function &);
template bool OptBisect::shouldRunPass(const Pass *, const BasicBlock &);
template bool OptBisect::shouldRunPass(const Pass *, const Loop &);
template bool OptBisect::shouldRunPass(const Pass *, const CallGraphSCC &);

template <class UnitT>
bool OptBisect::shouldRunPass(const Pass *P, const UnitT &U) {
  if (!BisectEnabled)
    return true;
  return checkPass(P->getPassName(), getDescription(U));
}

bool OptBisect::checkPass(const StringRef PassName,
                          const StringRef TargetDesc) {
  assert(BisectEnabled);

  int CurBisectNum = ++LastBisectNum;
  bool ShouldRun = (OptBisectLimit == -1 || CurBisectNum <= OptBisectLimit);
  printPassMessage(PassName, CurBisectNum, TargetDesc, ShouldRun);
  return ShouldRun;
}

bool OptBisect::shouldRunCase(const Twine &Msg) {
  if (!BisectEnabled)
    return true;
  int CurFuelNum = ++LastBisectNum;
  bool ShouldRun = (OptBisectLimit == -1 || CurFuelNum <= OptBisectLimit);
  printCaseMessage(CurFuelNum, Msg.str(), ShouldRun);
  return ShouldRun;
}

