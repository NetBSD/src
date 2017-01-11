//===--- IndexSymbol.cpp - Types and functions for indexing symbols -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Index/IndexSymbol.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/PrettyPrinter.h"

using namespace clang;
using namespace clang::index;

/// \returns true if \c D is a subclass of 'XCTestCase'.
static bool isUnitTestCase(const ObjCInterfaceDecl *D) {
  if (!D)
    return false;
  while (const ObjCInterfaceDecl *SuperD = D->getSuperClass()) {
    if (SuperD->getName() == "XCTestCase")
      return true;
    D = SuperD;
  }
  return false;
}

/// \returns true if \c D is in a subclass of 'XCTestCase', returns void, has
/// no parameters, and its name starts with 'test'.
static bool isUnitTest(const ObjCMethodDecl *D) {
  if (!D->parameters().empty())
    return false;
  if (!D->getReturnType()->isVoidType())
    return false;
  if (!D->getSelector().getNameForSlot(0).startswith("test"))
    return false;
  return isUnitTestCase(D->getClassInterface());
}

static void checkForIBOutlets(const Decl *D, SymbolPropertySet &PropSet) {
  if (D->hasAttr<IBOutletAttr>()) {
    PropSet |= (unsigned)SymbolProperty::IBAnnotated;
  } else if (D->hasAttr<IBOutletCollectionAttr>()) {
    PropSet |= (unsigned)SymbolProperty::IBAnnotated;
    PropSet |= (unsigned)SymbolProperty::IBOutletCollection;
  }
}

SymbolInfo index::getSymbolInfo(const Decl *D) {
  assert(D);
  SymbolInfo Info;
  Info.Kind = SymbolKind::Unknown;
  Info.SubKind = SymbolSubKind::None;
  Info.Properties = SymbolPropertySet();
  Info.Lang = SymbolLanguage::C;

  if (const TagDecl *TD = dyn_cast<TagDecl>(D)) {
    switch (TD->getTagKind()) {
    case TTK_Struct:
      Info.Kind = SymbolKind::Struct; break;
    case TTK_Union:
      Info.Kind = SymbolKind::Union; break;
    case TTK_Class:
      Info.Kind = SymbolKind::Class;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case TTK_Interface:
      Info.Kind = SymbolKind::Protocol;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case TTK_Enum:
      Info.Kind = SymbolKind::Enum; break;
    }

    if (const CXXRecordDecl *CXXRec = dyn_cast<CXXRecordDecl>(D)) {
      if (!CXXRec->isCLike()) {
        Info.Lang = SymbolLanguage::CXX;
        if (CXXRec->getDescribedClassTemplate()) {
          Info.Properties |= (unsigned)SymbolProperty::Generic;
        }
      }
    }

    if (isa<ClassTemplatePartialSpecializationDecl>(D)) {
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Properties |= (unsigned)SymbolProperty::TemplatePartialSpecialization;
    } else if (isa<ClassTemplateSpecializationDecl>(D)) {
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Properties |= (unsigned)SymbolProperty::TemplateSpecialization;
    }

  } else if (auto *VD = dyn_cast<VarDecl>(D)) {
    Info.Kind = SymbolKind::Variable;
    if (isa<CXXRecordDecl>(D->getDeclContext())) {
      Info.Kind = SymbolKind::StaticProperty;
      Info.Lang = SymbolLanguage::CXX;
    }
    if (isa<VarTemplatePartialSpecializationDecl>(D)) {
      Info.Lang = SymbolLanguage::CXX;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Properties |= (unsigned)SymbolProperty::TemplatePartialSpecialization;
    } else if (isa<VarTemplateSpecializationDecl>(D)) {
      Info.Lang = SymbolLanguage::CXX;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Properties |= (unsigned)SymbolProperty::TemplateSpecialization;
    } else if (VD->getDescribedVarTemplate()) {
      Info.Lang = SymbolLanguage::CXX;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
    }

  } else {
    switch (D->getKind()) {
    case Decl::Import:
      Info.Kind = SymbolKind::Module;
      break;
    case Decl::Typedef:
      Info.Kind = SymbolKind::TypeAlias; break; // Lang = C
    case Decl::Function:
      Info.Kind = SymbolKind::Function;
      break;
    case Decl::Field:
      Info.Kind = SymbolKind::Field;
      if (const CXXRecordDecl *
            CXXRec = dyn_cast<CXXRecordDecl>(D->getDeclContext())) {
        if (!CXXRec->isCLike())
          Info.Lang = SymbolLanguage::CXX;
      }
      break;
    case Decl::EnumConstant:
      Info.Kind = SymbolKind::EnumConstant; break;
    case Decl::ObjCInterface:
    case Decl::ObjCImplementation: {
      Info.Kind = SymbolKind::Class;
      Info.Lang = SymbolLanguage::ObjC;
      const ObjCInterfaceDecl *ClsD = dyn_cast<ObjCInterfaceDecl>(D);
      if (!ClsD)
        ClsD = cast<ObjCImplementationDecl>(D)->getClassInterface();
      if (isUnitTestCase(ClsD))
        Info.Properties |= (unsigned)SymbolProperty::UnitTest;
      break;
    }
    case Decl::ObjCProtocol:
      Info.Kind = SymbolKind::Protocol;
      Info.Lang = SymbolLanguage::ObjC;
      break;
    case Decl::ObjCCategory:
    case Decl::ObjCCategoryImpl:
      Info.Kind = SymbolKind::Extension;
      Info.Lang = SymbolLanguage::ObjC;
      break;
    case Decl::ObjCMethod:
      if (cast<ObjCMethodDecl>(D)->isInstanceMethod())
        Info.Kind = SymbolKind::InstanceMethod;
      else
        Info.Kind = SymbolKind::ClassMethod;
      Info.Lang = SymbolLanguage::ObjC;
      if (isUnitTest(cast<ObjCMethodDecl>(D)))
        Info.Properties |= (unsigned)SymbolProperty::UnitTest;
      if (D->hasAttr<IBActionAttr>())
        Info.Properties |= (unsigned)SymbolProperty::IBAnnotated;
      break;
    case Decl::ObjCProperty:
      Info.Kind = SymbolKind::InstanceProperty;
      Info.Lang = SymbolLanguage::ObjC;
      checkForIBOutlets(D, Info.Properties);
      if (auto *Annot = D->getAttr<AnnotateAttr>()) {
        if (Annot->getAnnotation() == "gk_inspectable")
          Info.Properties |= (unsigned)SymbolProperty::GKInspectable;
      }
      break;
    case Decl::ObjCIvar:
      Info.Kind = SymbolKind::Field;
      Info.Lang = SymbolLanguage::ObjC;
      checkForIBOutlets(D, Info.Properties);
      break;
    case Decl::Namespace:
      Info.Kind = SymbolKind::Namespace;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case Decl::NamespaceAlias:
      Info.Kind = SymbolKind::NamespaceAlias;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case Decl::CXXConstructor: {
      Info.Kind = SymbolKind::Constructor;
      Info.Lang = SymbolLanguage::CXX;
      auto *CD = cast<CXXConstructorDecl>(D);
      if (CD->isCopyConstructor())
        Info.SubKind = SymbolSubKind::CXXCopyConstructor;
      else if (CD->isMoveConstructor())
        Info.SubKind = SymbolSubKind::CXXMoveConstructor;
      break;
    }
    case Decl::CXXDestructor:
      Info.Kind = SymbolKind::Destructor;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case Decl::CXXConversion:
      Info.Kind = SymbolKind::ConversionFunction;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case Decl::CXXMethod: {
      const CXXMethodDecl *MD = cast<CXXMethodDecl>(D);
      if (MD->isStatic())
        Info.Kind = SymbolKind::StaticMethod;
      else
        Info.Kind = SymbolKind::InstanceMethod;
      Info.Lang = SymbolLanguage::CXX;
      break;
    }
    case Decl::ClassTemplate:
      Info.Kind = SymbolKind::Class;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Lang = SymbolLanguage::CXX;
      break;
    case Decl::FunctionTemplate:
      Info.Kind = SymbolKind::Function;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Lang = SymbolLanguage::CXX;
      if (const CXXMethodDecl *MD = dyn_cast_or_null<CXXMethodDecl>(
                           cast<FunctionTemplateDecl>(D)->getTemplatedDecl())) {
        if (isa<CXXConstructorDecl>(MD))
          Info.Kind = SymbolKind::Constructor;
        else if (isa<CXXDestructorDecl>(MD))
          Info.Kind = SymbolKind::Destructor;
        else if (isa<CXXConversionDecl>(MD))
          Info.Kind = SymbolKind::ConversionFunction;
        else {
          if (MD->isStatic())
            Info.Kind = SymbolKind::StaticMethod;
          else
            Info.Kind = SymbolKind::InstanceMethod;
        }
      }
      break;
    case Decl::TypeAliasTemplate:
      Info.Kind = SymbolKind::TypeAlias;
      Info.Lang = SymbolLanguage::CXX;
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      break;
    case Decl::TypeAlias:
      Info.Kind = SymbolKind::TypeAlias;
      Info.Lang = SymbolLanguage::CXX;
      break;
    default:
      break;
    }
  }

  if (Info.Kind == SymbolKind::Unknown)
    return Info;

  if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
    if (FD->getTemplatedKind() ==
          FunctionDecl::TK_FunctionTemplateSpecialization) {
      Info.Properties |= (unsigned)SymbolProperty::Generic;
      Info.Properties |= (unsigned)SymbolProperty::TemplateSpecialization;
    }
  }

  if (Info.Properties & (unsigned)SymbolProperty::Generic)
    Info.Lang = SymbolLanguage::CXX;

  return Info;
}

void index::applyForEachSymbolRole(SymbolRoleSet Roles,
                                   llvm::function_ref<void(SymbolRole)> Fn) {
#define APPLY_FOR_ROLE(Role) \
  if (Roles & (unsigned)SymbolRole::Role) \
    Fn(SymbolRole::Role)

  APPLY_FOR_ROLE(Declaration);
  APPLY_FOR_ROLE(Definition);
  APPLY_FOR_ROLE(Reference);
  APPLY_FOR_ROLE(Read);
  APPLY_FOR_ROLE(Write);
  APPLY_FOR_ROLE(Call);
  APPLY_FOR_ROLE(Dynamic);
  APPLY_FOR_ROLE(AddressOf);
  APPLY_FOR_ROLE(Implicit);
  APPLY_FOR_ROLE(RelationChildOf);
  APPLY_FOR_ROLE(RelationBaseOf);
  APPLY_FOR_ROLE(RelationOverrideOf);
  APPLY_FOR_ROLE(RelationReceivedBy);
  APPLY_FOR_ROLE(RelationCalledBy);
  APPLY_FOR_ROLE(RelationExtendedBy);
  APPLY_FOR_ROLE(RelationAccessorOf);

#undef APPLY_FOR_ROLE
}

void index::printSymbolRoles(SymbolRoleSet Roles, raw_ostream &OS) {
  bool VisitedOnce = false;
  applyForEachSymbolRole(Roles, [&](SymbolRole Role) {
    if (VisitedOnce)
      OS << ',';
    else
      VisitedOnce = true;
    switch (Role) {
    case SymbolRole::Declaration: OS << "Decl"; break;
    case SymbolRole::Definition: OS << "Def"; break;
    case SymbolRole::Reference: OS << "Ref"; break;
    case SymbolRole::Read: OS << "Read"; break;
    case SymbolRole::Write: OS << "Writ"; break;
    case SymbolRole::Call: OS << "Call"; break;
    case SymbolRole::Dynamic: OS << "Dyn"; break;
    case SymbolRole::AddressOf: OS << "Addr"; break;
    case SymbolRole::Implicit: OS << "Impl"; break;
    case SymbolRole::RelationChildOf: OS << "RelChild"; break;
    case SymbolRole::RelationBaseOf: OS << "RelBase"; break;
    case SymbolRole::RelationOverrideOf: OS << "RelOver"; break;
    case SymbolRole::RelationReceivedBy: OS << "RelRec"; break;
    case SymbolRole::RelationCalledBy: OS << "RelCall"; break;
    case SymbolRole::RelationExtendedBy: OS << "RelExt"; break;
    case SymbolRole::RelationAccessorOf: OS << "RelAcc"; break;
    }
  });
}

bool index::printSymbolName(const Decl *D, const LangOptions &LO,
                            raw_ostream &OS) {
  if (auto *ND = dyn_cast<NamedDecl>(D)) {
    PrintingPolicy Policy(LO);
    // Forward references can have different template argument names. Suppress
    // the template argument names in constructors to make their name more
    // stable.
    Policy.SuppressTemplateArgsInCXXConstructors = true;
    DeclarationName DeclName = ND->getDeclName();
    if (DeclName.isEmpty())
      return true;
    DeclName.print(OS, Policy);
    return false;
  } else {
    return true;
  }
}

StringRef index::getSymbolKindString(SymbolKind K) {
  switch (K) {
  case SymbolKind::Unknown: return "<unknown>";
  case SymbolKind::Module: return "module";
  case SymbolKind::Namespace: return "namespace";
  case SymbolKind::NamespaceAlias: return "namespace-alias";
  case SymbolKind::Macro: return "macro";
  case SymbolKind::Enum: return "enum";
  case SymbolKind::Struct: return "struct";
  case SymbolKind::Class: return "class";
  case SymbolKind::Protocol: return "protocol";
  case SymbolKind::Extension: return "extension";
  case SymbolKind::Union: return "union";
  case SymbolKind::TypeAlias: return "type-alias";
  case SymbolKind::Function: return "function";
  case SymbolKind::Variable: return "variable";
  case SymbolKind::Field: return "field";
  case SymbolKind::EnumConstant: return "enumerator";
  case SymbolKind::InstanceMethod: return "instance-method";
  case SymbolKind::ClassMethod: return "class-method";
  case SymbolKind::StaticMethod: return "static-method";
  case SymbolKind::InstanceProperty: return "instance-property";
  case SymbolKind::ClassProperty: return "class-property";
  case SymbolKind::StaticProperty: return "static-property";
  case SymbolKind::Constructor: return "constructor";
  case SymbolKind::Destructor: return "destructor";
  case SymbolKind::ConversionFunction: return "coversion-func";
  }
  llvm_unreachable("invalid symbol kind");
}

StringRef index::getSymbolSubKindString(SymbolSubKind K) {
  switch (K) {
  case SymbolSubKind::None: return "<none>";
  case SymbolSubKind::CXXCopyConstructor: return "cxx-copy-ctor";
  case SymbolSubKind::CXXMoveConstructor: return "cxx-move-ctor";
  }
  llvm_unreachable("invalid symbol subkind");
}

StringRef index::getSymbolLanguageString(SymbolLanguage K) {
  switch (K) {
  case SymbolLanguage::C: return "C";
  case SymbolLanguage::ObjC: return "ObjC";
  case SymbolLanguage::CXX: return "C++";
  }
  llvm_unreachable("invalid symbol language kind");
}

void index::applyForEachSymbolProperty(SymbolPropertySet Props,
                                  llvm::function_ref<void(SymbolProperty)> Fn) {
#define APPLY_FOR_PROPERTY(K) \
  if (Props & (unsigned)SymbolProperty::K) \
    Fn(SymbolProperty::K)

  APPLY_FOR_PROPERTY(Generic);
  APPLY_FOR_PROPERTY(TemplatePartialSpecialization);
  APPLY_FOR_PROPERTY(TemplateSpecialization);
  APPLY_FOR_PROPERTY(UnitTest);
  APPLY_FOR_PROPERTY(IBAnnotated);
  APPLY_FOR_PROPERTY(IBOutletCollection);
  APPLY_FOR_PROPERTY(GKInspectable);

#undef APPLY_FOR_PROPERTY
}

void index::printSymbolProperties(SymbolPropertySet Props, raw_ostream &OS) {
  bool VisitedOnce = false;
  applyForEachSymbolProperty(Props, [&](SymbolProperty Prop) {
    if (VisitedOnce)
      OS << ',';
    else
      VisitedOnce = true;
    switch (Prop) {
    case SymbolProperty::Generic: OS << "Gen"; break;
    case SymbolProperty::TemplatePartialSpecialization: OS << "TPS"; break;
    case SymbolProperty::TemplateSpecialization: OS << "TS"; break;
    case SymbolProperty::UnitTest: OS << "test"; break;
    case SymbolProperty::IBAnnotated: OS << "IB"; break;
    case SymbolProperty::IBOutletCollection: OS << "IBColl"; break;
    case SymbolProperty::GKInspectable: OS << "GKI"; break;
    }
  });
}
