//===- FunctionAttrs.cpp - Pass which marks functions attributes ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file implements interprocedural passes which walk the
/// call-graph deducing and/or propagating function attributes.
///
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Analysis/CaptureTracking.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
using namespace llvm;

#define DEBUG_TYPE "functionattrs"

STATISTIC(NumReadNone, "Number of functions marked readnone");
STATISTIC(NumReadOnly, "Number of functions marked readonly");
STATISTIC(NumNoCapture, "Number of arguments marked nocapture");
STATISTIC(NumReadNoneArg, "Number of arguments marked readnone");
STATISTIC(NumReadOnlyArg, "Number of arguments marked readonly");
STATISTIC(NumNoAlias, "Number of function returns marked noalias");
STATISTIC(NumNonNullReturn, "Number of function returns marked nonnull");
STATISTIC(NumNoRecurse, "Number of functions marked as norecurse");

namespace {
typedef SmallSetVector<Function *, 8> SCCNodeSet;
}

namespace {
struct PostOrderFunctionAttrs : public CallGraphSCCPass {
  static char ID; // Pass identification, replacement for typeid
  PostOrderFunctionAttrs() : CallGraphSCCPass(ID) {
    initializePostOrderFunctionAttrsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnSCC(CallGraphSCC &SCC) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<AssumptionCacheTracker>();
    AU.addRequired<TargetLibraryInfoWrapperPass>();
    CallGraphSCCPass::getAnalysisUsage(AU);
  }

private:
  TargetLibraryInfo *TLI;
};
}

char PostOrderFunctionAttrs::ID = 0;
INITIALIZE_PASS_BEGIN(PostOrderFunctionAttrs, "functionattrs",
                      "Deduce function attributes", false, false)
INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
INITIALIZE_PASS_END(PostOrderFunctionAttrs, "functionattrs",
                    "Deduce function attributes", false, false)

Pass *llvm::createPostOrderFunctionAttrsPass() { return new PostOrderFunctionAttrs(); }

namespace {
/// The three kinds of memory access relevant to 'readonly' and
/// 'readnone' attributes.
enum MemoryAccessKind {
  MAK_ReadNone = 0,
  MAK_ReadOnly = 1,
  MAK_MayWrite = 2
};
}

static MemoryAccessKind checkFunctionMemoryAccess(Function &F, AAResults &AAR,
                                                  const SCCNodeSet &SCCNodes) {
  FunctionModRefBehavior MRB = AAR.getModRefBehavior(&F);
  if (MRB == FMRB_DoesNotAccessMemory)
    // Already perfect!
    return MAK_ReadNone;

  // Definitions with weak linkage may be overridden at linktime with
  // something that writes memory, so treat them like declarations.
  if (F.isDeclaration() || F.mayBeOverridden()) {
    if (AliasAnalysis::onlyReadsMemory(MRB))
      return MAK_ReadOnly;

    // Conservatively assume it writes to memory.
    return MAK_MayWrite;
  }

  // Scan the function body for instructions that may read or write memory.
  bool ReadsMemory = false;
  for (inst_iterator II = inst_begin(F), E = inst_end(F); II != E; ++II) {
    Instruction *I = &*II;

    // Some instructions can be ignored even if they read or write memory.
    // Detect these now, skipping to the next instruction if one is found.
    CallSite CS(cast<Value>(I));
    if (CS) {
      // Ignore calls to functions in the same SCC.
      if (CS.getCalledFunction() && SCCNodes.count(CS.getCalledFunction()))
        continue;
      FunctionModRefBehavior MRB = AAR.getModRefBehavior(CS);

      // If the call doesn't access memory, we're done.
      if (!(MRB & MRI_ModRef))
        continue;

      if (!AliasAnalysis::onlyAccessesArgPointees(MRB)) {
        // The call could access any memory. If that includes writes, give up.
        if (MRB & MRI_Mod)
          return MAK_MayWrite;
        // If it reads, note it.
        if (MRB & MRI_Ref)
          ReadsMemory = true;
        continue;
      }

      // Check whether all pointer arguments point to local memory, and
      // ignore calls that only access local memory.
      for (CallSite::arg_iterator CI = CS.arg_begin(), CE = CS.arg_end();
           CI != CE; ++CI) {
        Value *Arg = *CI;
        if (!Arg->getType()->isPtrOrPtrVectorTy())
          continue;

        AAMDNodes AAInfo;
        I->getAAMetadata(AAInfo);
        MemoryLocation Loc(Arg, MemoryLocation::UnknownSize, AAInfo);

        // Skip accesses to local or constant memory as they don't impact the
        // externally visible mod/ref behavior.
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;

        if (MRB & MRI_Mod)
          // Writes non-local memory.  Give up.
          return MAK_MayWrite;
        if (MRB & MRI_Ref)
          // Ok, it reads non-local memory.
          ReadsMemory = true;
      }
      continue;
    } else if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      // Ignore non-volatile loads from local memory. (Atomic is okay here.)
      if (!LI->isVolatile()) {
        MemoryLocation Loc = MemoryLocation::get(LI);
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;
      }
    } else if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
      // Ignore non-volatile stores to local memory. (Atomic is okay here.)
      if (!SI->isVolatile()) {
        MemoryLocation Loc = MemoryLocation::get(SI);
        if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
          continue;
      }
    } else if (VAArgInst *VI = dyn_cast<VAArgInst>(I)) {
      // Ignore vaargs on local memory.
      MemoryLocation Loc = MemoryLocation::get(VI);
      if (AAR.pointsToConstantMemory(Loc, /*OrLocal=*/true))
        continue;
    }

    // Any remaining instructions need to be taken seriously!  Check if they
    // read or write memory.
    if (I->mayWriteToMemory())
      // Writes memory.  Just give up.
      return MAK_MayWrite;

    // If this instruction may read memory, remember that.
    ReadsMemory |= I->mayReadFromMemory();
  }

  return ReadsMemory ? MAK_ReadOnly : MAK_ReadNone;
}

/// Deduce readonly/readnone attributes for the SCC.
template <typename AARGetterT>
static bool addReadAttrs(const SCCNodeSet &SCCNodes, AARGetterT AARGetter) {
  // Check if any of the functions in the SCC read or write memory.  If they
  // write memory then they can't be marked readnone or readonly.
  bool ReadsMemory = false;
  for (Function *F : SCCNodes) {
    // Call the callable parameter to look up AA results for this function.
    AAResults &AAR = AARGetter(*F);

    switch (checkFunctionMemoryAccess(*F, AAR, SCCNodes)) {
    case MAK_MayWrite:
      return false;
    case MAK_ReadOnly:
      ReadsMemory = true;
      break;
    case MAK_ReadNone:
      // Nothing to do!
      break;
    }
  }

  // Success!  Functions in this SCC do not access memory, or only read memory.
  // Give them the appropriate attribute.
  bool MadeChange = false;
  for (Function *F : SCCNodes) {
    if (F->doesNotAccessMemory())
      // Already perfect!
      continue;

    if (F->onlyReadsMemory() && ReadsMemory)
      // No change.
      continue;

    MadeChange = true;

    // Clear out any existing attributes.
    AttrBuilder B;
    B.addAttribute(Attribute::ReadOnly).addAttribute(Attribute::ReadNone);
    F->removeAttributes(
        AttributeSet::FunctionIndex,
        AttributeSet::get(F->getContext(), AttributeSet::FunctionIndex, B));

    // Add in the new attribute.
    F->addAttribute(AttributeSet::FunctionIndex,
                    ReadsMemory ? Attribute::ReadOnly : Attribute::ReadNone);

    if (ReadsMemory)
      ++NumReadOnly;
    else
      ++NumReadNone;
  }

  return MadeChange;
}

namespace {
/// For a given pointer Argument, this retains a list of Arguments of functions
/// in the same SCC that the pointer data flows into. We use this to build an
/// SCC of the arguments.
struct ArgumentGraphNode {
  Argument *Definition;
  SmallVector<ArgumentGraphNode *, 4> Uses;
};

class ArgumentGraph {
  // We store pointers to ArgumentGraphNode objects, so it's important that
  // that they not move around upon insert.
  typedef std::map<Argument *, ArgumentGraphNode> ArgumentMapTy;

  ArgumentMapTy ArgumentMap;

  // There is no root node for the argument graph, in fact:
  //   void f(int *x, int *y) { if (...) f(x, y); }
  // is an example where the graph is disconnected. The SCCIterator requires a
  // single entry point, so we maintain a fake ("synthetic") root node that
  // uses every node. Because the graph is directed and nothing points into
  // the root, it will not participate in any SCCs (except for its own).
  ArgumentGraphNode SyntheticRoot;

public:
  ArgumentGraph() { SyntheticRoot.Definition = nullptr; }

  typedef SmallVectorImpl<ArgumentGraphNode *>::iterator iterator;

  iterator begin() { return SyntheticRoot.Uses.begin(); }
  iterator end() { return SyntheticRoot.Uses.end(); }
  ArgumentGraphNode *getEntryNode() { return &SyntheticRoot; }

  ArgumentGraphNode *operator[](Argument *A) {
    ArgumentGraphNode &Node = ArgumentMap[A];
    Node.Definition = A;
    SyntheticRoot.Uses.push_back(&Node);
    return &Node;
  }
};

/// This tracker checks whether callees are in the SCC, and if so it does not
/// consider that a capture, instead adding it to the "Uses" list and
/// continuing with the analysis.
struct ArgumentUsesTracker : public CaptureTracker {
  ArgumentUsesTracker(const SCCNodeSet &SCCNodes)
      : Captured(false), SCCNodes(SCCNodes) {}

  void tooManyUses() override { Captured = true; }

  bool captured(const Use *U) override {
    CallSite CS(U->getUser());
    if (!CS.getInstruction()) {
      Captured = true;
      return true;
    }

    Function *F = CS.getCalledFunction();
    if (!F || F->isDeclaration() || F->mayBeOverridden() ||
        !SCCNodes.count(F)) {
      Captured = true;
      return true;
    }

    // Note: the callee and the two successor blocks *follow* the argument
    // operands.  This means there is no need to adjust UseIndex to account for
    // these.

    unsigned UseIndex =
        std::distance(const_cast<const Use *>(CS.arg_begin()), U);

    assert(UseIndex < CS.data_operands_size() &&
           "Indirect function calls should have been filtered above!");

    if (UseIndex >= CS.getNumArgOperands()) {
      // Data operand, but not a argument operand -- must be a bundle operand
      assert(CS.hasOperandBundles() && "Must be!");

      // CaptureTracking told us that we're being captured by an operand bundle
      // use.  In this case it does not matter if the callee is within our SCC
      // or not -- we've been captured in some unknown way, and we have to be
      // conservative.
      Captured = true;
      return true;
    }

    if (UseIndex >= F->arg_size()) {
      assert(F->isVarArg() && "More params than args in non-varargs call");
      Captured = true;
      return true;
    }

    Uses.push_back(&*std::next(F->arg_begin(), UseIndex));
    return false;
  }

  bool Captured; // True only if certainly captured (used outside our SCC).
  SmallVector<Argument *, 4> Uses; // Uses within our SCC.

  const SCCNodeSet &SCCNodes;
};
}

namespace llvm {
template <> struct GraphTraits<ArgumentGraphNode *> {
  typedef ArgumentGraphNode NodeType;
  typedef SmallVectorImpl<ArgumentGraphNode *>::iterator ChildIteratorType;

  static inline NodeType *getEntryNode(NodeType *A) { return A; }
  static inline ChildIteratorType child_begin(NodeType *N) {
    return N->Uses.begin();
  }
  static inline ChildIteratorType child_end(NodeType *N) {
    return N->Uses.end();
  }
};
template <>
struct GraphTraits<ArgumentGraph *> : public GraphTraits<ArgumentGraphNode *> {
  static NodeType *getEntryNode(ArgumentGraph *AG) {
    return AG->getEntryNode();
  }
  static ChildIteratorType nodes_begin(ArgumentGraph *AG) {
    return AG->begin();
  }
  static ChildIteratorType nodes_end(ArgumentGraph *AG) { return AG->end(); }
};
}

/// Returns Attribute::None, Attribute::ReadOnly or Attribute::ReadNone.
static Attribute::AttrKind
determinePointerReadAttrs(Argument *A,
                          const SmallPtrSet<Argument *, 8> &SCCNodes) {

  SmallVector<Use *, 32> Worklist;
  SmallSet<Use *, 32> Visited;

  // inalloca arguments are always clobbered by the call.
  if (A->hasInAllocaAttr())
    return Attribute::None;

  bool IsRead = false;
  // We don't need to track IsWritten. If A is written to, return immediately.

  for (Use &U : A->uses()) {
    Visited.insert(&U);
    Worklist.push_back(&U);
  }

  while (!Worklist.empty()) {
    Use *U = Worklist.pop_back_val();
    Instruction *I = cast<Instruction>(U->getUser());

    switch (I->getOpcode()) {
    case Instruction::BitCast:
    case Instruction::GetElementPtr:
    case Instruction::PHI:
    case Instruction::Select:
    case Instruction::AddrSpaceCast:
      // The original value is not read/written via this if the new value isn't.
      for (Use &UU : I->uses())
        if (Visited.insert(&UU).second)
          Worklist.push_back(&UU);
      break;

    case Instruction::Call:
    case Instruction::Invoke: {
      bool Captures = true;

      if (I->getType()->isVoidTy())
        Captures = false;

      auto AddUsersToWorklistIfCapturing = [&] {
        if (Captures)
          for (Use &UU : I->uses())
            if (Visited.insert(&UU).second)
              Worklist.push_back(&UU);
      };

      CallSite CS(I);
      if (CS.doesNotAccessMemory()) {
        AddUsersToWorklistIfCapturing();
        continue;
      }

      Function *F = CS.getCalledFunction();
      if (!F) {
        if (CS.onlyReadsMemory()) {
          IsRead = true;
          AddUsersToWorklistIfCapturing();
          continue;
        }
        return Attribute::None;
      }

      // Note: the callee and the two successor blocks *follow* the argument
      // operands.  This means there is no need to adjust UseIndex to account
      // for these.

      unsigned UseIndex = std::distance(CS.arg_begin(), U);

      // U cannot be the callee operand use: since we're exploring the
      // transitive uses of an Argument, having such a use be a callee would
      // imply the CallSite is an indirect call or invoke; and we'd take the
      // early exit above.
      assert(UseIndex < CS.data_operands_size() &&
             "Data operand use expected!");

      bool IsOperandBundleUse = UseIndex >= CS.getNumArgOperands();

      if (UseIndex >= F->arg_size() && !IsOperandBundleUse) {
        assert(F->isVarArg() && "More params than args in non-varargs call");
        return Attribute::None;
      }

      Captures &= !CS.doesNotCapture(UseIndex);

      // Since the optimizer (by design) cannot see the data flow corresponding
      // to a operand bundle use, these cannot participate in the optimistic SCC
      // analysis.  Instead, we model the operand bundle uses as arguments in
      // call to a function external to the SCC.
      if (!SCCNodes.count(&*std::next(F->arg_begin(), UseIndex)) ||
          IsOperandBundleUse) {

        // The accessors used on CallSite here do the right thing for calls and
        // invokes with operand bundles.

        if (!CS.onlyReadsMemory() && !CS.onlyReadsMemory(UseIndex))
          return Attribute::None;
        if (!CS.doesNotAccessMemory(UseIndex))
          IsRead = true;
      }

      AddUsersToWorklistIfCapturing();
      break;
    }

    case Instruction::Load:
      IsRead = true;
      break;

    case Instruction::ICmp:
    case Instruction::Ret:
      break;

    default:
      return Attribute::None;
    }
  }

  return IsRead ? Attribute::ReadOnly : Attribute::ReadNone;
}

/// Deduce nocapture attributes for the SCC.
static bool addArgumentAttrs(const SCCNodeSet &SCCNodes) {
  bool Changed = false;

  ArgumentGraph AG;

  AttrBuilder B;
  B.addAttribute(Attribute::NoCapture);

  // Check each function in turn, determining which pointer arguments are not
  // captured.
  for (Function *F : SCCNodes) {
    // Definitions with weak linkage may be overridden at linktime with
    // something that captures pointers, so treat them like declarations.
    if (F->isDeclaration() || F->mayBeOverridden())
      continue;

    // Functions that are readonly (or readnone) and nounwind and don't return
    // a value can't capture arguments. Don't analyze them.
    if (F->onlyReadsMemory() && F->doesNotThrow() &&
        F->getReturnType()->isVoidTy()) {
      for (Function::arg_iterator A = F->arg_begin(), E = F->arg_end(); A != E;
           ++A) {
        if (A->getType()->isPointerTy() && !A->hasNoCaptureAttr()) {
          A->addAttr(AttributeSet::get(F->getContext(), A->getArgNo() + 1, B));
          ++NumNoCapture;
          Changed = true;
        }
      }
      continue;
    }

    for (Function::arg_iterator A = F->arg_begin(), E = F->arg_end(); A != E;
         ++A) {
      if (!A->getType()->isPointerTy())
        continue;
      bool HasNonLocalUses = false;
      if (!A->hasNoCaptureAttr()) {
        ArgumentUsesTracker Tracker(SCCNodes);
        PointerMayBeCaptured(&*A, &Tracker);
        if (!Tracker.Captured) {
          if (Tracker.Uses.empty()) {
            // If it's trivially not captured, mark it nocapture now.
            A->addAttr(
                AttributeSet::get(F->getContext(), A->getArgNo() + 1, B));
            ++NumNoCapture;
            Changed = true;
          } else {
            // If it's not trivially captured and not trivially not captured,
            // then it must be calling into another function in our SCC. Save
            // its particulars for Argument-SCC analysis later.
            ArgumentGraphNode *Node = AG[&*A];
            for (SmallVectorImpl<Argument *>::iterator
                     UI = Tracker.Uses.begin(),
                     UE = Tracker.Uses.end();
                 UI != UE; ++UI) {
              Node->Uses.push_back(AG[*UI]);
              if (*UI != A)
                HasNonLocalUses = true;
            }
          }
        }
        // Otherwise, it's captured. Don't bother doing SCC analysis on it.
      }
      if (!HasNonLocalUses && !A->onlyReadsMemory()) {
        // Can we determine that it's readonly/readnone without doing an SCC?
        // Note that we don't allow any calls at all here, or else our result
        // will be dependent on the iteration order through the functions in the
        // SCC.
        SmallPtrSet<Argument *, 8> Self;
        Self.insert(&*A);
        Attribute::AttrKind R = determinePointerReadAttrs(&*A, Self);
        if (R != Attribute::None) {
          AttrBuilder B;
          B.addAttribute(R);
          A->addAttr(AttributeSet::get(A->getContext(), A->getArgNo() + 1, B));
          Changed = true;
          R == Attribute::ReadOnly ? ++NumReadOnlyArg : ++NumReadNoneArg;
        }
      }
    }
  }

  // The graph we've collected is partial because we stopped scanning for
  // argument uses once we solved the argument trivially. These partial nodes
  // show up as ArgumentGraphNode objects with an empty Uses list, and for
  // these nodes the final decision about whether they capture has already been
  // made.  If the definition doesn't have a 'nocapture' attribute by now, it
  // captures.

  for (scc_iterator<ArgumentGraph *> I = scc_begin(&AG); !I.isAtEnd(); ++I) {
    const std::vector<ArgumentGraphNode *> &ArgumentSCC = *I;
    if (ArgumentSCC.size() == 1) {
      if (!ArgumentSCC[0]->Definition)
        continue; // synthetic root node

      // eg. "void f(int* x) { if (...) f(x); }"
      if (ArgumentSCC[0]->Uses.size() == 1 &&
          ArgumentSCC[0]->Uses[0] == ArgumentSCC[0]) {
        Argument *A = ArgumentSCC[0]->Definition;
        A->addAttr(AttributeSet::get(A->getContext(), A->getArgNo() + 1, B));
        ++NumNoCapture;
        Changed = true;
      }
      continue;
    }

    bool SCCCaptured = false;
    for (auto I = ArgumentSCC.begin(), E = ArgumentSCC.end();
         I != E && !SCCCaptured; ++I) {
      ArgumentGraphNode *Node = *I;
      if (Node->Uses.empty()) {
        if (!Node->Definition->hasNoCaptureAttr())
          SCCCaptured = true;
      }
    }
    if (SCCCaptured)
      continue;

    SmallPtrSet<Argument *, 8> ArgumentSCCNodes;
    // Fill ArgumentSCCNodes with the elements of the ArgumentSCC.  Used for
    // quickly looking up whether a given Argument is in this ArgumentSCC.
    for (auto I = ArgumentSCC.begin(), E = ArgumentSCC.end(); I != E; ++I) {
      ArgumentSCCNodes.insert((*I)->Definition);
    }

    for (auto I = ArgumentSCC.begin(), E = ArgumentSCC.end();
         I != E && !SCCCaptured; ++I) {
      ArgumentGraphNode *N = *I;
      for (SmallVectorImpl<ArgumentGraphNode *>::iterator UI = N->Uses.begin(),
                                                          UE = N->Uses.end();
           UI != UE; ++UI) {
        Argument *A = (*UI)->Definition;
        if (A->hasNoCaptureAttr() || ArgumentSCCNodes.count(A))
          continue;
        SCCCaptured = true;
        break;
      }
    }
    if (SCCCaptured)
      continue;

    for (unsigned i = 0, e = ArgumentSCC.size(); i != e; ++i) {
      Argument *A = ArgumentSCC[i]->Definition;
      A->addAttr(AttributeSet::get(A->getContext(), A->getArgNo() + 1, B));
      ++NumNoCapture;
      Changed = true;
    }

    // We also want to compute readonly/readnone. With a small number of false
    // negatives, we can assume that any pointer which is captured isn't going
    // to be provably readonly or readnone, since by definition we can't
    // analyze all uses of a captured pointer.
    //
    // The false negatives happen when the pointer is captured by a function
    // that promises readonly/readnone behaviour on the pointer, then the
    // pointer's lifetime ends before anything that writes to arbitrary memory.
    // Also, a readonly/readnone pointer may be returned, but returning a
    // pointer is capturing it.

    Attribute::AttrKind ReadAttr = Attribute::ReadNone;
    for (unsigned i = 0, e = ArgumentSCC.size(); i != e; ++i) {
      Argument *A = ArgumentSCC[i]->Definition;
      Attribute::AttrKind K = determinePointerReadAttrs(A, ArgumentSCCNodes);
      if (K == Attribute::ReadNone)
        continue;
      if (K == Attribute::ReadOnly) {
        ReadAttr = Attribute::ReadOnly;
        continue;
      }
      ReadAttr = K;
      break;
    }

    if (ReadAttr != Attribute::None) {
      AttrBuilder B, R;
      B.addAttribute(ReadAttr);
      R.addAttribute(Attribute::ReadOnly).addAttribute(Attribute::ReadNone);
      for (unsigned i = 0, e = ArgumentSCC.size(); i != e; ++i) {
        Argument *A = ArgumentSCC[i]->Definition;
        // Clear out existing readonly/readnone attributes
        A->removeAttr(AttributeSet::get(A->getContext(), A->getArgNo() + 1, R));
        A->addAttr(AttributeSet::get(A->getContext(), A->getArgNo() + 1, B));
        ReadAttr == Attribute::ReadOnly ? ++NumReadOnlyArg : ++NumReadNoneArg;
        Changed = true;
      }
    }
  }

  return Changed;
}

/// Tests whether a function is "malloc-like".
///
/// A function is "malloc-like" if it returns either null or a pointer that
/// doesn't alias any other pointer visible to the caller.
static bool isFunctionMallocLike(Function *F, const SCCNodeSet &SCCNodes) {
  SmallSetVector<Value *, 8> FlowsToReturn;
  for (Function::iterator I = F->begin(), E = F->end(); I != E; ++I)
    if (ReturnInst *Ret = dyn_cast<ReturnInst>(I->getTerminator()))
      FlowsToReturn.insert(Ret->getReturnValue());

  for (unsigned i = 0; i != FlowsToReturn.size(); ++i) {
    Value *RetVal = FlowsToReturn[i];

    if (Constant *C = dyn_cast<Constant>(RetVal)) {
      if (!C->isNullValue() && !isa<UndefValue>(C))
        return false;

      continue;
    }

    if (isa<Argument>(RetVal))
      return false;

    if (Instruction *RVI = dyn_cast<Instruction>(RetVal))
      switch (RVI->getOpcode()) {
      // Extend the analysis by looking upwards.
      case Instruction::BitCast:
      case Instruction::GetElementPtr:
      case Instruction::AddrSpaceCast:
        FlowsToReturn.insert(RVI->getOperand(0));
        continue;
      case Instruction::Select: {
        SelectInst *SI = cast<SelectInst>(RVI);
        FlowsToReturn.insert(SI->getTrueValue());
        FlowsToReturn.insert(SI->getFalseValue());
        continue;
      }
      case Instruction::PHI: {
        PHINode *PN = cast<PHINode>(RVI);
        for (Value *IncValue : PN->incoming_values())
          FlowsToReturn.insert(IncValue);
        continue;
      }

      // Check whether the pointer came from an allocation.
      case Instruction::Alloca:
        break;
      case Instruction::Call:
      case Instruction::Invoke: {
        CallSite CS(RVI);
        if (CS.paramHasAttr(0, Attribute::NoAlias))
          break;
        if (CS.getCalledFunction() && SCCNodes.count(CS.getCalledFunction()))
          break;
      } // fall-through
      default:
        return false; // Did not come from an allocation.
      }

    if (PointerMayBeCaptured(RetVal, false, /*StoreCaptures=*/false))
      return false;
  }

  return true;
}

/// Deduce noalias attributes for the SCC.
static bool addNoAliasAttrs(const SCCNodeSet &SCCNodes) {
  // Check each function in turn, determining which functions return noalias
  // pointers.
  for (Function *F : SCCNodes) {
    // Already noalias.
    if (F->doesNotAlias(0))
      continue;

    // Definitions with weak linkage may be overridden at linktime, so
    // treat them like declarations.
    if (F->isDeclaration() || F->mayBeOverridden())
      return false;

    // We annotate noalias return values, which are only applicable to
    // pointer types.
    if (!F->getReturnType()->isPointerTy())
      continue;

    if (!isFunctionMallocLike(F, SCCNodes))
      return false;
  }

  bool MadeChange = false;
  for (Function *F : SCCNodes) {
    if (F->doesNotAlias(0) || !F->getReturnType()->isPointerTy())
      continue;

    F->setDoesNotAlias(0);
    ++NumNoAlias;
    MadeChange = true;
  }

  return MadeChange;
}

/// Tests whether this function is known to not return null.
///
/// Requires that the function returns a pointer.
///
/// Returns true if it believes the function will not return a null, and sets
/// \p Speculative based on whether the returned conclusion is a speculative
/// conclusion due to SCC calls.
static bool isReturnNonNull(Function *F, const SCCNodeSet &SCCNodes,
                            const TargetLibraryInfo &TLI, bool &Speculative) {
  assert(F->getReturnType()->isPointerTy() &&
         "nonnull only meaningful on pointer types");
  Speculative = false;

  SmallSetVector<Value *, 8> FlowsToReturn;
  for (BasicBlock &BB : *F)
    if (auto *Ret = dyn_cast<ReturnInst>(BB.getTerminator()))
      FlowsToReturn.insert(Ret->getReturnValue());

  for (unsigned i = 0; i != FlowsToReturn.size(); ++i) {
    Value *RetVal = FlowsToReturn[i];

    // If this value is locally known to be non-null, we're good
    if (isKnownNonNull(RetVal, &TLI))
      continue;

    // Otherwise, we need to look upwards since we can't make any local
    // conclusions.
    Instruction *RVI = dyn_cast<Instruction>(RetVal);
    if (!RVI)
      return false;
    switch (RVI->getOpcode()) {
    // Extend the analysis by looking upwards.
    case Instruction::BitCast:
    case Instruction::GetElementPtr:
    case Instruction::AddrSpaceCast:
      FlowsToReturn.insert(RVI->getOperand(0));
      continue;
    case Instruction::Select: {
      SelectInst *SI = cast<SelectInst>(RVI);
      FlowsToReturn.insert(SI->getTrueValue());
      FlowsToReturn.insert(SI->getFalseValue());
      continue;
    }
    case Instruction::PHI: {
      PHINode *PN = cast<PHINode>(RVI);
      for (int i = 0, e = PN->getNumIncomingValues(); i != e; ++i)
        FlowsToReturn.insert(PN->getIncomingValue(i));
      continue;
    }
    case Instruction::Call:
    case Instruction::Invoke: {
      CallSite CS(RVI);
      Function *Callee = CS.getCalledFunction();
      // A call to a node within the SCC is assumed to return null until
      // proven otherwise
      if (Callee && SCCNodes.count(Callee)) {
        Speculative = true;
        continue;
      }
      return false;
    }
    default:
      return false; // Unknown source, may be null
    };
    llvm_unreachable("should have either continued or returned");
  }

  return true;
}

/// Deduce nonnull attributes for the SCC.
static bool addNonNullAttrs(const SCCNodeSet &SCCNodes,
                            const TargetLibraryInfo &TLI) {
  // Speculative that all functions in the SCC return only nonnull
  // pointers.  We may refute this as we analyze functions.
  bool SCCReturnsNonNull = true;

  bool MadeChange = false;

  // Check each function in turn, determining which functions return nonnull
  // pointers.
  for (Function *F : SCCNodes) {
    // Already nonnull.
    if (F->getAttributes().hasAttribute(AttributeSet::ReturnIndex,
                                        Attribute::NonNull))
      continue;

    // Definitions with weak linkage may be overridden at linktime, so
    // treat them like declarations.
    if (F->isDeclaration() || F->mayBeOverridden())
      return false;

    // We annotate nonnull return values, which are only applicable to
    // pointer types.
    if (!F->getReturnType()->isPointerTy())
      continue;

    bool Speculative = false;
    if (isReturnNonNull(F, SCCNodes, TLI, Speculative)) {
      if (!Speculative) {
        // Mark the function eagerly since we may discover a function
        // which prevents us from speculating about the entire SCC
        DEBUG(dbgs() << "Eagerly marking " << F->getName() << " as nonnull\n");
        F->addAttribute(AttributeSet::ReturnIndex, Attribute::NonNull);
        ++NumNonNullReturn;
        MadeChange = true;
      }
      continue;
    }
    // At least one function returns something which could be null, can't
    // speculate any more.
    SCCReturnsNonNull = false;
  }

  if (SCCReturnsNonNull) {
    for (Function *F : SCCNodes) {
      if (F->getAttributes().hasAttribute(AttributeSet::ReturnIndex,
                                          Attribute::NonNull) ||
          !F->getReturnType()->isPointerTy())
        continue;

      DEBUG(dbgs() << "SCC marking " << F->getName() << " as nonnull\n");
      F->addAttribute(AttributeSet::ReturnIndex, Attribute::NonNull);
      ++NumNonNullReturn;
      MadeChange = true;
    }
  }

  return MadeChange;
}

static bool setDoesNotRecurse(Function &F) {
  if (F.doesNotRecurse())
    return false;
  F.setDoesNotRecurse();
  ++NumNoRecurse;
  return true;
}

static bool addNoRecurseAttrs(const CallGraphSCC &SCC) {
  // Try and identify functions that do not recurse.

  // If the SCC contains multiple nodes we know for sure there is recursion.
  if (!SCC.isSingular())
    return false;

  const CallGraphNode *CGN = *SCC.begin();
  Function *F = CGN->getFunction();
  if (!F || F->isDeclaration() || F->doesNotRecurse())
    return false;

  // If all of the calls in F are identifiable and are to norecurse functions, F
  // is norecurse. This check also detects self-recursion as F is not currently
  // marked norecurse, so any called from F to F will not be marked norecurse.
  if (std::all_of(CGN->begin(), CGN->end(),
                  [](const CallGraphNode::CallRecord &CR) {
                    Function *F = CR.second->getFunction();
                    return F && F->doesNotRecurse();
                  }))
    // Function calls a potentially recursive function.
    return setDoesNotRecurse(*F);

  // Nothing else we can deduce usefully during the postorder traversal.
  return false;
}

bool PostOrderFunctionAttrs::runOnSCC(CallGraphSCC &SCC) {
  TLI = &getAnalysis<TargetLibraryInfoWrapperPass>().getTLI();
  bool Changed = false;

  // We compute dedicated AA results for each function in the SCC as needed. We
  // use a lambda referencing external objects so that they live long enough to
  // be queried, but we re-use them each time.
  Optional<BasicAAResult> BAR;
  Optional<AAResults> AAR;
  auto AARGetter = [&](Function &F) -> AAResults & {
    BAR.emplace(createLegacyPMBasicAAResult(*this, F));
    AAR.emplace(createLegacyPMAAResults(*this, F, *BAR));
    return *AAR;
  };

  // Fill SCCNodes with the elements of the SCC. Used for quickly looking up
  // whether a given CallGraphNode is in this SCC. Also track whether there are
  // any external or opt-none nodes that will prevent us from optimizing any
  // part of the SCC.
  SCCNodeSet SCCNodes;
  bool ExternalNode = false;
  for (CallGraphSCC::iterator I = SCC.begin(), E = SCC.end(); I != E; ++I) {
    Function *F = (*I)->getFunction();
    if (!F || F->hasFnAttribute(Attribute::OptimizeNone)) {
      // External node or function we're trying not to optimize - we both avoid
      // transform them and avoid leveraging information they provide.
      ExternalNode = true;
      continue;
    }

    SCCNodes.insert(F);
  }

  Changed |= addReadAttrs(SCCNodes, AARGetter);
  Changed |= addArgumentAttrs(SCCNodes);

  // If we have no external nodes participating in the SCC, we can deduce some
  // more precise attributes as well.
  if (!ExternalNode) {
    Changed |= addNoAliasAttrs(SCCNodes);
    Changed |= addNonNullAttrs(SCCNodes, *TLI);
  }

  Changed |= addNoRecurseAttrs(SCC);
  return Changed;
}

namespace {
/// A pass to do RPO deduction and propagation of function attributes.
///
/// This pass provides a general RPO or "top down" propagation of
/// function attributes. For a few (rare) cases, we can deduce significantly
/// more about function attributes by working in RPO, so this pass
/// provides the compliment to the post-order pass above where the majority of
/// deduction is performed.
// FIXME: Currently there is no RPO CGSCC pass structure to slide into and so
// this is a boring module pass, but eventually it should be an RPO CGSCC pass
// when such infrastructure is available.
struct ReversePostOrderFunctionAttrs : public ModulePass {
  static char ID; // Pass identification, replacement for typeid
  ReversePostOrderFunctionAttrs() : ModulePass(ID) {
    initializeReversePostOrderFunctionAttrsPass(*PassRegistry::getPassRegistry());
  }

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<CallGraphWrapperPass>();
  }
};
}

char ReversePostOrderFunctionAttrs::ID = 0;
INITIALIZE_PASS_BEGIN(ReversePostOrderFunctionAttrs, "rpo-functionattrs",
                      "Deduce function attributes in RPO", false, false)
INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
INITIALIZE_PASS_END(ReversePostOrderFunctionAttrs, "rpo-functionattrs",
                    "Deduce function attributes in RPO", false, false)

Pass *llvm::createReversePostOrderFunctionAttrsPass() {
  return new ReversePostOrderFunctionAttrs();
}

static bool addNoRecurseAttrsTopDown(Function &F) {
  // We check the preconditions for the function prior to calling this to avoid
  // the cost of building up a reversible post-order list. We assert them here
  // to make sure none of the invariants this relies on were violated.
  assert(!F.isDeclaration() && "Cannot deduce norecurse without a definition!");
  assert(!F.doesNotRecurse() &&
         "This function has already been deduced as norecurs!");
  assert(F.hasInternalLinkage() &&
         "Can only do top-down deduction for internal linkage functions!");

  // If F is internal and all of its uses are calls from a non-recursive
  // functions, then none of its calls could in fact recurse without going
  // through a function marked norecurse, and so we can mark this function too
  // as norecurse. Note that the uses must actually be calls -- otherwise
  // a pointer to this function could be returned from a norecurse function but
  // this function could be recursively (indirectly) called. Note that this
  // also detects if F is directly recursive as F is not yet marked as
  // a norecurse function.
  for (auto *U : F.users()) {
    auto *I = dyn_cast<Instruction>(U);
    if (!I)
      return false;
    CallSite CS(I);
    if (!CS || !CS.getParent()->getParent()->doesNotRecurse())
      return false;
  }
  return setDoesNotRecurse(F);
}

bool ReversePostOrderFunctionAttrs::runOnModule(Module &M) {
  // We only have a post-order SCC traversal (because SCCs are inherently
  // discovered in post-order), so we accumulate them in a vector and then walk
  // it in reverse. This is simpler than using the RPO iterator infrastructure
  // because we need to combine SCC detection and the PO walk of the call
  // graph. We can also cheat egregiously because we're primarily interested in
  // synthesizing norecurse and so we can only save the singular SCCs as SCCs
  // with multiple functions in them will clearly be recursive.
  auto &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  SmallVector<Function *, 16> Worklist;
  for (scc_iterator<CallGraph *> I = scc_begin(&CG); !I.isAtEnd(); ++I) {
    if (I->size() != 1)
      continue;

    Function *F = I->front()->getFunction();
    if (F && !F->isDeclaration() && !F->doesNotRecurse() &&
        F->hasInternalLinkage())
      Worklist.push_back(F);
  }

  bool Changed = false;
  for (auto *F : reverse(Worklist))
    Changed |= addNoRecurseAttrsTopDown(*F);

  return Changed;
}
