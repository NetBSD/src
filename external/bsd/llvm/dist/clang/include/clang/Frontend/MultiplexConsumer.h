//===-- MultiplexConsumer.h - AST Consumer for PCH Generation ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file declares the MultiplexConsumer class, which can be used to
//  multiplex ASTConsumer and SemaConsumer messages to many consumers.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_FRONTEND_MULTIPLEXCONSUMER_H
#define CLANG_FRONTEND_MULTIPLEXCONSUMER_H

#include "clang/Basic/LLVM.h"
#include "clang/Sema/SemaConsumer.h"
#include <memory>
#include <vector>

namespace clang {

class MultiplexASTMutationListener;
class MultiplexASTDeserializationListener;

// Has a list of ASTConsumers and calls each of them. Owns its children.
class MultiplexConsumer : public SemaConsumer {
public:
  // Takes ownership of the pointers in C.
  MultiplexConsumer(ArrayRef<ASTConsumer*> C);
  ~MultiplexConsumer();

  // ASTConsumer
  void Initialize(ASTContext &Context) override;
  void HandleCXXStaticMemberVarInstantiation(VarDecl *VD) override;
  bool HandleTopLevelDecl(DeclGroupRef D) override;
  void HandleInlineMethodDefinition(CXXMethodDecl *D) override;
  void HandleInterestingDecl(DeclGroupRef D) override;
  void HandleTranslationUnit(ASTContext &Ctx) override;
  void HandleTagDeclDefinition(TagDecl *D) override;
  void HandleCXXImplicitFunctionInstantiation(FunctionDecl *D) override;
  void HandleTopLevelDeclInObjCContainer(DeclGroupRef D) override;
  void CompleteTentativeDefinition(VarDecl *D) override;
  void HandleVTable(CXXRecordDecl *RD, bool DefinitionRequired) override;
  ASTMutationListener *GetASTMutationListener() override;
  ASTDeserializationListener *GetASTDeserializationListener() override;
  void PrintStats() override;

  // SemaConsumer
  void InitializeSema(Sema &S) override;
  void ForgetSema() override;

private:
  std::vector<ASTConsumer*> Consumers;  // Owns these.
  std::unique_ptr<MultiplexASTMutationListener> MutationListener;
  std::unique_ptr<MultiplexASTDeserializationListener> DeserializationListener;
};

}  // end namespace clang

#endif
