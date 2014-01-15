//===-- SITypeRewriter.cpp - Remove unwanted types ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This pass removes performs the following type substitution on all
/// non-compute shaders:
///
/// v16i8 => i128
///   - v16i8 is used for constant memory resource descriptors.  This type is
///      legal for some compute APIs, and we don't want to declare it as legal
///      in the backend, because we want the legalizer to expand all v16i8
///      operations.
/// v1* => *
///   - Having v1* types complicates the legalizer and we can easily replace
///   - them with the element type.
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/InstVisitor.h"

using namespace llvm;

namespace {

class SITypeRewriter : public FunctionPass,
                       public InstVisitor<SITypeRewriter> {

  static char ID;
  Module *Mod;
  Type *v16i8;
  Type *i128;

public:
  SITypeRewriter() : FunctionPass(ID) { }
  virtual bool doInitialization(Module &M);
  virtual bool runOnFunction(Function &F);
  virtual const char *getPassName() const {
    return "SI Type Rewriter";
  }
  void visitLoadInst(LoadInst &I);
  void visitCallInst(CallInst &I);
  void visitBitCast(BitCastInst &I);
};

} // End anonymous namespace

char SITypeRewriter::ID = 0;

bool SITypeRewriter::doInitialization(Module &M) {
  Mod = &M;
  v16i8 = VectorType::get(Type::getInt8Ty(M.getContext()), 16);
  i128 = Type::getIntNTy(M.getContext(), 128);
  return false;
}

bool SITypeRewriter::runOnFunction(Function &F) {
  AttributeSet Set = F.getAttributes();
  Attribute A = Set.getAttribute(AttributeSet::FunctionIndex, "ShaderType");

  unsigned ShaderType = ShaderType::COMPUTE;
  if (A.isStringAttribute()) {
    StringRef Str = A.getValueAsString();
    Str.getAsInteger(0, ShaderType);
  }
  if (ShaderType != ShaderType::COMPUTE) {
    visit(F);
  }

  visit(F);

  return false;
}

void SITypeRewriter::visitLoadInst(LoadInst &I) {
  Value *Ptr = I.getPointerOperand();
  Type *PtrTy = Ptr->getType();
  Type *ElemTy = PtrTy->getPointerElementType();
  IRBuilder<> Builder(&I);
  if (ElemTy == v16i8)  {
    Value *BitCast = Builder.CreateBitCast(Ptr, Type::getIntNPtrTy(I.getContext(), 128, 2));
    LoadInst *Load = Builder.CreateLoad(BitCast);
    SmallVector <std::pair<unsigned, MDNode*>, 8> MD;
    I.getAllMetadataOtherThanDebugLoc(MD);
    for (unsigned i = 0, e = MD.size(); i != e; ++i) {
      Load->setMetadata(MD[i].first, MD[i].second);
    }
    Value *BitCastLoad = Builder.CreateBitCast(Load, I.getType());
    I.replaceAllUsesWith(BitCastLoad);
    I.eraseFromParent();
  }
}

void SITypeRewriter::visitCallInst(CallInst &I) {
  IRBuilder<> Builder(&I);
  SmallVector <Value*, 8> Args;
  SmallVector <Type*, 8> Types;
  bool NeedToReplace = false;
  Function *F = I.getCalledFunction();
  std::string Name = F->getName().str();
  for (unsigned i = 0, e = I.getNumArgOperands(); i != e; ++i) {
    Value *Arg = I.getArgOperand(i);
    if (Arg->getType() == v16i8) {
      Args.push_back(Builder.CreateBitCast(Arg, i128));
      Types.push_back(i128);
      NeedToReplace = true;
      Name = Name + ".i128";
    } else if (Arg->getType()->isVectorTy() &&
               Arg->getType()->getVectorNumElements() == 1 &&
               Arg->getType()->getVectorElementType() ==
                                              Type::getInt32Ty(I.getContext())){
      Type *ElementTy = Arg->getType()->getVectorElementType();
      std::string TypeName = "i32";
      InsertElementInst *Def = dyn_cast<InsertElementInst>(Arg);
      assert(Def);
      Args.push_back(Def->getOperand(1));
      Types.push_back(ElementTy);
      std::string VecTypeName = "v1" + TypeName;
      Name = Name.replace(Name.find(VecTypeName), VecTypeName.length(), TypeName);
      NeedToReplace = true;
    } else {
      Args.push_back(Arg);
      Types.push_back(Arg->getType());
    }
  }

  if (!NeedToReplace) {
    return;
  }
  Function *NewF = Mod->getFunction(Name);
  if (!NewF) {
    NewF = Function::Create(FunctionType::get(F->getReturnType(), Types, false), GlobalValue::ExternalLinkage, Name, Mod);
    NewF->setAttributes(F->getAttributes());
  }
  I.replaceAllUsesWith(Builder.CreateCall(NewF, Args));
  I.eraseFromParent();
}

void SITypeRewriter::visitBitCast(BitCastInst &I) {
  IRBuilder<> Builder(&I);
  if (I.getDestTy() != i128) {
    return;
  }

  if (BitCastInst *Op = dyn_cast<BitCastInst>(I.getOperand(0))) {
    if (Op->getSrcTy() == i128) {
      I.replaceAllUsesWith(Op->getOperand(0));
      I.eraseFromParent();
    }
  }
}

FunctionPass *llvm::createSITypeRewriter() {
  return new SITypeRewriter();
}
