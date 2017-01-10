//===- LoopPassManager.h - Loop pass management -----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This header provides classes for managing passes over loops in LLVM IR.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_LOOPPASSMANAGER_H
#define LLVM_ANALYSIS_LOOPPASSMANAGER_H

#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"

namespace llvm {

extern template class PassManager<Loop>;
/// \brief The loop pass manager.
///
/// See the documentation for the PassManager template for details. It runs a
/// sequency of loop passes over each loop that the manager is run over. This
/// typedef serves as a convenient way to refer to this construct.
typedef PassManager<Loop> LoopPassManager;

extern template class AnalysisManager<Loop>;
/// \brief The loop analysis manager.
///
/// See the documentation for the AnalysisManager template for detail
/// documentation. This typedef serves as a convenient way to refer to this
/// construct in the adaptors and proxies used to integrate this into the larger
/// pass manager infrastructure.
typedef AnalysisManager<Loop> LoopAnalysisManager;

/// A proxy from a \c LoopAnalysisManager to a \c Function.
typedef InnerAnalysisManagerProxy<LoopAnalysisManager, Function>
    LoopAnalysisManagerFunctionProxy;

/// Specialization of the invalidate method for the \c
/// LoopAnalysisManagerFunctionProxy's result.
template <>
bool LoopAnalysisManagerFunctionProxy::Result::invalidate(
    Function &F, const PreservedAnalyses &PA,
    FunctionAnalysisManager::Invalidator &Inv);

// Ensure the \c LoopAnalysisManagerFunctionProxy is provided as an extern
// template.
extern template class InnerAnalysisManagerProxy<LoopAnalysisManager, Function>;

extern template class OuterAnalysisManagerProxy<FunctionAnalysisManager, Loop>;
/// A proxy from a \c FunctionAnalysisManager to a \c Loop.
typedef OuterAnalysisManagerProxy<FunctionAnalysisManager, Loop>
    FunctionAnalysisManagerLoopProxy;

/// Returns the minimum set of Analyses that all loop passes must preserve.
PreservedAnalyses getLoopPassPreservedAnalyses();

/// \brief Adaptor that maps from a function to its loops.
///
/// Designed to allow composition of a LoopPass(Manager) and a
/// FunctionPassManager. Note that if this pass is constructed with a \c
/// FunctionAnalysisManager it will run the \c LoopAnalysisManagerFunctionProxy
/// analysis prior to running the loop passes over the function to enable a \c
/// LoopAnalysisManager to be used within this run safely.
template <typename LoopPassT>
class FunctionToLoopPassAdaptor
    : public PassInfoMixin<FunctionToLoopPassAdaptor<LoopPassT>> {
public:
  explicit FunctionToLoopPassAdaptor(LoopPassT Pass)
      : Pass(std::move(Pass)) {}

  /// \brief Runs the loop passes across every loop in the function.
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    // Setup the loop analysis manager from its proxy.
    LoopAnalysisManager &LAM =
        AM.getResult<LoopAnalysisManagerFunctionProxy>(F).getManager();
    // Get the loop structure for this function
    LoopInfo &LI = AM.getResult<LoopAnalysis>(F);

    // Also precompute all of the function analyses used by loop passes.
    // FIXME: These should be handed into the loop passes when the loop pass
    // management layer is reworked to follow the design of CGSCC.
    (void)AM.getResult<AAManager>(F);
    (void)AM.getResult<DominatorTreeAnalysis>(F);
    (void)AM.getResult<ScalarEvolutionAnalysis>(F);
    (void)AM.getResult<TargetLibraryAnalysis>(F);

    PreservedAnalyses PA = PreservedAnalyses::all();

    // We want to visit the loops in reverse post-order. We'll build the stack
    // of loops to visit in Loops by first walking the loops in pre-order.
    SmallVector<Loop *, 2> Loops;
    SmallVector<Loop *, 2> WorkList(LI.begin(), LI.end());
    while (!WorkList.empty()) {
      Loop *L = WorkList.pop_back_val();
      WorkList.insert(WorkList.end(), L->begin(), L->end());
      Loops.push_back(L);
    }

    // Now pop each element off of the stack to visit the loops in reverse
    // post-order.
    for (auto *L : reverse(Loops)) {
      PreservedAnalyses PassPA = Pass.run(*L, LAM);
      // FIXME: We should verify the set of analyses relevant to Loop passes
      // are preserved.

      // We know that the loop pass couldn't have invalidated any other loop's
      // analyses (that's the contract of a loop pass), so directly handle the
      // loop analysis manager's invalidation here.
      LAM.invalidate(*L, PassPA);

      // Then intersect the preserved set so that invalidation of module
      // analyses will eventually occur when the module pass completes.
      PA.intersect(std::move(PassPA));
    }

    // By definition we preserve the proxy. We also preserve all analyses on
    // Loops. This precludes *any* invalidation of loop analyses by the proxy,
    // but that's OK because we've taken care to invalidate analyses in the
    // loop analysis manager incrementally above.
    PA.preserveSet<AllAnalysesOn<Loop>>();
    PA.preserve<LoopAnalysisManagerFunctionProxy>();
    return PA;
  }

private:
  LoopPassT Pass;
};

/// \brief A function to deduce a loop pass type and wrap it in the templated
/// adaptor.
template <typename LoopPassT>
FunctionToLoopPassAdaptor<LoopPassT>
createFunctionToLoopPassAdaptor(LoopPassT Pass) {
  return FunctionToLoopPassAdaptor<LoopPassT>(std::move(Pass));
}
}

#endif // LLVM_ANALYSIS_LOOPPASSMANAGER_H
