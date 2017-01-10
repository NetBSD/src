//===-- GlobalDCE.cpp - DCE unreachable internal functions ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This transform is designed to eliminate unreachable internal globals from the
// program.  It uses an aggressive algorithm, searching out globals that are
// known to be alive.  After it finds all of the globals which are needed, it
// deletes whatever is left over.  This allows it to delete recursive chunks of
// the program which are unreachable.
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/IPO/GlobalDCE.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/CtorUtils.h"
#include "llvm/Transforms/Utils/GlobalStatus.h"
#include <unordered_map>
using namespace llvm;

#define DEBUG_TYPE "globaldce"

STATISTIC(NumAliases  , "Number of global aliases removed");
STATISTIC(NumFunctions, "Number of functions removed");
STATISTIC(NumIFuncs,    "Number of indirect functions removed");
STATISTIC(NumVariables, "Number of global variables removed");

namespace {
  class GlobalDCELegacyPass : public ModulePass {
  public:
    static char ID; // Pass identification, replacement for typeid
    GlobalDCELegacyPass() : ModulePass(ID) {
      initializeGlobalDCELegacyPassPass(*PassRegistry::getPassRegistry());
    }

    // run - Do the GlobalDCE pass on the specified module, optionally updating
    // the specified callgraph to reflect the changes.
    //
    bool runOnModule(Module &M) override {
      if (skipModule(M))
        return false;

      ModuleAnalysisManager DummyMAM;
      auto PA = Impl.run(M, DummyMAM);
      return !PA.areAllPreserved();
    }

  private:
    GlobalDCEPass Impl;
  };
}

char GlobalDCELegacyPass::ID = 0;
INITIALIZE_PASS(GlobalDCELegacyPass, "globaldce",
                "Dead Global Elimination", false, false)

// Public interface to the GlobalDCEPass.
ModulePass *llvm::createGlobalDCEPass() {
  return new GlobalDCELegacyPass();
}

/// Returns true if F contains only a single "ret" instruction.
static bool isEmptyFunction(Function *F) {
  BasicBlock &Entry = F->getEntryBlock();
  if (Entry.size() != 1 || !isa<ReturnInst>(Entry.front()))
    return false;
  ReturnInst &RI = cast<ReturnInst>(Entry.front());
  return RI.getReturnValue() == nullptr;
}

PreservedAnalyses GlobalDCEPass::run(Module &M, ModuleAnalysisManager &) {
  bool Changed = false;

  // Remove empty functions from the global ctors list.
  Changed |= optimizeGlobalCtorsList(M, isEmptyFunction);

  // Collect the set of members for each comdat.
  for (Function &F : M)
    if (Comdat *C = F.getComdat())
      ComdatMembers.insert(std::make_pair(C, &F));
  for (GlobalVariable &GV : M.globals())
    if (Comdat *C = GV.getComdat())
      ComdatMembers.insert(std::make_pair(C, &GV));
  for (GlobalAlias &GA : M.aliases())
    if (Comdat *C = GA.getComdat())
      ComdatMembers.insert(std::make_pair(C, &GA));

  // Loop over the module, adding globals which are obviously necessary.
  for (GlobalObject &GO : M.global_objects()) {
    Changed |= RemoveUnusedGlobalValue(GO);
    // Functions with external linkage are needed if they have a body.
    // Externally visible & appending globals are needed, if they have an
    // initializer.
    if (!GO.isDeclaration() && !GO.hasAvailableExternallyLinkage())
      if (!GO.isDiscardableIfUnused())
        GlobalIsNeeded(&GO);
  }

  for (GlobalAlias &GA : M.aliases()) {
    Changed |= RemoveUnusedGlobalValue(GA);
    // Externally visible aliases are needed.
    if (!GA.isDiscardableIfUnused())
      GlobalIsNeeded(&GA);
  }

  for (GlobalIFunc &GIF : M.ifuncs()) {
    Changed |= RemoveUnusedGlobalValue(GIF);
    // Externally visible ifuncs are needed.
    if (!GIF.isDiscardableIfUnused())
      GlobalIsNeeded(&GIF);
  }

  // Now that all globals which are needed are in the AliveGlobals set, we loop
  // through the program, deleting those which are not alive.
  //

  // The first pass is to drop initializers of global variables which are dead.
  std::vector<GlobalVariable *> DeadGlobalVars; // Keep track of dead globals
  for (GlobalVariable &GV : M.globals())
    if (!AliveGlobals.count(&GV)) {
      DeadGlobalVars.push_back(&GV);         // Keep track of dead globals
      if (GV.hasInitializer()) {
        Constant *Init = GV.getInitializer();
        GV.setInitializer(nullptr);
        if (isSafeToDestroyConstant(Init))
          Init->destroyConstant();
      }
    }

  // The second pass drops the bodies of functions which are dead...
  std::vector<Function *> DeadFunctions;
  for (Function &F : M)
    if (!AliveGlobals.count(&F)) {
      DeadFunctions.push_back(&F);         // Keep track of dead globals
      if (!F.isDeclaration())
        F.deleteBody();
    }

  // The third pass drops targets of aliases which are dead...
  std::vector<GlobalAlias*> DeadAliases;
  for (GlobalAlias &GA : M.aliases())
    if (!AliveGlobals.count(&GA)) {
      DeadAliases.push_back(&GA);
      GA.setAliasee(nullptr);
    }

  // The third pass drops targets of ifuncs which are dead...
  std::vector<GlobalIFunc*> DeadIFuncs;
  for (GlobalIFunc &GIF : M.ifuncs())
    if (!AliveGlobals.count(&GIF)) {
      DeadIFuncs.push_back(&GIF);
      GIF.setResolver(nullptr);
    }

  // Now that all interferences have been dropped, delete the actual objects
  // themselves.
  auto EraseUnusedGlobalValue = [&](GlobalValue *GV) {
    RemoveUnusedGlobalValue(*GV);
    GV->eraseFromParent();
    Changed = true;
  };

  NumFunctions += DeadFunctions.size();
  for (Function *F : DeadFunctions)
    EraseUnusedGlobalValue(F);

  NumVariables += DeadGlobalVars.size();
  for (GlobalVariable *GV : DeadGlobalVars)
    EraseUnusedGlobalValue(GV);

  NumAliases += DeadAliases.size();
  for (GlobalAlias *GA : DeadAliases)
    EraseUnusedGlobalValue(GA);

  NumIFuncs += DeadIFuncs.size();
  for (GlobalIFunc *GIF : DeadIFuncs)
    EraseUnusedGlobalValue(GIF);

  // Make sure that all memory is released
  AliveGlobals.clear();
  SeenConstants.clear();
  ComdatMembers.clear();

  if (Changed)
    return PreservedAnalyses::none();
  return PreservedAnalyses::all();
}

/// GlobalIsNeeded - the specific global value as needed, and
/// recursively mark anything that it uses as also needed.
void GlobalDCEPass::GlobalIsNeeded(GlobalValue *G) {
  // If the global is already in the set, no need to reprocess it.
  if (!AliveGlobals.insert(G).second)
    return;

  if (Comdat *C = G->getComdat()) {
    for (auto &&CM : make_range(ComdatMembers.equal_range(C)))
      GlobalIsNeeded(CM.second);
  }

  if (GlobalVariable *GV = dyn_cast<GlobalVariable>(G)) {
    // If this is a global variable, we must make sure to add any global values
    // referenced by the initializer to the alive set.
    if (GV->hasInitializer())
      MarkUsedGlobalsAsNeeded(GV->getInitializer());
  } else if (GlobalIndirectSymbol *GIS = dyn_cast<GlobalIndirectSymbol>(G)) {
    // The target of a global alias or ifunc is needed.
    MarkUsedGlobalsAsNeeded(GIS->getIndirectSymbol());
  } else {
    // Otherwise this must be a function object.  We have to scan the body of
    // the function looking for constants and global values which are used as
    // operands.  Any operands of these types must be processed to ensure that
    // any globals used will be marked as needed.
    Function *F = cast<Function>(G);

    for (Use &U : F->operands())
      MarkUsedGlobalsAsNeeded(cast<Constant>(U.get()));

    for (BasicBlock &BB : *F)
      for (Instruction &I : BB)
        for (Use &U : I.operands())
          if (GlobalValue *GV = dyn_cast<GlobalValue>(U))
            GlobalIsNeeded(GV);
          else if (Constant *C = dyn_cast<Constant>(U))
            MarkUsedGlobalsAsNeeded(C);
  }
}

void GlobalDCEPass::MarkUsedGlobalsAsNeeded(Constant *C) {
  if (GlobalValue *GV = dyn_cast<GlobalValue>(C))
    return GlobalIsNeeded(GV);

  // Loop over all of the operands of the constant, adding any globals they
  // use to the list of needed globals.
  for (Use &U : C->operands()) {
    // If we've already processed this constant there's no need to do it again.
    Constant *Op = dyn_cast<Constant>(U);
    if (Op && SeenConstants.insert(Op).second)
      MarkUsedGlobalsAsNeeded(Op);
  }
}

// RemoveUnusedGlobalValue - Loop over all of the uses of the specified
// GlobalValue, looking for the constant pointer ref that may be pointing to it.
// If found, check to see if the constant pointer ref is safe to destroy, and if
// so, nuke it.  This will reduce the reference count on the global value, which
// might make it deader.
//
bool GlobalDCEPass::RemoveUnusedGlobalValue(GlobalValue &GV) {
  if (GV.use_empty())
    return false;
  GV.removeDeadConstantUsers();
  return GV.use_empty();
}
