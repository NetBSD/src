//===- ConstructionContext.cpp - CFG constructor information --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ConstructionContext class and its sub-classes,
// which represent various different ways of constructing C++ objects
// with the additional information the users may want to know about
// the constructor.
//
//===----------------------------------------------------------------------===//

#include "clang/Analysis/ConstructionContext.h"

using namespace clang;

const ConstructionContextLayer *
ConstructionContextLayer::create(BumpVectorContext &C, TriggerTy Trigger,
                                 const ConstructionContextLayer *Parent) {
  ConstructionContextLayer *CC =
      C.getAllocator().Allocate<ConstructionContextLayer>();
  return new (CC) ConstructionContextLayer(Trigger, Parent);
}

bool ConstructionContextLayer::isStrictlyMoreSpecificThan(
    const ConstructionContextLayer *Other) const {
  const ConstructionContextLayer *Self = this;
  while (true) {
    if (!Other)
      return Self;
    if (!Self || !Self->isSameLayer(Other))
      return false;
    Self = Self->getParent();
    Other = Other->getParent();
  }
  llvm_unreachable("The above loop can only be terminated via return!");
}

const ConstructionContext *ConstructionContext::createFromLayers(
    BumpVectorContext &C, const ConstructionContextLayer *TopLayer) {
  // Before this point all we've had was a stockpile of arbitrary layers.
  // Now validate that it is shaped as one of the finite amount of expected
  // patterns.
  if (const Stmt *S = TopLayer->getTriggerStmt()) {
    if (const auto *DS = dyn_cast<DeclStmt>(S)) {
      assert(TopLayer->isLast());
      return create<SimpleVariableConstructionContext>(C, DS);
    }
    if (const auto *NE = dyn_cast<CXXNewExpr>(S)) {
      assert(TopLayer->isLast());
      return create<NewAllocatedObjectConstructionContext>(C, NE);
    }
    if (const auto *BTE = dyn_cast<CXXBindTemporaryExpr>(S)) {
      const MaterializeTemporaryExpr *MTE = nullptr;
      assert(BTE->getType().getCanonicalType()
                ->getAsCXXRecordDecl()->hasNonTrivialDestructor());
      // For temporaries with destructors, there may or may not be
      // lifetime extension on the parent layer.
      if (const ConstructionContextLayer *ParentLayer = TopLayer->getParent()) {
        // C++17 *requires* elision of the constructor at the return site
        // and at variable/member initialization site, while previous standards
        // were allowing an optional elidable constructor.
        // This is the C++17 copy-elided construction into a ctor initializer.
        if (const CXXCtorInitializer *I = ParentLayer->getTriggerInit()) {
          return create<
              CXX17ElidedCopyConstructorInitializerConstructionContext>(C,
                                                                        I, BTE);
        }
        assert(ParentLayer->getTriggerStmt() &&
               "Non-statement-based layers have been handled above!");
        // This is the normal, non-C++17 case: a temporary object which has
        // both destruction and materialization info attached to it in the AST.
        if ((MTE = dyn_cast<MaterializeTemporaryExpr>(
                 ParentLayer->getTriggerStmt()))) {
          if (MTE->getStorageDuration() != SD_FullExpression) {
            // If the temporary is lifetime-extended, don't save the BTE,
            // because we don't need a temporary destructor, but an automatic
            // destructor.
            BTE = nullptr;
          }

          // Handle pre-C++17 copy and move elision.
          const CXXConstructExpr *ElidedCE = nullptr;
          const ConstructionContext *ElidedCC = nullptr;
          if (const ConstructionContextLayer *ElidedLayer =
                  ParentLayer->getParent()) {
            ElidedCE = cast<CXXConstructExpr>(ElidedLayer->getTriggerStmt());
            assert(ElidedCE->isElidable());
            // We're creating a construction context that might have already
            // been created elsewhere. Maybe we should unique our construction
            // contexts. That's what we often do, but in this case it's unlikely
            // to bring any benefits.
            ElidedCC = createFromLayers(C, ElidedLayer->getParent());
            if (!ElidedCC) {
              // We may fail to create the elided construction context.
              // In this case, skip copy elision entirely.
              return create<SimpleTemporaryObjectConstructionContext>(C, BTE,
                                                                      MTE);
            } else {
              return create<ElidedTemporaryObjectConstructionContext>(
                  C, BTE, MTE, ElidedCE, ElidedCC);
            }
          }
          assert(ParentLayer->isLast());
          return create<SimpleTemporaryObjectConstructionContext>(C, BTE, MTE);
        }
        assert(ParentLayer->isLast());

        // This is a constructor into a function argument. Not implemented yet.
        if (isa<CallExpr>(ParentLayer->getTriggerStmt()))
          return nullptr;
        // This is C++17 copy-elided construction into return statement.
        if (auto *RS = dyn_cast<ReturnStmt>(ParentLayer->getTriggerStmt())) {
          assert(!RS->getRetValue()->getType().getCanonicalType()
                    ->getAsCXXRecordDecl()->hasTrivialDestructor());
          return create<CXX17ElidedCopyReturnedValueConstructionContext>(C,
                                                                       RS, BTE);
        }
        // This is C++17 copy-elided construction into a simple variable.
        if (auto *DS = dyn_cast<DeclStmt>(ParentLayer->getTriggerStmt())) {
          assert(!cast<VarDecl>(DS->getSingleDecl())->getType()
                      .getCanonicalType()->getAsCXXRecordDecl()
                      ->hasTrivialDestructor());
          return create<CXX17ElidedCopyVariableConstructionContext>(C, DS, BTE);
        }
        llvm_unreachable("Unexpected construction context with destructor!");
      }
      // A temporary object that doesn't require materialization.
      // In particular, it shouldn't require copy elision, because
      // copy/move constructors take a reference, which requires
      // materialization to obtain the glvalue.
      return create<SimpleTemporaryObjectConstructionContext>(C, BTE,
                                                              /*MTE=*/nullptr);
    }
    if (const auto *MTE = dyn_cast<MaterializeTemporaryExpr>(S)) {
      // If the object requires destruction and is not lifetime-extended,
      // then it must have a BTE within its MTE.
      // FIXME: This should be an assertion.
      if (!(MTE->getType().getCanonicalType()
                ->getAsCXXRecordDecl()->hasTrivialDestructor() ||
             MTE->getStorageDuration() != SD_FullExpression))
        return nullptr;

      // Handle pre-C++17 copy and move elision.
      const CXXConstructExpr *ElidedCE = nullptr;
      const ConstructionContext *ElidedCC = nullptr;
      if (const ConstructionContextLayer *ElidedLayer = TopLayer->getParent()) {
        ElidedCE = cast<CXXConstructExpr>(ElidedLayer->getTriggerStmt());
        assert(ElidedCE->isElidable());
        // We're creating a construction context that might have already
        // been created elsewhere. Maybe we should unique our construction
        // contexts. That's what we often do, but in this case it's unlikely
        // to bring any benefits.
        ElidedCC = createFromLayers(C, ElidedLayer->getParent());
        if (!ElidedCC) {
          // We may fail to create the elided construction context.
          // In this case, skip copy elision entirely.
          return create<SimpleTemporaryObjectConstructionContext>(C, nullptr,
                                                                  MTE);
        }
        return create<ElidedTemporaryObjectConstructionContext>(
            C, nullptr, MTE, ElidedCE, ElidedCC);
      }
      assert(TopLayer->isLast());
      return create<SimpleTemporaryObjectConstructionContext>(C, nullptr, MTE);
    }
    if (const auto *RS = dyn_cast<ReturnStmt>(S)) {
      assert(TopLayer->isLast());
      return create<SimpleReturnedValueConstructionContext>(C, RS);
    }
    // This is a constructor into a function argument. Not implemented yet.
    if (isa<CallExpr>(TopLayer->getTriggerStmt()))
      return nullptr;
    llvm_unreachable("Unexpected construction context with statement!");
  } else if (const CXXCtorInitializer *I = TopLayer->getTriggerInit()) {
    assert(TopLayer->isLast());
    return create<SimpleConstructorInitializerConstructionContext>(C, I);
  }
  llvm_unreachable("Unexpected construction context!");
}
