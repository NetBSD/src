//===-- LoopUnroll.cpp - Loop unroller pass -------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass implements a simple loop unroller.  It works best when loops have
// been canonicalized by the -indvars pass, allowing it to determine the trip
// counts of loops easily.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "loop-unroll"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Analysis/CodeMetrics.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/UnrollLoop.h"
#include <climits>

using namespace llvm;

static cl::opt<unsigned>
UnrollThreshold("unroll-threshold", cl::init(150), cl::Hidden,
  cl::desc("The cut-off point for automatic loop unrolling"));

static cl::opt<unsigned>
UnrollCount("unroll-count", cl::init(0), cl::Hidden,
  cl::desc("Use this unroll count for all loops, for testing purposes"));

static cl::opt<bool>
UnrollAllowPartial("unroll-allow-partial", cl::init(false), cl::Hidden,
  cl::desc("Allows loops to be partially unrolled until "
           "-unroll-threshold loop size is reached."));

static cl::opt<bool>
UnrollRuntime("unroll-runtime", cl::ZeroOrMore, cl::init(false), cl::Hidden,
  cl::desc("Unroll loops with run-time trip counts"));

namespace {
  class LoopUnroll : public LoopPass {
  public:
    static char ID; // Pass ID, replacement for typeid
    LoopUnroll(int T = -1, int C = -1, int P = -1, int R = -1) : LoopPass(ID) {
      CurrentThreshold = (T == -1) ? UnrollThreshold : unsigned(T);
      CurrentCount = (C == -1) ? UnrollCount : unsigned(C);
      CurrentAllowPartial = (P == -1) ? UnrollAllowPartial : (bool)P;
      CurrentRuntime = (R == -1) ? UnrollRuntime : (bool)R;

      UserThreshold = (T != -1) || (UnrollThreshold.getNumOccurrences() > 0);
      UserAllowPartial = (P != -1) ||
                         (UnrollAllowPartial.getNumOccurrences() > 0);
      UserRuntime = (R != -1) || (UnrollRuntime.getNumOccurrences() > 0);
      UserCount = (C != -1) || (UnrollCount.getNumOccurrences() > 0);

      initializeLoopUnrollPass(*PassRegistry::getPassRegistry());
    }

    /// A magic value for use with the Threshold parameter to indicate
    /// that the loop unroll should be performed regardless of how much
    /// code expansion would result.
    static const unsigned NoThreshold = UINT_MAX;

    // Threshold to use when optsize is specified (and there is no
    // explicit -unroll-threshold).
    static const unsigned OptSizeUnrollThreshold = 50;

    // Default unroll count for loops with run-time trip count if
    // -unroll-count is not set
    static const unsigned UnrollRuntimeCount = 8;

    unsigned CurrentCount;
    unsigned CurrentThreshold;
    bool     CurrentAllowPartial;
    bool     CurrentRuntime;
    bool     UserCount;            // CurrentCount is user-specified.
    bool     UserThreshold;        // CurrentThreshold is user-specified.
    bool     UserAllowPartial;     // CurrentAllowPartial is user-specified.
    bool     UserRuntime;          // CurrentRuntime is user-specified.

    bool runOnLoop(Loop *L, LPPassManager &LPM);

    /// This transformation requires natural loop information & requires that
    /// loop preheaders be inserted into the CFG...
    ///
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<LoopInfo>();
      AU.addPreserved<LoopInfo>();
      AU.addRequiredID(LoopSimplifyID);
      AU.addPreservedID(LoopSimplifyID);
      AU.addRequiredID(LCSSAID);
      AU.addPreservedID(LCSSAID);
      AU.addRequired<ScalarEvolution>();
      AU.addPreserved<ScalarEvolution>();
      AU.addRequired<TargetTransformInfo>();
      // FIXME: Loop unroll requires LCSSA. And LCSSA requires dom info.
      // If loop unroll does not preserve dom info then LCSSA pass on next
      // loop will receive invalid dom info.
      // For now, recreate dom info, if loop is unrolled.
      AU.addPreserved<DominatorTreeWrapperPass>();
    }
  };
}

char LoopUnroll::ID = 0;
INITIALIZE_PASS_BEGIN(LoopUnroll, "loop-unroll", "Unroll loops", false, false)
INITIALIZE_AG_DEPENDENCY(TargetTransformInfo)
INITIALIZE_PASS_DEPENDENCY(LoopInfo)
INITIALIZE_PASS_DEPENDENCY(LoopSimplify)
INITIALIZE_PASS_DEPENDENCY(LCSSA)
INITIALIZE_PASS_DEPENDENCY(ScalarEvolution)
INITIALIZE_PASS_END(LoopUnroll, "loop-unroll", "Unroll loops", false, false)

Pass *llvm::createLoopUnrollPass(int Threshold, int Count, int AllowPartial,
                                 int Runtime) {
  return new LoopUnroll(Threshold, Count, AllowPartial, Runtime);
}

/// ApproximateLoopSize - Approximate the size of the loop.
static unsigned ApproximateLoopSize(const Loop *L, unsigned &NumCalls,
                                    bool &NotDuplicatable,
                                    const TargetTransformInfo &TTI) {
  CodeMetrics Metrics;
  for (Loop::block_iterator I = L->block_begin(), E = L->block_end();
       I != E; ++I)
    Metrics.analyzeBasicBlock(*I, TTI);
  NumCalls = Metrics.NumInlineCandidates;
  NotDuplicatable = Metrics.notDuplicatable;

  unsigned LoopSize = Metrics.NumInsts;

  // Don't allow an estimate of size zero.  This would allows unrolling of loops
  // with huge iteration counts, which is a compile time problem even if it's
  // not a problem for code quality.
  if (LoopSize == 0) LoopSize = 1;

  return LoopSize;
}

bool LoopUnroll::runOnLoop(Loop *L, LPPassManager &LPM) {
  LoopInfo *LI = &getAnalysis<LoopInfo>();
  ScalarEvolution *SE = &getAnalysis<ScalarEvolution>();
  const TargetTransformInfo &TTI = getAnalysis<TargetTransformInfo>();

  BasicBlock *Header = L->getHeader();
  DEBUG(dbgs() << "Loop Unroll: F[" << Header->getParent()->getName()
        << "] Loop %" << Header->getName() << "\n");
  (void)Header;

  TargetTransformInfo::UnrollingPreferences UP;
  UP.Threshold = CurrentThreshold;
  UP.OptSizeThreshold = OptSizeUnrollThreshold;
  UP.Count = CurrentCount;
  UP.Partial = CurrentAllowPartial;
  UP.Runtime = CurrentRuntime;
  TTI.getUnrollingPreferences(L, UP);

  // Determine the current unrolling threshold.  While this is normally set
  // from UnrollThreshold, it is overridden to a smaller value if the current
  // function is marked as optimize-for-size, and the unroll threshold was
  // not user specified.
  unsigned Threshold = UserThreshold ? CurrentThreshold : UP.Threshold;
  if (!UserThreshold &&
      Header->getParent()->getAttributes().
        hasAttribute(AttributeSet::FunctionIndex,
                     Attribute::OptimizeForSize))
    Threshold = UP.OptSizeThreshold;

  // Find trip count and trip multiple if count is not available
  unsigned TripCount = 0;
  unsigned TripMultiple = 1;
  // Find "latch trip count". UnrollLoop assumes that control cannot exit
  // via the loop latch on any iteration prior to TripCount. The loop may exit
  // early via an earlier branch.
  BasicBlock *LatchBlock = L->getLoopLatch();
  if (LatchBlock) {
    TripCount = SE->getSmallConstantTripCount(L, LatchBlock);
    TripMultiple = SE->getSmallConstantTripMultiple(L, LatchBlock);
  }

  bool Runtime = UserRuntime ? CurrentRuntime : UP.Runtime;

  // Use a default unroll-count if the user doesn't specify a value
  // and the trip count is a run-time value.  The default is different
  // for run-time or compile-time trip count loops.
  unsigned Count = UserCount ? CurrentCount : UP.Count;
  if (Runtime && Count == 0 && TripCount == 0)
    Count = UnrollRuntimeCount;

  if (Count == 0) {
    // Conservative heuristic: if we know the trip count, see if we can
    // completely unroll (subject to the threshold, checked below); otherwise
    // try to find greatest modulo of the trip count which is still under
    // threshold value.
    if (TripCount == 0)
      return false;
    Count = TripCount;
  }

  // Enforce the threshold.
  if (Threshold != NoThreshold) {
    unsigned NumInlineCandidates;
    bool notDuplicatable;
    unsigned LoopSize = ApproximateLoopSize(L, NumInlineCandidates,
                                            notDuplicatable, TTI);
    DEBUG(dbgs() << "  Loop Size = " << LoopSize << "\n");
    if (notDuplicatable) {
      DEBUG(dbgs() << "  Not unrolling loop which contains non-duplicatable"
            << " instructions.\n");
      return false;
    }
    if (NumInlineCandidates != 0) {
      DEBUG(dbgs() << "  Not unrolling loop with inlinable calls.\n");
      return false;
    }
    uint64_t Size = (uint64_t)LoopSize*Count;
    if (TripCount != 1 && Size > Threshold) {
      DEBUG(dbgs() << "  Too large to fully unroll with count: " << Count
            << " because size: " << Size << ">" << Threshold << "\n");
      bool AllowPartial = UserAllowPartial ? CurrentAllowPartial : UP.Partial;
      if (!AllowPartial && !(Runtime && TripCount == 0)) {
        DEBUG(dbgs() << "  will not try to unroll partially because "
              << "-unroll-allow-partial not given\n");
        return false;
      }
      if (TripCount) {
        // Reduce unroll count to be modulo of TripCount for partial unrolling
        Count = Threshold / LoopSize;
        while (Count != 0 && TripCount%Count != 0)
          Count--;
      }
      else if (Runtime) {
        // Reduce unroll count to be a lower power-of-two value
        while (Count != 0 && Size > Threshold) {
          Count >>= 1;
          Size = LoopSize*Count;
        }
      }
      if (Count < 2) {
        DEBUG(dbgs() << "  could not unroll partially\n");
        return false;
      }
      DEBUG(dbgs() << "  partially unrolling with count: " << Count << "\n");
    }
  }

  // Unroll the loop.
  if (!UnrollLoop(L, Count, TripCount, Runtime, TripMultiple, LI, &LPM))
    return false;

  return true;
}
