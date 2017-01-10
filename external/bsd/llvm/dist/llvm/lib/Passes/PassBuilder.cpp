//===- Parsing, selection, and construction of pass pipelines -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// This file provides the implementation of the PassBuilder based on our
/// static pass registry as well as related functionality. It also provides
/// helpers to aid in analyzing, debugging, and testing passes and pass
/// pipelines.
///
//===----------------------------------------------------------------------===//

#include "llvm/Passes/PassBuilder.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasAnalysisEvaluator.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/CFLAndersAliasAnalysis.h"
#include "llvm/Analysis/CFLSteensAliasAnalysis.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/IVUsers.h"
#include "llvm/Analysis/LazyCallGraph.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/Analysis/OptimizationDiagnosticInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ProfileSummaryInfo.h"
#include "llvm/Analysis/RegionInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/Analysis/ScopedNoAliasAA.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#include "llvm/CodeGen/PreISelIntrinsicLowering.h"
#include "llvm/CodeGen/UnreachableBlockElim.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Regex.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/GCOVProfiler.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "llvm/Transforms/IPO/ConstantMerge.h"
#include "llvm/Transforms/IPO/CrossDSOCFI.h"
#include "llvm/Transforms/IPO/DeadArgumentElimination.h"
#include "llvm/Transforms/IPO/ElimAvailExtern.h"
#include "llvm/Transforms/IPO/ForceFunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/IPO/FunctionImport.h"
#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/Transforms/IPO/GlobalOpt.h"
#include "llvm/Transforms/IPO/GlobalSplit.h"
#include "llvm/Transforms/IPO/InferFunctionAttrs.h"
#include "llvm/Transforms/IPO/Inliner.h"
#include "llvm/Transforms/IPO/Internalize.h"
#include "llvm/Transforms/IPO/LowerTypeTests.h"
#include "llvm/Transforms/IPO/PartialInlining.h"
#include "llvm/Transforms/IPO/SCCP.h"
#include "llvm/Transforms/IPO/StripDeadPrototypes.h"
#include "llvm/Transforms/IPO/WholeProgramDevirt.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/InstrProfiling.h"
#include "llvm/Transforms/PGOInstrumentation.h"
#include "llvm/Transforms/SampleProfile.h"
#include "llvm/Transforms/Scalar/ADCE.h"
#include "llvm/Transforms/Scalar/AlignmentFromAssumptions.h"
#include "llvm/Transforms/Scalar/BDCE.h"
#include "llvm/Transforms/Scalar/ConstantHoisting.h"
#include "llvm/Transforms/Scalar/CorrelatedValuePropagation.h"
#include "llvm/Transforms/Scalar/DCE.h"
#include "llvm/Transforms/Scalar/DeadStoreElimination.h"
#include "llvm/Transforms/Scalar/EarlyCSE.h"
#include "llvm/Transforms/Scalar/Float2Int.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/GuardWidening.h"
#include "llvm/Transforms/Scalar/IndVarSimplify.h"
#include "llvm/Transforms/Scalar/JumpThreading.h"
#include "llvm/Transforms/Scalar/LICM.h"
#include "llvm/Transforms/Scalar/LoopDataPrefetch.h"
#include "llvm/Transforms/Scalar/LoopDeletion.h"
#include "llvm/Transforms/Scalar/LoopDistribute.h"
#include "llvm/Transforms/Scalar/LoopIdiomRecognize.h"
#include "llvm/Transforms/Scalar/LoopInstSimplify.h"
#include "llvm/Transforms/Scalar/LoopRotation.h"
#include "llvm/Transforms/Scalar/LoopSimplifyCFG.h"
#include "llvm/Transforms/Scalar/LoopStrengthReduce.h"
#include "llvm/Transforms/Scalar/LoopUnrollPass.h"
#include "llvm/Transforms/Scalar/LowerAtomic.h"
#include "llvm/Transforms/Scalar/LowerExpectIntrinsic.h"
#include "llvm/Transforms/Scalar/LowerGuardIntrinsic.h"
#include "llvm/Transforms/Scalar/MemCpyOptimizer.h"
#include "llvm/Transforms/Scalar/MergedLoadStoreMotion.h"
#include "llvm/Transforms/Scalar/NaryReassociate.h"
#include "llvm/Transforms/Scalar/NewGVN.h"
#include "llvm/Transforms/Scalar/PartiallyInlineLibCalls.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SCCP.h"
#include "llvm/Transforms/Scalar/SROA.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Scalar/Sink.h"
#include "llvm/Transforms/Scalar/SpeculativeExecution.h"
#include "llvm/Transforms/Scalar/TailRecursionElimination.h"
#include "llvm/Transforms/Utils/AddDiscriminators.h"
#include "llvm/Transforms/Utils/BreakCriticalEdges.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LibCallsShrinkWrap.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LowerInvoke.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
#include "llvm/Transforms/Utils/MemorySSA.h"
#include "llvm/Transforms/Utils/NameAnonGlobals.h"
#include "llvm/Transforms/Utils/SimplifyInstructions.h"
#include "llvm/Transforms/Utils/SymbolRewriter.h"
#include "llvm/Transforms/Vectorize/LoopVectorize.h"
#include "llvm/Transforms/Vectorize/SLPVectorizer.h"

#include <type_traits>

using namespace llvm;

static Regex DefaultAliasRegex("^(default|lto-pre-link|lto)<(O[0123sz])>$");

static bool isOptimizingForSize(PassBuilder::OptimizationLevel Level) {
  switch (Level) {
  case PassBuilder::O0:
  case PassBuilder::O1:
  case PassBuilder::O2:
  case PassBuilder::O3:
    return false;

  case PassBuilder::Os:
  case PassBuilder::Oz:
    return true;
  }
  llvm_unreachable("Invalid optimization level!");
}

namespace {

/// \brief No-op module pass which does nothing.
struct NoOpModulePass {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &) {
    return PreservedAnalyses::all();
  }
  static StringRef name() { return "NoOpModulePass"; }
};

/// \brief No-op module analysis.
class NoOpModuleAnalysis : public AnalysisInfoMixin<NoOpModuleAnalysis> {
  friend AnalysisInfoMixin<NoOpModuleAnalysis>;
  static AnalysisKey Key;

public:
  struct Result {};
  Result run(Module &, ModuleAnalysisManager &) { return Result(); }
  static StringRef name() { return "NoOpModuleAnalysis"; }
};

/// \brief No-op CGSCC pass which does nothing.
struct NoOpCGSCCPass {
  PreservedAnalyses run(LazyCallGraph::SCC &C, CGSCCAnalysisManager &,
                        LazyCallGraph &, CGSCCUpdateResult &UR) {
    return PreservedAnalyses::all();
  }
  static StringRef name() { return "NoOpCGSCCPass"; }
};

/// \brief No-op CGSCC analysis.
class NoOpCGSCCAnalysis : public AnalysisInfoMixin<NoOpCGSCCAnalysis> {
  friend AnalysisInfoMixin<NoOpCGSCCAnalysis>;
  static AnalysisKey Key;

public:
  struct Result {};
  Result run(LazyCallGraph::SCC &, CGSCCAnalysisManager &, LazyCallGraph &G) {
    return Result();
  }
  static StringRef name() { return "NoOpCGSCCAnalysis"; }
};

/// \brief No-op function pass which does nothing.
struct NoOpFunctionPass {
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    return PreservedAnalyses::all();
  }
  static StringRef name() { return "NoOpFunctionPass"; }
};

/// \brief No-op function analysis.
class NoOpFunctionAnalysis : public AnalysisInfoMixin<NoOpFunctionAnalysis> {
  friend AnalysisInfoMixin<NoOpFunctionAnalysis>;
  static AnalysisKey Key;

public:
  struct Result {};
  Result run(Function &, FunctionAnalysisManager &) { return Result(); }
  static StringRef name() { return "NoOpFunctionAnalysis"; }
};

/// \brief No-op loop pass which does nothing.
struct NoOpLoopPass {
  PreservedAnalyses run(Loop &L, LoopAnalysisManager &) {
    return PreservedAnalyses::all();
  }
  static StringRef name() { return "NoOpLoopPass"; }
};

/// \brief No-op loop analysis.
class NoOpLoopAnalysis : public AnalysisInfoMixin<NoOpLoopAnalysis> {
  friend AnalysisInfoMixin<NoOpLoopAnalysis>;
  static AnalysisKey Key;

public:
  struct Result {};
  Result run(Loop &, LoopAnalysisManager &) { return Result(); }
  static StringRef name() { return "NoOpLoopAnalysis"; }
};

AnalysisKey NoOpModuleAnalysis::Key;
AnalysisKey NoOpCGSCCAnalysis::Key;
AnalysisKey NoOpFunctionAnalysis::Key;
AnalysisKey NoOpLoopAnalysis::Key;

} // End anonymous namespace.

void PassBuilder::registerModuleAnalyses(ModuleAnalysisManager &MAM) {
#define MODULE_ANALYSIS(NAME, CREATE_PASS)                                     \
  MAM.registerPass([&] { return CREATE_PASS; });
#include "PassRegistry.def"
}

void PassBuilder::registerCGSCCAnalyses(CGSCCAnalysisManager &CGAM) {
#define CGSCC_ANALYSIS(NAME, CREATE_PASS)                                      \
  CGAM.registerPass([&] { return CREATE_PASS; });
#include "PassRegistry.def"
}

void PassBuilder::registerFunctionAnalyses(FunctionAnalysisManager &FAM) {
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                   \
  FAM.registerPass([&] { return CREATE_PASS; });
#include "PassRegistry.def"
}

void PassBuilder::registerLoopAnalyses(LoopAnalysisManager &LAM) {
#define LOOP_ANALYSIS(NAME, CREATE_PASS)                                       \
  LAM.registerPass([&] { return CREATE_PASS; });
#include "PassRegistry.def"
}

FunctionPassManager
PassBuilder::buildFunctionSimplificationPipeline(OptimizationLevel Level,
                                                 bool DebugLogging) {
  assert(Level != O0 && "Must request optimizations!");
  FunctionPassManager FPM(DebugLogging);

  // Form SSA out of local memory accesses after breaking apart aggregates into
  // scalars.
  FPM.addPass(SROA());

  // Catch trivial redundancies
  FPM.addPass(EarlyCSEPass());

  // Speculative execution if the target has divergent branches; otherwise nop.
  FPM.addPass(SpeculativeExecutionPass());

  // Optimize based on known information about branches, and cleanup afterward.
  FPM.addPass(JumpThreadingPass());
  FPM.addPass(CorrelatedValuePropagationPass());
  FPM.addPass(SimplifyCFGPass());
  FPM.addPass(InstCombinePass());

  if (!isOptimizingForSize(Level))
    FPM.addPass(LibCallsShrinkWrapPass());

  FPM.addPass(TailCallElimPass());
  FPM.addPass(SimplifyCFGPass());

  // Form canonically associated expression trees, and simplify the trees using
  // basic mathematical properties. For example, this will form (nearly)
  // minimal multiplication trees.
  FPM.addPass(ReassociatePass());

  // Add the primary loop simplification pipeline.
  // FIXME: Currently this is split into two loop pass pipelines because we run
  // some function passes in between them. These can and should be replaced by
  // loop pass equivalenst but those aren't ready yet. Specifically,
  // `SimplifyCFGPass` and `InstCombinePass` are used. We have
  // `LoopSimplifyCFGPass` which isn't yet powerful enough, and the closest to
  // the other we have is `LoopInstSimplify`.
  LoopPassManager LPM1(DebugLogging), LPM2(DebugLogging);

  // FIXME: Enable these when the loop pass manager can support enforcing loop
  // simplified and LCSSA form as well as updating the loop nest after
  // transformations and we finsih porting the loop passes.
#if 0
  // Rotate Loop - disable header duplication at -Oz
  LPM1.addPass(LoopRotatePass(Level != Oz));
  LPM1.addPass(LICMPass());
  LPM1.addPass(LoopUnswitchPass(/* OptimizeForSize */ Level != O3));
  LPM2.addPass(IndVarSimplifyPass());
  LPM2.addPass(LoopIdiomPass());
  LPM2.addPass(LoopDeletionPass());
  LPM2.addPass(SimpleLoopUnrollPass());
#endif
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM1)));
  FPM.addPass(SimplifyCFGPass());
  FPM.addPass(InstCombinePass());
  FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM2)));

  // Eliminate redundancies.
  if (Level != O1) {
    // These passes add substantial compile time so skip them at O1.
    FPM.addPass(MergedLoadStoreMotionPass());
    FPM.addPass(GVN());
  }

  // Specially optimize memory movement as it doesn't look like dataflow in SSA.
  FPM.addPass(MemCpyOptPass());

  // Sparse conditional constant propagation.
  // FIXME: It isn't clear why we do this *after* loop passes rather than
  // before...
  FPM.addPass(SCCPPass());

  // Delete dead bit computations (instcombine runs after to fold away the dead
  // computations, and then ADCE will run later to exploit any new DCE
  // opportunities that creates).
  FPM.addPass(BDCEPass());

  // Run instcombine after redundancy and dead bit elimination to exploit
  // opportunities opened up by them.
  FPM.addPass(InstCombinePass());

  // Re-consider control flow based optimizations after redundancy elimination,
  // redo DCE, etc.
  FPM.addPass(JumpThreadingPass());
  FPM.addPass(CorrelatedValuePropagationPass());
  FPM.addPass(DSEPass());
  // FIXME: Enable this when the loop pass manager can support enforcing loop
  // simplified and LCSSA form as well as updating the loop nest after
  // transformations and we finsih porting the loop passes.
#if 0
  FPM.addPass(createFunctionToLoopPassAdaptor(LICMPass()));
#endif

  // Finally, do an expensive DCE pass to catch all the dead code exposed by
  // the simplifications and basic cleanup after all the simplifications.
  FPM.addPass(ADCEPass());
  FPM.addPass(SimplifyCFGPass());
  FPM.addPass(InstCombinePass());

  return FPM;
}

ModulePassManager
PassBuilder::buildPerModuleDefaultPipeline(OptimizationLevel Level,
                                           bool DebugLogging) {
  assert(Level != O0 && "Must request optimizations for the default pipeline!");
  ModulePassManager MPM(DebugLogging);

  // Force any function attributes we want the rest of the pipeline te observe.
  MPM.addPass(ForceFunctionAttrsPass());

  // Do basic inference of function attributes from known properties of system
  // libraries and other oracles.
  MPM.addPass(InferFunctionAttrsPass());

  // Create an early function pass manager to cleanup the output of the
  // frontend.
  FunctionPassManager EarlyFPM(DebugLogging);
  EarlyFPM.addPass(SimplifyCFGPass());
  EarlyFPM.addPass(SROA());
  EarlyFPM.addPass(EarlyCSEPass());
  EarlyFPM.addPass(LowerExpectIntrinsicPass());
  EarlyFPM.addPass(GVNHoistPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(EarlyFPM)));

  // Interprocedural constant propagation now that basic cleanup has occured
  // and prior to optimizing globals.
  // FIXME: This position in the pipeline hasn't been carefully considered in
  // years, it should be re-analyzed.
  MPM.addPass(IPSCCPPass());

  // Optimize globals to try and fold them into constants.
  MPM.addPass(GlobalOptPass());

  // Promote any localized globals to SSA registers.
  // FIXME: Should this instead by a run of SROA?
  // FIXME: We should probably run instcombine and simplify-cfg afterward to
  // delete control flows that are dead once globals have been folded to
  // constants.
  MPM.addPass(createModuleToFunctionPassAdaptor(PromotePass()));

  // Remove any dead arguments exposed by cleanups and constand folding
  // globals.
  MPM.addPass(DeadArgumentEliminationPass());

  // Create a small function pass pipeline to cleanup after all the global
  // optimizations.
  FunctionPassManager GlobalCleanupPM(DebugLogging);
  GlobalCleanupPM.addPass(InstCombinePass());
  GlobalCleanupPM.addPass(SimplifyCFGPass());
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(GlobalCleanupPM)));

  // FIXME: Enable this when cross-IR-unit analysis invalidation is working.
#if 0
  MPM.addPass(RequireAnalysisPass<GlobalsAA>());
#endif

  // Now begin the main postorder CGSCC pipeline.
  // FIXME: The current CGSCC pipeline has its origins in the legacy pass
  // manager and trying to emulate its precise behavior. Much of this doesn't
  // make a lot of sense and we should revisit the core CGSCC structure.
  CGSCCPassManager MainCGPipeline(DebugLogging);

  // Note: historically, the PruneEH pass was run first to deduce nounwind and
  // generally clean up exception handling overhead. It isn't clear this is
  // valuable as the inliner doesn't currently care whether it is inlining an
  // invoke or a call.

  // Run the inliner first. The theory is that we are walking bottom-up and so
  // the callees have already been fully optimized, and we want to inline them
  // into the callers so that our optimizations can reflect that.
  // FIXME; Customize the threshold based on optimization level.
  MainCGPipeline.addPass(InlinerPass());

  // Now deduce any function attributes based in the current code.
  MainCGPipeline.addPass(PostOrderFunctionAttrsPass());

  // Lastly, add the core function simplification pipeline nested inside the
  // CGSCC walk.
  MainCGPipeline.addPass(createCGSCCToFunctionPassAdaptor(
      buildFunctionSimplificationPipeline(Level, DebugLogging)));

  MPM.addPass(
      createModuleToPostOrderCGSCCPassAdaptor(std::move(MainCGPipeline)));

  // This ends the canonicalization and simplification phase of the pipeline.
  // At this point, we expect to have canonical and simple IR which we begin
  // *optimizing* for efficient execution going forward.

  // Eliminate externally available functions now that inlining is over -- we
  // won't emit these anyways.
  MPM.addPass(EliminateAvailableExternallyPass());

  // Do RPO function attribute inference across the module to forward-propagate
  // attributes where applicable.
  // FIXME: Is this really an optimization rather than a canonicalization?
  MPM.addPass(ReversePostOrderFunctionAttrsPass());

  // Recompute GloblasAA here prior to function passes. This is particularly
  // useful as the above will have inlined, DCE'ed, and function-attr
  // propagated everything. We should at this point have a reasonably minimal
  // and richly annotated call graph. By computing aliasing and mod/ref
  // information for all local globals here, the late loop passes and notably
  // the vectorizer will be able to use them to help recognize vectorizable
  // memory operations.
  // FIXME: Enable this once analysis invalidation is fully supported.
#if 0
  MPM.addPass(Require<GlobalsAA>());
#endif

  FunctionPassManager OptimizePM(DebugLogging);
  OptimizePM.addPass(Float2IntPass());
  // FIXME: We need to run some loop optimizations to re-rotate loops after
  // simplify-cfg and others undo their rotation.

  // Optimize the loop execution. These passes operate on entire loop nests
  // rather than on each loop in an inside-out manner, and so they are actually
  // function passes.
  OptimizePM.addPass(LoopDistributePass());
#if 0
  // FIXME: LoopVectorize relies on "requiring" LCSSA which isn't supported in
  // the new PM.
  OptimizePM.addPass(LoopVectorizePass());
#endif
  // FIXME: Need to port Loop Load Elimination and add it here.
  OptimizePM.addPass(InstCombinePass());

  // Optimize parallel scalar instruction chains into SIMD instructions.
  OptimizePM.addPass(SLPVectorizerPass());

  // Cleanup after vectorizers.
  OptimizePM.addPass(SimplifyCFGPass());
  OptimizePM.addPass(InstCombinePass());

  // Unroll small loops to hide loop backedge latency and saturate any parallel
  // execution resources of an out-of-order processor.
  // FIXME: Need to add once loop pass pipeline is available.

  // FIXME: Add the loop sink pass when ported.

  // FIXME: Add cleanup from the loop pass manager when we're forming LCSSA
  // here.

  // Now that we've vectorized and unrolled loops, we may have more refined
  // alignment information, try to re-derive it here.
  OptimizePM.addPass(AlignmentFromAssumptionsPass());

  // ADd the core optimizing pipeline.
  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(OptimizePM)));

  // Now we need to do some global optimization transforms.
  // FIXME: It would seem like these should come first in the optimization
  // pipeline and maybe be the bottom of the canonicalization pipeline? Weird
  // ordering here.
  MPM.addPass(GlobalDCEPass());
  MPM.addPass(ConstantMergePass());

  return MPM;
}

ModulePassManager
PassBuilder::buildLTOPreLinkDefaultPipeline(OptimizationLevel Level,
                                            bool DebugLogging) {
  assert(Level != O0 && "Must request optimizations for the default pipeline!");
  // FIXME: We should use a customized pre-link pipeline!
  return buildPerModuleDefaultPipeline(Level, DebugLogging);
}

ModulePassManager PassBuilder::buildLTODefaultPipeline(OptimizationLevel Level,
                                                       bool DebugLogging) {
  assert(Level != O0 && "Must request optimizations for the default pipeline!");
  ModulePassManager MPM(DebugLogging);

  // FIXME: Finish fleshing this out to match the legacy LTO pipelines.
  FunctionPassManager LateFPM(DebugLogging);
  LateFPM.addPass(InstCombinePass());
  LateFPM.addPass(SimplifyCFGPass());

  MPM.addPass(createModuleToFunctionPassAdaptor(std::move(LateFPM)));

  return MPM;
}

AAManager PassBuilder::buildDefaultAAPipeline() {
  AAManager AA;

  // The order in which these are registered determines their priority when
  // being queried.

  // First we register the basic alias analysis that provides the majority of
  // per-function local AA logic. This is a stateless, on-demand local set of
  // AA techniques.
  AA.registerFunctionAnalysis<BasicAA>();

  // Next we query fast, specialized alias analyses that wrap IR-embedded
  // information about aliasing.
  AA.registerFunctionAnalysis<ScopedNoAliasAA>();
  AA.registerFunctionAnalysis<TypeBasedAA>();

  // Add support for querying global aliasing information when available.
  // Because the `AAManager` is a function analysis and `GlobalsAA` is a module
  // analysis, all that the `AAManager` can do is query for any *cached*
  // results from `GlobalsAA` through a readonly proxy..
#if 0
  // FIXME: Enable once the invalidation logic supports this. Currently, the
  // `AAManager` will hold stale references to the module analyses.
  AA.registerModuleAnalysis<GlobalsAA>();
#endif

  return AA;
}

static Optional<int> parseRepeatPassName(StringRef Name) {
  if (!Name.consume_front("repeat<") || !Name.consume_back(">"))
    return None;
  int Count;
  if (Name.getAsInteger(0, Count) || Count <= 0)
    return None;
  return Count;
}

static Optional<int> parseDevirtPassName(StringRef Name) {
  if (!Name.consume_front("devirt<") || !Name.consume_back(">"))
    return None;
  int Count;
  if (Name.getAsInteger(0, Count) || Count <= 0)
    return None;
  return Count;
}

static bool isModulePassName(StringRef Name) {
  // Manually handle aliases for pre-configured pipeline fragments.
  if (Name.startswith("default") || Name.startswith("lto"))
    return DefaultAliasRegex.match(Name);

  // Explicitly handle pass manager names.
  if (Name == "module")
    return true;
  if (Name == "cgscc")
    return true;
  if (Name == "function")
    return true;

  // Explicitly handle custom-parsed pass names.
  if (parseRepeatPassName(Name))
    return true;

#define MODULE_PASS(NAME, CREATE_PASS)                                         \
  if (Name == NAME)                                                            \
    return true;
#define MODULE_ANALYSIS(NAME, CREATE_PASS)                                     \
  if (Name == "require<" NAME ">" || Name == "invalidate<" NAME ">")           \
    return true;
#include "PassRegistry.def"

  return false;
}

static bool isCGSCCPassName(StringRef Name) {
  // Explicitly handle pass manager names.
  if (Name == "cgscc")
    return true;
  if (Name == "function")
    return true;

  // Explicitly handle custom-parsed pass names.
  if (parseRepeatPassName(Name))
    return true;
  if (parseDevirtPassName(Name))
    return true;

#define CGSCC_PASS(NAME, CREATE_PASS)                                          \
  if (Name == NAME)                                                            \
    return true;
#define CGSCC_ANALYSIS(NAME, CREATE_PASS)                                      \
  if (Name == "require<" NAME ">" || Name == "invalidate<" NAME ">")           \
    return true;
#include "PassRegistry.def"

  return false;
}

static bool isFunctionPassName(StringRef Name) {
  // Explicitly handle pass manager names.
  if (Name == "function")
    return true;
  if (Name == "loop")
    return true;

  // Explicitly handle custom-parsed pass names.
  if (parseRepeatPassName(Name))
    return true;

#define FUNCTION_PASS(NAME, CREATE_PASS)                                       \
  if (Name == NAME)                                                            \
    return true;
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                   \
  if (Name == "require<" NAME ">" || Name == "invalidate<" NAME ">")           \
    return true;
#include "PassRegistry.def"

  return false;
}

static bool isLoopPassName(StringRef Name) {
  // Explicitly handle pass manager names.
  if (Name == "loop")
    return true;

  // Explicitly handle custom-parsed pass names.
  if (parseRepeatPassName(Name))
    return true;

#define LOOP_PASS(NAME, CREATE_PASS)                                           \
  if (Name == NAME)                                                            \
    return true;
#define LOOP_ANALYSIS(NAME, CREATE_PASS)                                       \
  if (Name == "require<" NAME ">" || Name == "invalidate<" NAME ">")           \
    return true;
#include "PassRegistry.def"

  return false;
}

Optional<std::vector<PassBuilder::PipelineElement>>
PassBuilder::parsePipelineText(StringRef Text) {
  std::vector<PipelineElement> ResultPipeline;

  SmallVector<std::vector<PipelineElement> *, 4> PipelineStack = {
      &ResultPipeline};
  for (;;) {
    std::vector<PipelineElement> &Pipeline = *PipelineStack.back();
    size_t Pos = Text.find_first_of(",()");
    Pipeline.push_back({Text.substr(0, Pos), {}});

    // If we have a single terminating name, we're done.
    if (Pos == Text.npos)
      break;

    char Sep = Text[Pos];
    Text = Text.substr(Pos + 1);
    if (Sep == ',')
      // Just a name ending in a comma, continue.
      continue;

    if (Sep == '(') {
      // Push the inner pipeline onto the stack to continue processing.
      PipelineStack.push_back(&Pipeline.back().InnerPipeline);
      continue;
    }

    assert(Sep == ')' && "Bogus separator!");
    // When handling the close parenthesis, we greedily consume them to avoid
    // empty strings in the pipeline.
    do {
      // If we try to pop the outer pipeline we have unbalanced parentheses.
      if (PipelineStack.size() == 1)
        return None;

      PipelineStack.pop_back();
    } while (Text.consume_front(")"));

    // Check if we've finished parsing.
    if (Text.empty())
      break;

    // Otherwise, the end of an inner pipeline always has to be followed by
    // a comma, and then we can continue.
    if (!Text.consume_front(","))
      return None;
  }

  if (PipelineStack.size() > 1)
    // Unbalanced paretheses.
    return None;

  assert(PipelineStack.back() == &ResultPipeline &&
         "Wrong pipeline at the bottom of the stack!");
  return {std::move(ResultPipeline)};
}

bool PassBuilder::parseModulePass(ModulePassManager &MPM,
                                  const PipelineElement &E, bool VerifyEachPass,
                                  bool DebugLogging) {
  auto &Name = E.Name;
  auto &InnerPipeline = E.InnerPipeline;

  // First handle complex passes like the pass managers which carry pipelines.
  if (!InnerPipeline.empty()) {
    if (Name == "module") {
      ModulePassManager NestedMPM(DebugLogging);
      if (!parseModulePassPipeline(NestedMPM, InnerPipeline, VerifyEachPass,
                                   DebugLogging))
        return false;
      MPM.addPass(std::move(NestedMPM));
      return true;
    }
    if (Name == "cgscc") {
      CGSCCPassManager CGPM(DebugLogging);
      if (!parseCGSCCPassPipeline(CGPM, InnerPipeline, VerifyEachPass,
                                  DebugLogging))
        return false;
      MPM.addPass(createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM),
                                                          DebugLogging));
      return true;
    }
    if (Name == "function") {
      FunctionPassManager FPM(DebugLogging);
      if (!parseFunctionPassPipeline(FPM, InnerPipeline, VerifyEachPass,
                                     DebugLogging))
        return false;
      MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
      return true;
    }
    if (auto Count = parseRepeatPassName(Name)) {
      ModulePassManager NestedMPM(DebugLogging);
      if (!parseModulePassPipeline(NestedMPM, InnerPipeline, VerifyEachPass,
                                   DebugLogging))
        return false;
      MPM.addPass(createRepeatedPass(*Count, std::move(NestedMPM)));
      return true;
    }
    // Normal passes can't have pipelines.
    return false;
  }

  // Manually handle aliases for pre-configured pipeline fragments.
  if (Name.startswith("default") || Name.startswith("lto")) {
    SmallVector<StringRef, 3> Matches;
    if (!DefaultAliasRegex.match(Name, &Matches))
      return false;
    assert(Matches.size() == 3 && "Must capture two matched strings!");

    OptimizationLevel L = StringSwitch<OptimizationLevel>(Matches[2])
        .Case("O0", O0)
        .Case("O1", O1)
        .Case("O2", O2)
        .Case("O3", O3)
        .Case("Os", Os)
        .Case("Oz", Oz);
    if (L == O0)
      // At O0 we do nothing at all!
      return true;

    if (Matches[1] == "default") {
      MPM.addPass(buildPerModuleDefaultPipeline(L, DebugLogging));
    } else if (Matches[1] == "lto-pre-link") {
      MPM.addPass(buildLTOPreLinkDefaultPipeline(L, DebugLogging));
    } else {
      assert(Matches[1] == "lto" && "Not one of the matched options!");
      MPM.addPass(buildLTODefaultPipeline(L, DebugLogging));
    }
    return true;
  }

  // Finally expand the basic registered passes from the .inc file.
#define MODULE_PASS(NAME, CREATE_PASS)                                         \
  if (Name == NAME) {                                                          \
    MPM.addPass(CREATE_PASS);                                                  \
    return true;                                                               \
  }
#define MODULE_ANALYSIS(NAME, CREATE_PASS)                                     \
  if (Name == "require<" NAME ">") {                                           \
    MPM.addPass(                                                               \
        RequireAnalysisPass<                                                   \
            std::remove_reference<decltype(CREATE_PASS)>::type, Module>());    \
    return true;                                                               \
  }                                                                            \
  if (Name == "invalidate<" NAME ">") {                                        \
    MPM.addPass(InvalidateAnalysisPass<                                        \
                std::remove_reference<decltype(CREATE_PASS)>::type>());        \
    return true;                                                               \
  }
#include "PassRegistry.def"

  return false;
}

bool PassBuilder::parseCGSCCPass(CGSCCPassManager &CGPM,
                                 const PipelineElement &E, bool VerifyEachPass,
                                 bool DebugLogging) {
  auto &Name = E.Name;
  auto &InnerPipeline = E.InnerPipeline;

  // First handle complex passes like the pass managers which carry pipelines.
  if (!InnerPipeline.empty()) {
    if (Name == "cgscc") {
      CGSCCPassManager NestedCGPM(DebugLogging);
      if (!parseCGSCCPassPipeline(NestedCGPM, InnerPipeline, VerifyEachPass,
                                  DebugLogging))
        return false;
      // Add the nested pass manager with the appropriate adaptor.
      CGPM.addPass(std::move(NestedCGPM));
      return true;
    }
    if (Name == "function") {
      FunctionPassManager FPM(DebugLogging);
      if (!parseFunctionPassPipeline(FPM, InnerPipeline, VerifyEachPass,
                                     DebugLogging))
        return false;
      // Add the nested pass manager with the appropriate adaptor.
      CGPM.addPass(
          createCGSCCToFunctionPassAdaptor(std::move(FPM), DebugLogging));
      return true;
    }
    if (auto Count = parseRepeatPassName(Name)) {
      CGSCCPassManager NestedCGPM(DebugLogging);
      if (!parseCGSCCPassPipeline(NestedCGPM, InnerPipeline, VerifyEachPass,
                                  DebugLogging))
        return false;
      CGPM.addPass(createRepeatedPass(*Count, std::move(NestedCGPM)));
      return true;
    }
    if (auto MaxRepetitions = parseDevirtPassName(Name)) {
      CGSCCPassManager NestedCGPM(DebugLogging);
      if (!parseCGSCCPassPipeline(NestedCGPM, InnerPipeline, VerifyEachPass,
                                  DebugLogging))
        return false;
      CGPM.addPass(createDevirtSCCRepeatedPass(std::move(NestedCGPM),
                                               *MaxRepetitions, DebugLogging));
      return true;
    }
    // Normal passes can't have pipelines.
    return false;
  }

  // Now expand the basic registered passes from the .inc file.
#define CGSCC_PASS(NAME, CREATE_PASS)                                          \
  if (Name == NAME) {                                                          \
    CGPM.addPass(CREATE_PASS);                                                 \
    return true;                                                               \
  }
#define CGSCC_ANALYSIS(NAME, CREATE_PASS)                                      \
  if (Name == "require<" NAME ">") {                                           \
    CGPM.addPass(RequireAnalysisPass<                                          \
                 std::remove_reference<decltype(CREATE_PASS)>::type,           \
                 LazyCallGraph::SCC, CGSCCAnalysisManager, LazyCallGraph &,    \
                 CGSCCUpdateResult &>());                                      \
    return true;                                                               \
  }                                                                            \
  if (Name == "invalidate<" NAME ">") {                                        \
    CGPM.addPass(InvalidateAnalysisPass<                                       \
                 std::remove_reference<decltype(CREATE_PASS)>::type>());       \
    return true;                                                               \
  }
#include "PassRegistry.def"

  return false;
}

bool PassBuilder::parseFunctionPass(FunctionPassManager &FPM,
                                    const PipelineElement &E,
                                    bool VerifyEachPass, bool DebugLogging) {
  auto &Name = E.Name;
  auto &InnerPipeline = E.InnerPipeline;

  // First handle complex passes like the pass managers which carry pipelines.
  if (!InnerPipeline.empty()) {
    if (Name == "function") {
      FunctionPassManager NestedFPM(DebugLogging);
      if (!parseFunctionPassPipeline(NestedFPM, InnerPipeline, VerifyEachPass,
                                     DebugLogging))
        return false;
      // Add the nested pass manager with the appropriate adaptor.
      FPM.addPass(std::move(NestedFPM));
      return true;
    }
    if (Name == "loop") {
      LoopPassManager LPM(DebugLogging);
      if (!parseLoopPassPipeline(LPM, InnerPipeline, VerifyEachPass,
                                 DebugLogging))
        return false;
      // Add the nested pass manager with the appropriate adaptor.
      FPM.addPass(createFunctionToLoopPassAdaptor(std::move(LPM)));
      return true;
    }
    if (auto Count = parseRepeatPassName(Name)) {
      FunctionPassManager NestedFPM(DebugLogging);
      if (!parseFunctionPassPipeline(NestedFPM, InnerPipeline, VerifyEachPass,
                                     DebugLogging))
        return false;
      FPM.addPass(createRepeatedPass(*Count, std::move(NestedFPM)));
      return true;
    }
    // Normal passes can't have pipelines.
    return false;
  }

  // Now expand the basic registered passes from the .inc file.
#define FUNCTION_PASS(NAME, CREATE_PASS)                                       \
  if (Name == NAME) {                                                          \
    FPM.addPass(CREATE_PASS);                                                  \
    return true;                                                               \
  }
#define FUNCTION_ANALYSIS(NAME, CREATE_PASS)                                   \
  if (Name == "require<" NAME ">") {                                           \
    FPM.addPass(                                                               \
        RequireAnalysisPass<                                                   \
            std::remove_reference<decltype(CREATE_PASS)>::type, Function>());  \
    return true;                                                               \
  }                                                                            \
  if (Name == "invalidate<" NAME ">") {                                        \
    FPM.addPass(InvalidateAnalysisPass<                                        \
                std::remove_reference<decltype(CREATE_PASS)>::type>());        \
    return true;                                                               \
  }
#include "PassRegistry.def"

  return false;
}

bool PassBuilder::parseLoopPass(LoopPassManager &LPM, const PipelineElement &E,
                                bool VerifyEachPass, bool DebugLogging) {
  StringRef Name = E.Name;
  auto &InnerPipeline = E.InnerPipeline;

  // First handle complex passes like the pass managers which carry pipelines.
  if (!InnerPipeline.empty()) {
    if (Name == "loop") {
      LoopPassManager NestedLPM(DebugLogging);
      if (!parseLoopPassPipeline(NestedLPM, InnerPipeline, VerifyEachPass,
                                 DebugLogging))
        return false;
      // Add the nested pass manager with the appropriate adaptor.
      LPM.addPass(std::move(NestedLPM));
      return true;
    }
    if (auto Count = parseRepeatPassName(Name)) {
      LoopPassManager NestedLPM(DebugLogging);
      if (!parseLoopPassPipeline(NestedLPM, InnerPipeline, VerifyEachPass,
                                 DebugLogging))
        return false;
      LPM.addPass(createRepeatedPass(*Count, std::move(NestedLPM)));
      return true;
    }
    // Normal passes can't have pipelines.
    return false;
  }

  // Now expand the basic registered passes from the .inc file.
#define LOOP_PASS(NAME, CREATE_PASS)                                           \
  if (Name == NAME) {                                                          \
    LPM.addPass(CREATE_PASS);                                                  \
    return true;                                                               \
  }
#define LOOP_ANALYSIS(NAME, CREATE_PASS)                                       \
  if (Name == "require<" NAME ">") {                                           \
    LPM.addPass(RequireAnalysisPass<                                           \
                std::remove_reference<decltype(CREATE_PASS)>::type, Loop>());  \
    return true;                                                               \
  }                                                                            \
  if (Name == "invalidate<" NAME ">") {                                        \
    LPM.addPass(InvalidateAnalysisPass<                                        \
                std::remove_reference<decltype(CREATE_PASS)>::type>());        \
    return true;                                                               \
  }
#include "PassRegistry.def"

  return false;
}

bool PassBuilder::parseAAPassName(AAManager &AA, StringRef Name) {
#define MODULE_ALIAS_ANALYSIS(NAME, CREATE_PASS)                               \
  if (Name == NAME) {                                                          \
    AA.registerModuleAnalysis<                                                 \
        std::remove_reference<decltype(CREATE_PASS)>::type>();                 \
    return true;                                                               \
  }
#define FUNCTION_ALIAS_ANALYSIS(NAME, CREATE_PASS)                             \
  if (Name == NAME) {                                                          \
    AA.registerFunctionAnalysis<                                               \
        std::remove_reference<decltype(CREATE_PASS)>::type>();                 \
    return true;                                                               \
  }
#include "PassRegistry.def"

  return false;
}

bool PassBuilder::parseLoopPassPipeline(LoopPassManager &LPM,
                                        ArrayRef<PipelineElement> Pipeline,
                                        bool VerifyEachPass,
                                        bool DebugLogging) {
  for (const auto &Element : Pipeline) {
    if (!parseLoopPass(LPM, Element, VerifyEachPass, DebugLogging))
      return false;
    // FIXME: No verifier support for Loop passes!
  }
  return true;
}

bool PassBuilder::parseFunctionPassPipeline(FunctionPassManager &FPM,
                                            ArrayRef<PipelineElement> Pipeline,
                                            bool VerifyEachPass,
                                            bool DebugLogging) {
  for (const auto &Element : Pipeline) {
    if (!parseFunctionPass(FPM, Element, VerifyEachPass, DebugLogging))
      return false;
    if (VerifyEachPass)
      FPM.addPass(VerifierPass());
  }
  return true;
}

bool PassBuilder::parseCGSCCPassPipeline(CGSCCPassManager &CGPM,
                                         ArrayRef<PipelineElement> Pipeline,
                                         bool VerifyEachPass,
                                         bool DebugLogging) {
  for (const auto &Element : Pipeline) {
    if (!parseCGSCCPass(CGPM, Element, VerifyEachPass, DebugLogging))
      return false;
    // FIXME: No verifier support for CGSCC passes!
  }
  return true;
}

void PassBuilder::crossRegisterProxies(LoopAnalysisManager &LAM,
                                       FunctionAnalysisManager &FAM,
                                       CGSCCAnalysisManager &CGAM,
                                       ModuleAnalysisManager &MAM) {
  MAM.registerPass([&] { return FunctionAnalysisManagerModuleProxy(FAM); });
  MAM.registerPass([&] { return CGSCCAnalysisManagerModuleProxy(CGAM); });
  CGAM.registerPass([&] { return ModuleAnalysisManagerCGSCCProxy(MAM); });
  FAM.registerPass([&] { return CGSCCAnalysisManagerFunctionProxy(CGAM); });
  FAM.registerPass([&] { return ModuleAnalysisManagerFunctionProxy(MAM); });
  FAM.registerPass([&] { return LoopAnalysisManagerFunctionProxy(LAM); });
  LAM.registerPass([&] { return FunctionAnalysisManagerLoopProxy(FAM); });
}

bool PassBuilder::parseModulePassPipeline(ModulePassManager &MPM,
                                          ArrayRef<PipelineElement> Pipeline,
                                          bool VerifyEachPass,
                                          bool DebugLogging) {
  for (const auto &Element : Pipeline) {
    if (!parseModulePass(MPM, Element, VerifyEachPass, DebugLogging))
      return false;
    if (VerifyEachPass)
      MPM.addPass(VerifierPass());
  }
  return true;
}

// Primary pass pipeline description parsing routine.
// FIXME: Should this routine accept a TargetMachine or require the caller to
// pre-populate the analysis managers with target-specific stuff?
bool PassBuilder::parsePassPipeline(ModulePassManager &MPM,
                                    StringRef PipelineText, bool VerifyEachPass,
                                    bool DebugLogging) {
  auto Pipeline = parsePipelineText(PipelineText);
  if (!Pipeline || Pipeline->empty())
    return false;

  // If the first name isn't at the module layer, wrap the pipeline up
  // automatically.
  StringRef FirstName = Pipeline->front().Name;

  if (!isModulePassName(FirstName)) {
    if (isCGSCCPassName(FirstName))
      Pipeline = {{"cgscc", std::move(*Pipeline)}};
    else if (isFunctionPassName(FirstName))
      Pipeline = {{"function", std::move(*Pipeline)}};
    else if (isLoopPassName(FirstName))
      Pipeline = {{"function", {{"loop", std::move(*Pipeline)}}}};
    else
      // Unknown pass name!
      return false;
  }

  return parseModulePassPipeline(MPM, *Pipeline, VerifyEachPass, DebugLogging);
}

bool PassBuilder::parseAAPipeline(AAManager &AA, StringRef PipelineText) {
  // If the pipeline just consists of the word 'default' just replace the AA
  // manager with our default one.
  if (PipelineText == "default") {
    AA = buildDefaultAAPipeline();
    return true;
  }

  while (!PipelineText.empty()) {
    StringRef Name;
    std::tie(Name, PipelineText) = PipelineText.split(',');
    if (!parseAAPassName(AA, Name))
      return false;
  }

  return true;
}
