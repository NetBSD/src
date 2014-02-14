//===--- ObjCMT.cpp - ObjC Migrate Tool -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Transforms.h"
#include "clang/ARCMigrate/ARCMT.h"
#include "clang/ARCMigrate/ARCMTActions.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/NSAPI.h"
#include "clang/AST/ParentMap.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Analysis/DomainSpecific/CocoaConventions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Edit/Commit.h"
#include "clang/Edit/EditedSource.h"
#include "clang/Edit/EditsReceiver.h"
#include "clang/Edit/Rewriters.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/MultiplexConsumer.h"
#include "clang/Lex/PPConditionalDirectiveRecord.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/StaticAnalyzer/Checkers/ObjCRetainCount.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/YAMLParser.h"

using namespace clang;
using namespace arcmt;
using namespace ento::objc_retain;

namespace {

class ObjCMigrateASTConsumer : public ASTConsumer {
  enum CF_BRIDGING_KIND {
    CF_BRIDGING_NONE,
    CF_BRIDGING_ENABLE,
    CF_BRIDGING_MAY_INCLUDE
  };
  
  void migrateDecl(Decl *D);
  void migrateObjCInterfaceDecl(ASTContext &Ctx, ObjCContainerDecl *D);
  void migrateProtocolConformance(ASTContext &Ctx,
                                  const ObjCImplementationDecl *ImpDecl);
  void CacheObjCNSIntegerTypedefed(const TypedefDecl *TypedefDcl);
  bool migrateNSEnumDecl(ASTContext &Ctx, const EnumDecl *EnumDcl,
                     const TypedefDecl *TypedefDcl);
  void migrateAllMethodInstaceType(ASTContext &Ctx, ObjCContainerDecl *CDecl);
  void migrateMethodInstanceType(ASTContext &Ctx, ObjCContainerDecl *CDecl,
                                 ObjCMethodDecl *OM);
  bool migrateProperty(ASTContext &Ctx, ObjCContainerDecl *D, ObjCMethodDecl *OM);
  void migrateNsReturnsInnerPointer(ASTContext &Ctx, ObjCMethodDecl *OM);
  void migratePropertyNsReturnsInnerPointer(ASTContext &Ctx, ObjCPropertyDecl *P);
  void migrateFactoryMethod(ASTContext &Ctx, ObjCContainerDecl *CDecl,
                            ObjCMethodDecl *OM,
                            ObjCInstanceTypeFamily OIT_Family = OIT_None);
  
  void migrateCFAnnotation(ASTContext &Ctx, const Decl *Decl);
  void AddCFAnnotations(ASTContext &Ctx, const CallEffects &CE,
                        const FunctionDecl *FuncDecl, bool ResultAnnotated);
  void AddCFAnnotations(ASTContext &Ctx, const CallEffects &CE,
                        const ObjCMethodDecl *MethodDecl, bool ResultAnnotated);
  
  void AnnotateImplicitBridging(ASTContext &Ctx);
  
  CF_BRIDGING_KIND migrateAddFunctionAnnotation(ASTContext &Ctx,
                                                const FunctionDecl *FuncDecl);
  
  void migrateARCSafeAnnotation(ASTContext &Ctx, ObjCContainerDecl *CDecl);
  
  void migrateAddMethodAnnotation(ASTContext &Ctx,
                                  const ObjCMethodDecl *MethodDecl);

  void inferDesignatedInitializers(ASTContext &Ctx,
                                   const ObjCImplementationDecl *ImplD);

public:
  std::string MigrateDir;
  unsigned ASTMigrateActions;
  FileID FileId;
  const TypedefDecl *NSIntegerTypedefed;
  const TypedefDecl *NSUIntegerTypedefed;
  OwningPtr<NSAPI> NSAPIObj;
  OwningPtr<edit::EditedSource> Editor;
  FileRemapper &Remapper;
  FileManager &FileMgr;
  const PPConditionalDirectiveRecord *PPRec;
  Preprocessor &PP;
  bool IsOutputFile;
  llvm::SmallPtrSet<ObjCProtocolDecl *, 32> ObjCProtocolDecls;
  llvm::SmallVector<const Decl *, 8> CFFunctionIBCandidates;
  llvm::StringMap<char> WhiteListFilenames;
  
  ObjCMigrateASTConsumer(StringRef migrateDir,
                         unsigned astMigrateActions,
                         FileRemapper &remapper,
                         FileManager &fileMgr,
                         const PPConditionalDirectiveRecord *PPRec,
                         Preprocessor &PP,
                         bool isOutputFile,
                         ArrayRef<std::string> WhiteList)
  : MigrateDir(migrateDir),
    ASTMigrateActions(astMigrateActions),
    NSIntegerTypedefed(0), NSUIntegerTypedefed(0),
    Remapper(remapper), FileMgr(fileMgr), PPRec(PPRec), PP(PP),
    IsOutputFile(isOutputFile) {

    for (ArrayRef<std::string>::iterator
           I = WhiteList.begin(), E = WhiteList.end(); I != E; ++I) {
      WhiteListFilenames.GetOrCreateValue(*I);
    }
  }

protected:
  virtual void Initialize(ASTContext &Context) {
    NSAPIObj.reset(new NSAPI(Context));
    Editor.reset(new edit::EditedSource(Context.getSourceManager(),
                                        Context.getLangOpts(),
                                        PPRec, false));
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef DG) {
    for (DeclGroupRef::iterator I = DG.begin(), E = DG.end(); I != E; ++I)
      migrateDecl(*I);
    return true;
  }
  virtual void HandleInterestingDecl(DeclGroupRef DG) {
    // Ignore decls from the PCH.
  }
  virtual void HandleTopLevelDeclInObjCContainer(DeclGroupRef DG) {
    ObjCMigrateASTConsumer::HandleTopLevelDecl(DG);
  }

  virtual void HandleTranslationUnit(ASTContext &Ctx);

  bool canModifyFile(StringRef Path) {
    if (WhiteListFilenames.empty())
      return true;
    return WhiteListFilenames.find(llvm::sys::path::filename(Path))
        != WhiteListFilenames.end();
  }
  bool canModifyFile(const FileEntry *FE) {
    if (!FE)
      return false;
    return canModifyFile(FE->getName());
  }
  bool canModifyFile(FileID FID) {
    if (FID.isInvalid())
      return false;
    return canModifyFile(PP.getSourceManager().getFileEntryForID(FID));
  }

  bool canModify(const Decl *D) {
    if (!D)
      return false;
    if (const ObjCCategoryImplDecl *CatImpl = dyn_cast<ObjCCategoryImplDecl>(D))
      return canModify(CatImpl->getCategoryDecl());
    if (const ObjCImplementationDecl *Impl = dyn_cast<ObjCImplementationDecl>(D))
      return canModify(Impl->getClassInterface());
    if (const ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(D))
      return canModify(cast<Decl>(MD->getDeclContext()));

    FileID FID = PP.getSourceManager().getFileID(D->getLocation());
    return canModifyFile(FID);
  }
};

}

ObjCMigrateAction::ObjCMigrateAction(FrontendAction *WrappedAction,
                                     StringRef migrateDir,
                                     unsigned migrateAction)
  : WrapperFrontendAction(WrappedAction), MigrateDir(migrateDir),
    ObjCMigAction(migrateAction),
    CompInst(0) {
  if (MigrateDir.empty())
    MigrateDir = "."; // user current directory if none is given.
}

ASTConsumer *ObjCMigrateAction::CreateASTConsumer(CompilerInstance &CI,
                                                  StringRef InFile) {
  PPConditionalDirectiveRecord *
    PPRec = new PPConditionalDirectiveRecord(CompInst->getSourceManager());
  CompInst->getPreprocessor().addPPCallbacks(PPRec);
  ASTConsumer *
    WrappedConsumer = WrapperFrontendAction::CreateASTConsumer(CI, InFile);
  ASTConsumer *MTConsumer = new ObjCMigrateASTConsumer(MigrateDir,
                                                       ObjCMigAction,
                                                       Remapper,
                                                    CompInst->getFileManager(),
                                                       PPRec,
                                                       CompInst->getPreprocessor(),
                                                       false,
                                                       ArrayRef<std::string>());
  ASTConsumer *Consumers[] = { MTConsumer, WrappedConsumer };
  return new MultiplexConsumer(Consumers);
}

bool ObjCMigrateAction::BeginInvocation(CompilerInstance &CI) {
  Remapper.initFromDisk(MigrateDir, CI.getDiagnostics(),
                        /*ignoreIfFilesChanges=*/true);
  CompInst = &CI;
  CI.getDiagnostics().setIgnoreAllWarnings(true);
  return true;
}

namespace {
class ObjCMigrator : public RecursiveASTVisitor<ObjCMigrator> {
  ObjCMigrateASTConsumer &Consumer;
  ParentMap &PMap;

public:
  ObjCMigrator(ObjCMigrateASTConsumer &consumer, ParentMap &PMap)
    : Consumer(consumer), PMap(PMap) { }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (Consumer.ASTMigrateActions & FrontendOptions::ObjCMT_Literals) {
      edit::Commit commit(*Consumer.Editor);
      edit::rewriteToObjCLiteralSyntax(E, *Consumer.NSAPIObj, commit, &PMap);
      Consumer.Editor->commit(commit);
    }

    if (Consumer.ASTMigrateActions & FrontendOptions::ObjCMT_Subscripting) {
      edit::Commit commit(*Consumer.Editor);
      edit::rewriteToObjCSubscriptSyntax(E, *Consumer.NSAPIObj, commit);
      Consumer.Editor->commit(commit);
    }

    return true;
  }

  bool TraverseObjCMessageExpr(ObjCMessageExpr *E) {
    // Do depth first; we want to rewrite the subexpressions first so that if
    // we have to move expressions we will move them already rewritten.
    for (Stmt::child_range range = E->children(); range; ++range)
      if (!TraverseStmt(*range))
        return false;

    return WalkUpFromObjCMessageExpr(E);
  }
};

class BodyMigrator : public RecursiveASTVisitor<BodyMigrator> {
  ObjCMigrateASTConsumer &Consumer;
  OwningPtr<ParentMap> PMap;

public:
  BodyMigrator(ObjCMigrateASTConsumer &consumer) : Consumer(consumer) { }

  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool TraverseStmt(Stmt *S) {
    PMap.reset(new ParentMap(S));
    ObjCMigrator(Consumer, *PMap).TraverseStmt(S);
    return true;
  }
};
}

void ObjCMigrateASTConsumer::migrateDecl(Decl *D) {
  if (!D)
    return;
  if (isa<ObjCMethodDecl>(D))
    return; // Wait for the ObjC container declaration.

  BodyMigrator(*this).TraverseDecl(D);
}

static void append_attr(std::string &PropertyString, const char *attr,
                        bool &LParenAdded) {
  if (!LParenAdded) {
    PropertyString += "(";
    LParenAdded = true;
  }
  else
    PropertyString += ", ";
  PropertyString += attr;
}

static
void MigrateBlockOrFunctionPointerTypeVariable(std::string & PropertyString,
                                               const std::string& TypeString,
                                               const char *name) {
  const char *argPtr = TypeString.c_str();
  int paren = 0;
  while (*argPtr) {
    switch (*argPtr) {
      case '(':
        PropertyString += *argPtr;
        paren++;
        break;
      case ')':
        PropertyString += *argPtr;
        paren--;
        break;
      case '^':
      case '*':
        PropertyString += (*argPtr);
        if (paren == 1) {
          PropertyString += name;
          name = "";
        }
        break;
      default:
        PropertyString += *argPtr;
        break;
    }
    argPtr++;
  }
}

static const char *PropertyMemoryAttribute(ASTContext &Context, QualType ArgType) {
  Qualifiers::ObjCLifetime propertyLifetime = ArgType.getObjCLifetime();
  bool RetainableObject = ArgType->isObjCRetainableType();
  if (RetainableObject &&
      (propertyLifetime == Qualifiers::OCL_Strong
       || propertyLifetime == Qualifiers::OCL_None)) {
    if (const ObjCObjectPointerType *ObjPtrTy =
        ArgType->getAs<ObjCObjectPointerType>()) {
      ObjCInterfaceDecl *IDecl = ObjPtrTy->getObjectType()->getInterface();
      if (IDecl &&
          IDecl->lookupNestedProtocol(&Context.Idents.get("NSCopying")))
        return "copy";
      else
        return "strong";
    }
    else if (ArgType->isBlockPointerType())
      return "copy";
  } else if (propertyLifetime == Qualifiers::OCL_Weak)
    // TODO. More precise determination of 'weak' attribute requires
    // looking into setter's implementation for backing weak ivar.
    return "weak";
  else if (RetainableObject)
    return ArgType->isBlockPointerType() ? "copy" : "strong";
  return 0;
}

static void rewriteToObjCProperty(const ObjCMethodDecl *Getter,
                                  const ObjCMethodDecl *Setter,
                                  const NSAPI &NS, edit::Commit &commit,
                                  unsigned LengthOfPrefix,
                                  bool Atomic, bool UseNsIosOnlyMacro,
                                  bool AvailabilityArgsMatch) {
  ASTContext &Context = NS.getASTContext();
  bool LParenAdded = false;
  std::string PropertyString = "@property ";
  if (UseNsIosOnlyMacro && Context.Idents.get("NS_NONATOMIC_IOSONLY").hasMacroDefinition()) {
    PropertyString += "(NS_NONATOMIC_IOSONLY";
    LParenAdded = true;
  } else if (!Atomic) {
    PropertyString += "(nonatomic";
    LParenAdded = true;
  }
  
  std::string PropertyNameString = Getter->getNameAsString();
  StringRef PropertyName(PropertyNameString);
  if (LengthOfPrefix > 0) {
    if (!LParenAdded) {
      PropertyString += "(getter=";
      LParenAdded = true;
    }
    else
      PropertyString += ", getter=";
    PropertyString += PropertyNameString;
  }
  // Property with no setter may be suggested as a 'readonly' property.
  if (!Setter)
    append_attr(PropertyString, "readonly", LParenAdded);
  
  
  // Short circuit 'delegate' properties that contain the name "delegate" or
  // "dataSource", or have exact name "target" to have 'assign' attribute.
  if (PropertyName.equals("target") ||
      (PropertyName.find("delegate") != StringRef::npos) ||
      (PropertyName.find("dataSource") != StringRef::npos)) {
    QualType QT = Getter->getReturnType();
    if (!QT->isRealType())
      append_attr(PropertyString, "assign", LParenAdded);
  } else if (!Setter) {
    QualType ResType = Context.getCanonicalType(Getter->getReturnType());
    if (const char *MemoryManagementAttr = PropertyMemoryAttribute(Context, ResType))
      append_attr(PropertyString, MemoryManagementAttr, LParenAdded);
  } else {
    const ParmVarDecl *argDecl = *Setter->param_begin();
    QualType ArgType = Context.getCanonicalType(argDecl->getType());
    if (const char *MemoryManagementAttr = PropertyMemoryAttribute(Context, ArgType))
      append_attr(PropertyString, MemoryManagementAttr, LParenAdded);
  }
  if (LParenAdded)
    PropertyString += ')';
  QualType RT = Getter->getReturnType();
  if (!isa<TypedefType>(RT)) {
    // strip off any ARC lifetime qualifier.
    QualType CanResultTy = Context.getCanonicalType(RT);
    if (CanResultTy.getQualifiers().hasObjCLifetime()) {
      Qualifiers Qs = CanResultTy.getQualifiers();
      Qs.removeObjCLifetime();
      RT = Context.getQualifiedType(CanResultTy.getUnqualifiedType(), Qs);
    }
  }
  PropertyString += " ";
  PrintingPolicy SubPolicy(Context.getPrintingPolicy());
  SubPolicy.SuppressStrongLifetime = true;
  SubPolicy.SuppressLifetimeQualifiers = true;
  std::string TypeString = RT.getAsString(SubPolicy);
  if (LengthOfPrefix > 0) {
    // property name must strip off "is" and lower case the first character
    // after that; e.g. isContinuous will become continuous.
    StringRef PropertyNameStringRef(PropertyNameString);
    PropertyNameStringRef = PropertyNameStringRef.drop_front(LengthOfPrefix);
    PropertyNameString = PropertyNameStringRef;
    bool NoLowering = (isUppercase(PropertyNameString[0]) &&
                       PropertyNameString.size() > 1 &&
                       isUppercase(PropertyNameString[1]));
    if (!NoLowering)
      PropertyNameString[0] = toLowercase(PropertyNameString[0]);
  }
  if (RT->isBlockPointerType() || RT->isFunctionPointerType())
    MigrateBlockOrFunctionPointerTypeVariable(PropertyString,
                                              TypeString,
                                              PropertyNameString.c_str());
  else {
    char LastChar = TypeString[TypeString.size()-1];
    PropertyString += TypeString;
    if (LastChar != '*')
      PropertyString += ' ';
    PropertyString += PropertyNameString;
  }
  SourceLocation StartGetterSelectorLoc = Getter->getSelectorStartLoc();
  Selector GetterSelector = Getter->getSelector();
  
  SourceLocation EndGetterSelectorLoc =
    StartGetterSelectorLoc.getLocWithOffset(GetterSelector.getNameForSlot(0).size());
  commit.replace(CharSourceRange::getCharRange(Getter->getLocStart(),
                                               EndGetterSelectorLoc),
                 PropertyString);
  if (Setter && AvailabilityArgsMatch) {
    SourceLocation EndLoc = Setter->getDeclaratorEndLoc();
    // Get location past ';'
    EndLoc = EndLoc.getLocWithOffset(1);
    SourceLocation BeginOfSetterDclLoc = Setter->getLocStart();
    // FIXME. This assumes that setter decl; is immediately preceded by eoln.
    // It is trying to remove the setter method decl. line entirely.
    BeginOfSetterDclLoc = BeginOfSetterDclLoc.getLocWithOffset(-1);
    commit.remove(SourceRange(BeginOfSetterDclLoc, EndLoc));
  }
}

static bool IsCategoryNameWithDeprecatedSuffix(ObjCContainerDecl *D) {
  if (ObjCCategoryDecl *CatDecl = dyn_cast<ObjCCategoryDecl>(D)) {
    StringRef Name = CatDecl->getName();
    return Name.endswith("Deprecated");
  }
  return false;
}

void ObjCMigrateASTConsumer::migrateObjCInterfaceDecl(ASTContext &Ctx,
                                                      ObjCContainerDecl *D) {
  if (D->isDeprecated() || IsCategoryNameWithDeprecatedSuffix(D))
    return;
    
  for (ObjCContainerDecl::method_iterator M = D->meth_begin(), MEnd = D->meth_end();
       M != MEnd; ++M) {
    ObjCMethodDecl *Method = (*M);
    if (Method->isDeprecated())
      continue;
    bool PropertyInferred = migrateProperty(Ctx, D, Method);
    // If a property is inferred, do not attempt to attach NS_RETURNS_INNER_POINTER to
    // the getter method as it ends up on the property itself which we don't want
    // to do unless -objcmt-returns-innerpointer-property  option is on.
    if (!PropertyInferred ||
        (ASTMigrateActions & FrontendOptions::ObjCMT_ReturnsInnerPointerProperty))
      if (ASTMigrateActions & FrontendOptions::ObjCMT_Annotation)
        migrateNsReturnsInnerPointer(Ctx, Method);
  }
  if (!(ASTMigrateActions & FrontendOptions::ObjCMT_ReturnsInnerPointerProperty))
    return;
  
  for (ObjCContainerDecl::prop_iterator P = D->prop_begin(),
       E = D->prop_end(); P != E; ++P) {
    ObjCPropertyDecl *Prop = *P;
    if ((ASTMigrateActions & FrontendOptions::ObjCMT_Annotation) &&
        !Prop->isDeprecated())
      migratePropertyNsReturnsInnerPointer(Ctx, Prop);
  }
}

static bool
ClassImplementsAllMethodsAndProperties(ASTContext &Ctx,
                                      const ObjCImplementationDecl *ImpDecl,
                                       const ObjCInterfaceDecl *IDecl,
                                      ObjCProtocolDecl *Protocol) {
  // In auto-synthesis, protocol properties are not synthesized. So,
  // a conforming protocol must have its required properties declared
  // in class interface.
  bool HasAtleastOneRequiredProperty = false;
  if (const ObjCProtocolDecl *PDecl = Protocol->getDefinition())
    for (ObjCProtocolDecl::prop_iterator P = PDecl->prop_begin(),
         E = PDecl->prop_end(); P != E; ++P) {
      ObjCPropertyDecl *Property = *P;
      if (Property->getPropertyImplementation() == ObjCPropertyDecl::Optional)
        continue;
      HasAtleastOneRequiredProperty = true;
      DeclContext::lookup_const_result R = IDecl->lookup(Property->getDeclName());
      if (R.size() == 0) {
        // Relax the rule and look into class's implementation for a synthesize
        // or dynamic declaration. Class is implementing a property coming from
        // another protocol. This still makes the target protocol as conforming.
        if (!ImpDecl->FindPropertyImplDecl(
                                  Property->getDeclName().getAsIdentifierInfo()))
          return false;
      }
      else if (ObjCPropertyDecl *ClassProperty = dyn_cast<ObjCPropertyDecl>(R[0])) {
          if ((ClassProperty->getPropertyAttributes()
              != Property->getPropertyAttributes()) ||
              !Ctx.hasSameType(ClassProperty->getType(), Property->getType()))
            return false;
      }
      else
        return false;
    }
  
  // At this point, all required properties in this protocol conform to those
  // declared in the class.
  // Check that class implements the required methods of the protocol too.
  bool HasAtleastOneRequiredMethod = false;
  if (const ObjCProtocolDecl *PDecl = Protocol->getDefinition()) {
    if (PDecl->meth_begin() == PDecl->meth_end())
      return HasAtleastOneRequiredProperty;
    for (ObjCContainerDecl::method_iterator M = PDecl->meth_begin(),
         MEnd = PDecl->meth_end(); M != MEnd; ++M) {
      ObjCMethodDecl *MD = (*M);
      if (MD->isImplicit())
        continue;
      if (MD->getImplementationControl() == ObjCMethodDecl::Optional)
        continue;
      DeclContext::lookup_const_result R = ImpDecl->lookup(MD->getDeclName());
      if (R.size() == 0)
        return false;
      bool match = false;
      HasAtleastOneRequiredMethod = true;
      for (unsigned I = 0, N = R.size(); I != N; ++I)
        if (ObjCMethodDecl *ImpMD = dyn_cast<ObjCMethodDecl>(R[0]))
          if (Ctx.ObjCMethodsAreEqual(MD, ImpMD)) {
            match = true;
            break;
          }
      if (!match)
        return false;
    }
  }
  if (HasAtleastOneRequiredProperty || HasAtleastOneRequiredMethod)
    return true;
  return false;
}

static bool rewriteToObjCInterfaceDecl(const ObjCInterfaceDecl *IDecl,
                    llvm::SmallVectorImpl<ObjCProtocolDecl*> &ConformingProtocols,
                    const NSAPI &NS, edit::Commit &commit) {
  const ObjCList<ObjCProtocolDecl> &Protocols = IDecl->getReferencedProtocols();
  std::string ClassString;
  SourceLocation EndLoc =
  IDecl->getSuperClass() ? IDecl->getSuperClassLoc() : IDecl->getLocation();
  
  if (Protocols.empty()) {
    ClassString = '<';
    for (unsigned i = 0, e = ConformingProtocols.size(); i != e; i++) {
      ClassString += ConformingProtocols[i]->getNameAsString();
      if (i != (e-1))
        ClassString += ", ";
    }
    ClassString += "> ";
  }
  else {
    ClassString = ", ";
    for (unsigned i = 0, e = ConformingProtocols.size(); i != e; i++) {
      ClassString += ConformingProtocols[i]->getNameAsString();
      if (i != (e-1))
        ClassString += ", ";
    }
    ObjCInterfaceDecl::protocol_loc_iterator PL = IDecl->protocol_loc_end() - 1;
    EndLoc = *PL;
  }
  
  commit.insertAfterToken(EndLoc, ClassString);
  return true;
}

static bool rewriteToNSEnumDecl(const EnumDecl *EnumDcl,
                                const TypedefDecl *TypedefDcl,
                                const NSAPI &NS, edit::Commit &commit,
                                bool IsNSIntegerType,
                                bool NSOptions) {
  std::string ClassString;
  if (NSOptions)
    ClassString = "typedef NS_OPTIONS(NSUInteger, ";
  else
    ClassString =
      IsNSIntegerType ? "typedef NS_ENUM(NSInteger, "
                      : "typedef NS_ENUM(NSUInteger, ";
  
  ClassString += TypedefDcl->getIdentifier()->getName();
  ClassString += ')';
  SourceRange R(EnumDcl->getLocStart(), EnumDcl->getLocStart());
  commit.replace(R, ClassString);
  SourceLocation EndOfEnumDclLoc = EnumDcl->getLocEnd();
  EndOfEnumDclLoc = trans::findSemiAfterLocation(EndOfEnumDclLoc,
                                                 NS.getASTContext(), /*IsDecl*/true);
  if (!EndOfEnumDclLoc.isInvalid()) {
    SourceRange EnumDclRange(EnumDcl->getLocStart(), EndOfEnumDclLoc);
    commit.insertFromRange(TypedefDcl->getLocStart(), EnumDclRange);
  }
  else
    return false;
  
  SourceLocation EndTypedefDclLoc = TypedefDcl->getLocEnd();
  EndTypedefDclLoc = trans::findSemiAfterLocation(EndTypedefDclLoc,
                                                 NS.getASTContext(), /*IsDecl*/true);
  if (!EndTypedefDclLoc.isInvalid()) {
    SourceRange TDRange(TypedefDcl->getLocStart(), EndTypedefDclLoc);
    commit.remove(TDRange);
  }
  else
    return false;

  EndOfEnumDclLoc = trans::findLocationAfterSemi(EnumDcl->getLocEnd(), NS.getASTContext(),
                                                 /*IsDecl*/true);
  if (!EndOfEnumDclLoc.isInvalid()) {
    SourceLocation BeginOfEnumDclLoc = EnumDcl->getLocStart();
    // FIXME. This assumes that enum decl; is immediately preceded by eoln.
    // It is trying to remove the enum decl. lines entirely.
    BeginOfEnumDclLoc = BeginOfEnumDclLoc.getLocWithOffset(-1);
    commit.remove(SourceRange(BeginOfEnumDclLoc, EndOfEnumDclLoc));
    return true;
  }
  return false;
}

static void rewriteToNSMacroDecl(const EnumDecl *EnumDcl,
                                const TypedefDecl *TypedefDcl,
                                const NSAPI &NS, edit::Commit &commit,
                                 bool IsNSIntegerType) {
  std::string ClassString =
    IsNSIntegerType ? "NS_ENUM(NSInteger, " : "NS_OPTIONS(NSUInteger, ";
  ClassString += TypedefDcl->getIdentifier()->getName();
  ClassString += ')';
  SourceRange R(EnumDcl->getLocStart(), EnumDcl->getLocStart());
  commit.replace(R, ClassString);
  SourceLocation TypedefLoc = TypedefDcl->getLocEnd();
  commit.remove(SourceRange(TypedefLoc, TypedefLoc));
}

static bool UseNSOptionsMacro(Preprocessor &PP, ASTContext &Ctx,
                              const EnumDecl *EnumDcl) {
  bool PowerOfTwo = true;
  bool AllHexdecimalEnumerator = true;
  uint64_t MaxPowerOfTwoVal = 0;
  for (EnumDecl::enumerator_iterator EI = EnumDcl->enumerator_begin(),
       EE = EnumDcl->enumerator_end(); EI != EE; ++EI) {
    EnumConstantDecl *Enumerator = (*EI);
    const Expr *InitExpr = Enumerator->getInitExpr();
    if (!InitExpr) {
      PowerOfTwo = false;
      AllHexdecimalEnumerator = false;
      continue;
    }
    InitExpr = InitExpr->IgnoreParenCasts();
    if (const BinaryOperator *BO = dyn_cast<BinaryOperator>(InitExpr))
      if (BO->isShiftOp() || BO->isBitwiseOp())
        return true;
    
    uint64_t EnumVal = Enumerator->getInitVal().getZExtValue();
    if (PowerOfTwo && EnumVal) {
      if (!llvm::isPowerOf2_64(EnumVal))
        PowerOfTwo = false;
      else if (EnumVal > MaxPowerOfTwoVal)
        MaxPowerOfTwoVal = EnumVal;
    }
    if (AllHexdecimalEnumerator && EnumVal) {
      bool FoundHexdecimalEnumerator = false;
      SourceLocation EndLoc = Enumerator->getLocEnd();
      Token Tok;
      if (!PP.getRawToken(EndLoc, Tok, /*IgnoreWhiteSpace=*/true))
        if (Tok.isLiteral() && Tok.getLength() > 2) {
          if (const char *StringLit = Tok.getLiteralData())
            FoundHexdecimalEnumerator =
              (StringLit[0] == '0' && (toLowercase(StringLit[1]) == 'x'));
        }
      if (!FoundHexdecimalEnumerator)
        AllHexdecimalEnumerator = false;
    }
  }
  return AllHexdecimalEnumerator || (PowerOfTwo && (MaxPowerOfTwoVal > 2));
}

void ObjCMigrateASTConsumer::migrateProtocolConformance(ASTContext &Ctx,   
                                            const ObjCImplementationDecl *ImpDecl) {
  const ObjCInterfaceDecl *IDecl = ImpDecl->getClassInterface();
  if (!IDecl || ObjCProtocolDecls.empty() || IDecl->isDeprecated())
    return;
  // Find all implicit conforming protocols for this class
  // and make them explicit.
  llvm::SmallPtrSet<ObjCProtocolDecl *, 8> ExplicitProtocols;
  Ctx.CollectInheritedProtocols(IDecl, ExplicitProtocols);
  llvm::SmallVector<ObjCProtocolDecl *, 8> PotentialImplicitProtocols;
  
  for (llvm::SmallPtrSet<ObjCProtocolDecl*, 32>::iterator I =
       ObjCProtocolDecls.begin(),
       E = ObjCProtocolDecls.end(); I != E; ++I)
    if (!ExplicitProtocols.count(*I))
      PotentialImplicitProtocols.push_back(*I);
  
  if (PotentialImplicitProtocols.empty())
    return;

  // go through list of non-optional methods and properties in each protocol
  // in the PotentialImplicitProtocols list. If class implements every one of the
  // methods and properties, then this class conforms to this protocol.
  llvm::SmallVector<ObjCProtocolDecl*, 8> ConformingProtocols;
  for (unsigned i = 0, e = PotentialImplicitProtocols.size(); i != e; i++)
    if (ClassImplementsAllMethodsAndProperties(Ctx, ImpDecl, IDecl,
                                              PotentialImplicitProtocols[i]))
      ConformingProtocols.push_back(PotentialImplicitProtocols[i]);
  
  if (ConformingProtocols.empty())
    return;
  
  // Further reduce number of conforming protocols. If protocol P1 is in the list
  // protocol P2 (P2<P1>), No need to include P1.
  llvm::SmallVector<ObjCProtocolDecl*, 8> MinimalConformingProtocols;
  for (unsigned i = 0, e = ConformingProtocols.size(); i != e; i++) {
    bool DropIt = false;
    ObjCProtocolDecl *TargetPDecl = ConformingProtocols[i];
    for (unsigned i1 = 0, e1 = ConformingProtocols.size(); i1 != e1; i1++) {
      ObjCProtocolDecl *PDecl = ConformingProtocols[i1];
      if (PDecl == TargetPDecl)
        continue;
      if (PDecl->lookupProtocolNamed(
            TargetPDecl->getDeclName().getAsIdentifierInfo())) {
        DropIt = true;
        break;
      }
    }
    if (!DropIt)
      MinimalConformingProtocols.push_back(TargetPDecl);
  }
  if (MinimalConformingProtocols.empty())
    return;
  edit::Commit commit(*Editor);
  rewriteToObjCInterfaceDecl(IDecl, MinimalConformingProtocols,
                             *NSAPIObj, commit);
  Editor->commit(commit);
}

void ObjCMigrateASTConsumer::CacheObjCNSIntegerTypedefed(
                                          const TypedefDecl *TypedefDcl) {
  
  QualType qt = TypedefDcl->getTypeSourceInfo()->getType();
  if (NSAPIObj->isObjCNSIntegerType(qt))
    NSIntegerTypedefed = TypedefDcl;
  else if (NSAPIObj->isObjCNSUIntegerType(qt))
    NSUIntegerTypedefed = TypedefDcl;
}

bool ObjCMigrateASTConsumer::migrateNSEnumDecl(ASTContext &Ctx,
                                           const EnumDecl *EnumDcl,
                                           const TypedefDecl *TypedefDcl) {
  if (!EnumDcl->isCompleteDefinition() || EnumDcl->getIdentifier() ||
      EnumDcl->isDeprecated())
    return false;
  if (!TypedefDcl) {
    if (NSIntegerTypedefed) {
      TypedefDcl = NSIntegerTypedefed;
      NSIntegerTypedefed = 0;
    }
    else if (NSUIntegerTypedefed) {
      TypedefDcl = NSUIntegerTypedefed;
      NSUIntegerTypedefed = 0;
    }
    else
      return false;
    FileID FileIdOfTypedefDcl =
      PP.getSourceManager().getFileID(TypedefDcl->getLocation());
    FileID FileIdOfEnumDcl =
      PP.getSourceManager().getFileID(EnumDcl->getLocation());
    if (FileIdOfTypedefDcl != FileIdOfEnumDcl)
      return false;
  }
  if (TypedefDcl->isDeprecated())
    return false;
  
  QualType qt = TypedefDcl->getTypeSourceInfo()->getType();
  bool IsNSIntegerType = NSAPIObj->isObjCNSIntegerType(qt);
  bool IsNSUIntegerType = !IsNSIntegerType && NSAPIObj->isObjCNSUIntegerType(qt);
  
  if (!IsNSIntegerType && !IsNSUIntegerType) {
    // Also check for typedef enum {...} TD;
    if (const EnumType *EnumTy = qt->getAs<EnumType>()) {
      if (EnumTy->getDecl() == EnumDcl) {
        bool NSOptions = UseNSOptionsMacro(PP, Ctx, EnumDcl);
        if (NSOptions) {
          if (!Ctx.Idents.get("NS_OPTIONS").hasMacroDefinition())
            return false;
        }
        else if (!Ctx.Idents.get("NS_ENUM").hasMacroDefinition())
          return false;
        edit::Commit commit(*Editor);
        rewriteToNSMacroDecl(EnumDcl, TypedefDcl, *NSAPIObj, commit, !NSOptions);
        Editor->commit(commit);
        return true;
      }
    }
    return false;
  }
  
  // We may still use NS_OPTIONS based on what we find in the enumertor list.
  bool NSOptions = UseNSOptionsMacro(PP, Ctx, EnumDcl);
  // NS_ENUM must be available.
  if (IsNSIntegerType && !Ctx.Idents.get("NS_ENUM").hasMacroDefinition())
    return false;
  // NS_OPTIONS must be available.
  if (IsNSUIntegerType && !Ctx.Idents.get("NS_OPTIONS").hasMacroDefinition())
    return false;
  edit::Commit commit(*Editor);
  bool Res = rewriteToNSEnumDecl(EnumDcl, TypedefDcl, *NSAPIObj,
                                 commit, IsNSIntegerType, NSOptions);
  Editor->commit(commit);
  return Res;
}

static void ReplaceWithInstancetype(const ObjCMigrateASTConsumer &ASTC,
                                    ObjCMethodDecl *OM) {
  SourceRange R;
  std::string ClassString;
  if (TypeSourceInfo *TSInfo = OM->getReturnTypeSourceInfo()) {
    TypeLoc TL = TSInfo->getTypeLoc();
    R = SourceRange(TL.getBeginLoc(), TL.getEndLoc());
    ClassString = "instancetype";
  }
  else {
    R = SourceRange(OM->getLocStart(), OM->getLocStart());
    ClassString = OM->isInstanceMethod() ? '-' : '+';
    ClassString += " (instancetype)";
  }
  edit::Commit commit(*ASTC.Editor);
  commit.replace(R, ClassString);
  ASTC.Editor->commit(commit);
}

static void ReplaceWithClasstype(const ObjCMigrateASTConsumer &ASTC,
                                    ObjCMethodDecl *OM) {
  ObjCInterfaceDecl *IDecl = OM->getClassInterface();
  SourceRange R;
  std::string ClassString;
  if (TypeSourceInfo *TSInfo = OM->getReturnTypeSourceInfo()) {
    TypeLoc TL = TSInfo->getTypeLoc();
    R = SourceRange(TL.getBeginLoc(), TL.getEndLoc()); {
      ClassString  = IDecl->getName();
      ClassString += "*";
    }
  }
  else {
    R = SourceRange(OM->getLocStart(), OM->getLocStart());
    ClassString = "+ (";
    ClassString += IDecl->getName(); ClassString += "*)";
  }
  edit::Commit commit(*ASTC.Editor);
  commit.replace(R, ClassString);
  ASTC.Editor->commit(commit);
}

void ObjCMigrateASTConsumer::migrateMethodInstanceType(ASTContext &Ctx,
                                                       ObjCContainerDecl *CDecl,
                                                       ObjCMethodDecl *OM) {
  ObjCInstanceTypeFamily OIT_Family =
    Selector::getInstTypeMethodFamily(OM->getSelector());
  
  std::string ClassName;
  switch (OIT_Family) {
    case OIT_None:
      migrateFactoryMethod(Ctx, CDecl, OM);
      return;
    case OIT_Array:
      ClassName = "NSArray";
      break;
    case OIT_Dictionary:
      ClassName = "NSDictionary";
      break;
    case OIT_Singleton:
      migrateFactoryMethod(Ctx, CDecl, OM, OIT_Singleton);
      return;
    case OIT_Init:
      if (OM->getReturnType()->isObjCIdType())
        ReplaceWithInstancetype(*this, OM);
      return;
    case OIT_ReturnsSelf:
      migrateFactoryMethod(Ctx, CDecl, OM, OIT_ReturnsSelf);
      return;
  }
  if (!OM->getReturnType()->isObjCIdType())
    return;
  
  ObjCInterfaceDecl *IDecl = dyn_cast<ObjCInterfaceDecl>(CDecl);
  if (!IDecl) {
    if (ObjCCategoryDecl *CatDecl = dyn_cast<ObjCCategoryDecl>(CDecl))
      IDecl = CatDecl->getClassInterface();
    else if (ObjCImplDecl *ImpDecl = dyn_cast<ObjCImplDecl>(CDecl))
      IDecl = ImpDecl->getClassInterface();
  }
  if (!IDecl ||
      !IDecl->lookupInheritedClass(&Ctx.Idents.get(ClassName))) {
    migrateFactoryMethod(Ctx, CDecl, OM);
    return;
  }
  ReplaceWithInstancetype(*this, OM);
}

static bool TypeIsInnerPointer(QualType T) {
  if (!T->isAnyPointerType())
    return false;
  if (T->isObjCObjectPointerType() || T->isObjCBuiltinType() ||
      T->isBlockPointerType() || T->isFunctionPointerType() ||
      ento::coreFoundation::isCFObjectRef(T))
    return false;
  // Also, typedef-of-pointer-to-incomplete-struct is something that we assume
  // is not an innter pointer type.
  QualType OrigT = T;
  while (const TypedefType *TD = dyn_cast<TypedefType>(T.getTypePtr()))
    T = TD->getDecl()->getUnderlyingType();
  if (OrigT == T || !T->isPointerType())
    return true;
  const PointerType* PT = T->getAs<PointerType>();
  QualType UPointeeT = PT->getPointeeType().getUnqualifiedType();
  if (UPointeeT->isRecordType()) {
    const RecordType *RecordTy = UPointeeT->getAs<RecordType>();
    if (!RecordTy->getDecl()->isCompleteDefinition())
      return false;
  }
  return true;
}

/// \brief Check whether the two versions match.
static bool versionsMatch(const VersionTuple &X, const VersionTuple &Y) {
  return (X == Y);
}

/// AvailabilityAttrsMatch - This routine checks that if comparing two
/// availability attributes, all their components match. It returns
/// true, if not dealing with availability or when all components of
/// availability attributes match. This routine is only called when
/// the attributes are of the same kind.
static bool AvailabilityAttrsMatch(Attr *At1, Attr *At2) {
  const AvailabilityAttr *AA1 = dyn_cast<AvailabilityAttr>(At1);
  if (!AA1)
    return true;
  const AvailabilityAttr *AA2 = dyn_cast<AvailabilityAttr>(At2);
  
  VersionTuple Introduced1 = AA1->getIntroduced();
  VersionTuple Deprecated1 = AA1->getDeprecated();
  VersionTuple Obsoleted1 = AA1->getObsoleted();
  bool IsUnavailable1 = AA1->getUnavailable();
  VersionTuple Introduced2 = AA2->getIntroduced();
  VersionTuple Deprecated2 = AA2->getDeprecated();
  VersionTuple Obsoleted2 = AA2->getObsoleted();
  bool IsUnavailable2 = AA2->getUnavailable();
  return (versionsMatch(Introduced1, Introduced2) &&
          versionsMatch(Deprecated1, Deprecated2) &&
          versionsMatch(Obsoleted1, Obsoleted2) &&
          IsUnavailable1 == IsUnavailable2);
  
}

static bool MatchTwoAttributeLists(const AttrVec &Attrs1, const AttrVec &Attrs2,
                                   bool &AvailabilityArgsMatch) {
  // This list is very small, so this need not be optimized.
  for (unsigned i = 0, e = Attrs1.size(); i != e; i++) {
    bool match = false;
    for (unsigned j = 0, f = Attrs2.size(); j != f; j++) {
      // Matching attribute kind only. Except for Availabilty attributes,
      // we are not getting into details of the attributes. For all practical purposes
      // this is sufficient.
      if (Attrs1[i]->getKind() == Attrs2[j]->getKind()) {
        if (AvailabilityArgsMatch)
          AvailabilityArgsMatch = AvailabilityAttrsMatch(Attrs1[i], Attrs2[j]);
        match = true;
        break;
      }
    }
    if (!match)
      return false;
  }
  return true;
}

/// AttributesMatch - This routine checks list of attributes for two
/// decls. It returns false, if there is a mismatch in kind of
/// attributes seen in the decls. It returns true if the two decls
/// have list of same kind of attributes. Furthermore, when there
/// are availability attributes in the two decls, it sets the
/// AvailabilityArgsMatch to false if availability attributes have
/// different versions, etc.
static bool AttributesMatch(const Decl *Decl1, const Decl *Decl2,
                            bool &AvailabilityArgsMatch) {
  if (!Decl1->hasAttrs() || !Decl2->hasAttrs()) {
    AvailabilityArgsMatch = (Decl1->hasAttrs() == Decl2->hasAttrs());
    return true;
  }
  AvailabilityArgsMatch = true;
  const AttrVec &Attrs1 = Decl1->getAttrs();
  const AttrVec &Attrs2 = Decl2->getAttrs();
  bool match = MatchTwoAttributeLists(Attrs1, Attrs2, AvailabilityArgsMatch);
  if (match && (Attrs2.size() > Attrs1.size()))
    return MatchTwoAttributeLists(Attrs2, Attrs1, AvailabilityArgsMatch);
  return match;
}

static bool IsValidIdentifier(ASTContext &Ctx,
                              const char *Name) {
  if (!isIdentifierHead(Name[0]))
    return false;
  std::string NameString = Name;
  NameString[0] = toLowercase(NameString[0]);
  IdentifierInfo *II = &Ctx.Idents.get(NameString);
  return II->getTokenID() ==  tok::identifier;
}

bool ObjCMigrateASTConsumer::migrateProperty(ASTContext &Ctx,
                             ObjCContainerDecl *D,
                             ObjCMethodDecl *Method) {
  if (Method->isPropertyAccessor() || !Method->isInstanceMethod() ||
      Method->param_size() != 0)
    return false;
  // Is this method candidate to be a getter?
  QualType GRT = Method->getReturnType();
  if (GRT->isVoidType())
    return false;
  
  Selector GetterSelector = Method->getSelector();
  ObjCInstanceTypeFamily OIT_Family =
    Selector::getInstTypeMethodFamily(GetterSelector);
  
  if (OIT_Family != OIT_None)
    return false;
  
  IdentifierInfo *getterName = GetterSelector.getIdentifierInfoForSlot(0);
  Selector SetterSelector =
  SelectorTable::constructSetterSelector(PP.getIdentifierTable(),
                                         PP.getSelectorTable(),
                                         getterName);
  ObjCMethodDecl *SetterMethod = D->getInstanceMethod(SetterSelector);
  unsigned LengthOfPrefix = 0;
  if (!SetterMethod) {
    // try a different naming convention for getter: isXxxxx
    StringRef getterNameString = getterName->getName();
    bool IsPrefix = getterNameString.startswith("is");
    // Note that we don't want to change an isXXX method of retainable object
    // type to property (readonly or otherwise).
    if (IsPrefix && GRT->isObjCRetainableType())
      return false;
    if (IsPrefix || getterNameString.startswith("get")) {
      LengthOfPrefix = (IsPrefix ? 2 : 3);
      const char *CGetterName = getterNameString.data() + LengthOfPrefix;
      // Make sure that first character after "is" or "get" prefix can
      // start an identifier.
      if (!IsValidIdentifier(Ctx, CGetterName))
        return false;
      if (CGetterName[0] && isUppercase(CGetterName[0])) {
        getterName = &Ctx.Idents.get(CGetterName);
        SetterSelector =
        SelectorTable::constructSetterSelector(PP.getIdentifierTable(),
                                               PP.getSelectorTable(),
                                               getterName);
        SetterMethod = D->getInstanceMethod(SetterSelector);
      }
    }
  }
  
  if (SetterMethod) {
    if ((ASTMigrateActions & FrontendOptions::ObjCMT_ReadwriteProperty) == 0)
      return false;
    bool AvailabilityArgsMatch;
    if (SetterMethod->isDeprecated() ||
        !AttributesMatch(Method, SetterMethod, AvailabilityArgsMatch))
      return false;
    
    // Is this a valid setter, matching the target getter?
    QualType SRT = SetterMethod->getReturnType();
    if (!SRT->isVoidType())
      return false;
    const ParmVarDecl *argDecl = *SetterMethod->param_begin();
    QualType ArgType = argDecl->getType();
    if (!Ctx.hasSameUnqualifiedType(ArgType, GRT))
      return false;
    edit::Commit commit(*Editor);
    rewriteToObjCProperty(Method, SetterMethod, *NSAPIObj, commit,
                          LengthOfPrefix,
                          (ASTMigrateActions &
                           FrontendOptions::ObjCMT_AtomicProperty) != 0,
                          (ASTMigrateActions &
                           FrontendOptions::ObjCMT_NsAtomicIOSOnlyProperty) != 0,
                          AvailabilityArgsMatch);
    Editor->commit(commit);
    return true;
  }
  else if (ASTMigrateActions & FrontendOptions::ObjCMT_ReadonlyProperty) {
    // Try a non-void method with no argument (and no setter or property of same name
    // as a 'readonly' property.
    edit::Commit commit(*Editor);
    rewriteToObjCProperty(Method, 0 /*SetterMethod*/, *NSAPIObj, commit,
                          LengthOfPrefix,
                          (ASTMigrateActions &
                           FrontendOptions::ObjCMT_AtomicProperty) != 0,
                          (ASTMigrateActions &
                           FrontendOptions::ObjCMT_NsAtomicIOSOnlyProperty) != 0,
                          /*AvailabilityArgsMatch*/false);
    Editor->commit(commit);
    return true;
  }
  return false;
}

void ObjCMigrateASTConsumer::migrateNsReturnsInnerPointer(ASTContext &Ctx,
                                                          ObjCMethodDecl *OM) {
  if (OM->isImplicit() ||
      !OM->isInstanceMethod() ||
      OM->hasAttr<ObjCReturnsInnerPointerAttr>())
    return;

  QualType RT = OM->getReturnType();
  if (!TypeIsInnerPointer(RT) ||
      !Ctx.Idents.get("NS_RETURNS_INNER_POINTER").hasMacroDefinition())
    return;
  
  edit::Commit commit(*Editor);
  commit.insertBefore(OM->getLocEnd(), " NS_RETURNS_INNER_POINTER");
  Editor->commit(commit);
}

void ObjCMigrateASTConsumer::migratePropertyNsReturnsInnerPointer(ASTContext &Ctx,
                                                                  ObjCPropertyDecl *P) {
  QualType T = P->getType();
  
  if (!TypeIsInnerPointer(T) ||
      !Ctx.Idents.get("NS_RETURNS_INNER_POINTER").hasMacroDefinition())
    return;
  edit::Commit commit(*Editor);
  commit.insertBefore(P->getLocEnd(), " NS_RETURNS_INNER_POINTER ");
  Editor->commit(commit);
}

void ObjCMigrateASTConsumer::migrateAllMethodInstaceType(ASTContext &Ctx,
                                                 ObjCContainerDecl *CDecl) {
  if (CDecl->isDeprecated() || IsCategoryNameWithDeprecatedSuffix(CDecl))
    return;
  
  // migrate methods which can have instancetype as their result type.
  for (ObjCContainerDecl::method_iterator M = CDecl->meth_begin(),
       MEnd = CDecl->meth_end();
       M != MEnd; ++M) {
    ObjCMethodDecl *Method = (*M);
    if (Method->isDeprecated())
      continue;
    migrateMethodInstanceType(Ctx, CDecl, Method);
  }
}

void ObjCMigrateASTConsumer::migrateFactoryMethod(ASTContext &Ctx,
                                                  ObjCContainerDecl *CDecl,
                                                  ObjCMethodDecl *OM,
                                                  ObjCInstanceTypeFamily OIT_Family) {
  if (OM->isInstanceMethod() ||
      OM->getReturnType() == Ctx.getObjCInstanceType() ||
      !OM->getReturnType()->isObjCIdType())
    return;
  
  // Candidate factory methods are + (id) NaMeXXX : ... which belong to a class
  // NSYYYNamE with matching names be at least 3 characters long.
  ObjCInterfaceDecl *IDecl = dyn_cast<ObjCInterfaceDecl>(CDecl);
  if (!IDecl) {
    if (ObjCCategoryDecl *CatDecl = dyn_cast<ObjCCategoryDecl>(CDecl))
      IDecl = CatDecl->getClassInterface();
    else if (ObjCImplDecl *ImpDecl = dyn_cast<ObjCImplDecl>(CDecl))
      IDecl = ImpDecl->getClassInterface();
  }
  if (!IDecl)
    return;
  
  std::string StringClassName = IDecl->getName();
  StringRef LoweredClassName(StringClassName);
  std::string StringLoweredClassName = LoweredClassName.lower();
  LoweredClassName = StringLoweredClassName;
  
  IdentifierInfo *MethodIdName = OM->getSelector().getIdentifierInfoForSlot(0);
  // Handle method with no name at its first selector slot; e.g. + (id):(int)x.
  if (!MethodIdName)
    return;
  
  std::string MethodName = MethodIdName->getName();
  if (OIT_Family == OIT_Singleton || OIT_Family == OIT_ReturnsSelf) {
    StringRef STRefMethodName(MethodName);
    size_t len = 0;
    if (STRefMethodName.startswith("standard"))
      len = strlen("standard");
    else if (STRefMethodName.startswith("shared"))
      len = strlen("shared");
    else if (STRefMethodName.startswith("default"))
      len = strlen("default");
    else
      return;
    MethodName = STRefMethodName.substr(len);
  }
  std::string MethodNameSubStr = MethodName.substr(0, 3);
  StringRef MethodNamePrefix(MethodNameSubStr);
  std::string StringLoweredMethodNamePrefix = MethodNamePrefix.lower();
  MethodNamePrefix = StringLoweredMethodNamePrefix;
  size_t Ix = LoweredClassName.rfind(MethodNamePrefix);
  if (Ix == StringRef::npos)
    return;
  std::string ClassNamePostfix = LoweredClassName.substr(Ix);
  StringRef LoweredMethodName(MethodName);
  std::string StringLoweredMethodName = LoweredMethodName.lower();
  LoweredMethodName = StringLoweredMethodName;
  if (!LoweredMethodName.startswith(ClassNamePostfix))
    return;
  if (OIT_Family == OIT_ReturnsSelf)
    ReplaceWithClasstype(*this, OM);
  else
    ReplaceWithInstancetype(*this, OM);
}

static bool IsVoidStarType(QualType Ty) {
  if (!Ty->isPointerType())
    return false;
  
  while (const TypedefType *TD = dyn_cast<TypedefType>(Ty.getTypePtr()))
    Ty = TD->getDecl()->getUnderlyingType();
  
  // Is the type void*?
  const PointerType* PT = Ty->getAs<PointerType>();
  if (PT->getPointeeType().getUnqualifiedType()->isVoidType())
    return true;
  return IsVoidStarType(PT->getPointeeType());
}

/// AuditedType - This routine audits the type AT and returns false if it is one of known
/// CF object types or of the "void *" variety. It returns true if we don't care about the type
/// such as a non-pointer or pointers which have no ownership issues (such as "int *").
static bool AuditedType (QualType AT) {
  if (!AT->isAnyPointerType() && !AT->isBlockPointerType())
    return true;
  // FIXME. There isn't much we can say about CF pointer type; or is there?
  if (ento::coreFoundation::isCFObjectRef(AT) ||
      IsVoidStarType(AT) ||
      // If an ObjC object is type, assuming that it is not a CF function and
      // that it is an un-audited function.
      AT->isObjCObjectPointerType() || AT->isObjCBuiltinType())
    return false;
  // All other pointers are assumed audited as harmless.
  return true;
}

void ObjCMigrateASTConsumer::AnnotateImplicitBridging(ASTContext &Ctx) {
  if (CFFunctionIBCandidates.empty())
    return;
  if (!Ctx.Idents.get("CF_IMPLICIT_BRIDGING_ENABLED").hasMacroDefinition()) {
    CFFunctionIBCandidates.clear();
    FileId = FileID();
    return;
  }
  // Insert CF_IMPLICIT_BRIDGING_ENABLE/CF_IMPLICIT_BRIDGING_DISABLED
  const Decl *FirstFD = CFFunctionIBCandidates[0];
  const Decl *LastFD  =
    CFFunctionIBCandidates[CFFunctionIBCandidates.size()-1];
  const char *PragmaString = "\nCF_IMPLICIT_BRIDGING_ENABLED\n\n";
  edit::Commit commit(*Editor);
  commit.insertBefore(FirstFD->getLocStart(), PragmaString);
  PragmaString = "\n\nCF_IMPLICIT_BRIDGING_DISABLED\n";
  SourceLocation EndLoc = LastFD->getLocEnd();
  // get location just past end of function location.
  EndLoc = PP.getLocForEndOfToken(EndLoc);
  if (isa<FunctionDecl>(LastFD)) {
    // For Methods, EndLoc points to the ending semcolon. So,
    // not of these extra work is needed.
    Token Tok;
    // get locaiton of token that comes after end of function.
    bool Failed = PP.getRawToken(EndLoc, Tok, /*IgnoreWhiteSpace=*/true);
    if (!Failed)
      EndLoc = Tok.getLocation();
  }
  commit.insertAfterToken(EndLoc, PragmaString);
  Editor->commit(commit);
  FileId = FileID();
  CFFunctionIBCandidates.clear();
}

void ObjCMigrateASTConsumer::migrateCFAnnotation(ASTContext &Ctx, const Decl *Decl) {
  if (Decl->isDeprecated())
    return;
  
  if (Decl->hasAttr<CFAuditedTransferAttr>()) {
    assert(CFFunctionIBCandidates.empty() &&
           "Cannot have audited functions/methods inside user "
           "provided CF_IMPLICIT_BRIDGING_ENABLE");
    return;
  }
  
  // Finction must be annotated first.
  if (const FunctionDecl *FuncDecl = dyn_cast<FunctionDecl>(Decl)) {
    CF_BRIDGING_KIND AuditKind = migrateAddFunctionAnnotation(Ctx, FuncDecl);
    if (AuditKind == CF_BRIDGING_ENABLE) {
      CFFunctionIBCandidates.push_back(Decl);
      if (FileId.isInvalid())
        FileId = PP.getSourceManager().getFileID(Decl->getLocation());
    }
    else if (AuditKind == CF_BRIDGING_MAY_INCLUDE) {
      if (!CFFunctionIBCandidates.empty()) {
        CFFunctionIBCandidates.push_back(Decl);
        if (FileId.isInvalid())
          FileId = PP.getSourceManager().getFileID(Decl->getLocation());
      }
    }
    else
      AnnotateImplicitBridging(Ctx);
  }
  else {
    migrateAddMethodAnnotation(Ctx, cast<ObjCMethodDecl>(Decl));
    AnnotateImplicitBridging(Ctx);
  }
}

void ObjCMigrateASTConsumer::AddCFAnnotations(ASTContext &Ctx,
                                              const CallEffects &CE,
                                              const FunctionDecl *FuncDecl,
                                              bool ResultAnnotated) {
  // Annotate function.
  if (!ResultAnnotated) {
    RetEffect Ret = CE.getReturnValue();
    const char *AnnotationString = 0;
    if (Ret.getObjKind() == RetEffect::CF) {
      if (Ret.isOwned() &&
          Ctx.Idents.get("CF_RETURNS_RETAINED").hasMacroDefinition())
        AnnotationString = " CF_RETURNS_RETAINED";
      else if (Ret.notOwned() &&
               Ctx.Idents.get("CF_RETURNS_NOT_RETAINED").hasMacroDefinition())
        AnnotationString = " CF_RETURNS_NOT_RETAINED";
    }
    else if (Ret.getObjKind() == RetEffect::ObjC) {
      if (Ret.isOwned() &&
          Ctx.Idents.get("NS_RETURNS_RETAINED").hasMacroDefinition())
        AnnotationString = " NS_RETURNS_RETAINED";
    }
    
    if (AnnotationString) {
      edit::Commit commit(*Editor);
      commit.insertAfterToken(FuncDecl->getLocEnd(), AnnotationString);
      Editor->commit(commit);
    }
  }
  llvm::ArrayRef<ArgEffect> AEArgs = CE.getArgs();
  unsigned i = 0;
  for (FunctionDecl::param_const_iterator pi = FuncDecl->param_begin(),
       pe = FuncDecl->param_end(); pi != pe; ++pi, ++i) {
    const ParmVarDecl *pd = *pi;
    ArgEffect AE = AEArgs[i];
    if (AE == DecRef && !pd->hasAttr<CFConsumedAttr>() &&
        Ctx.Idents.get("CF_CONSUMED").hasMacroDefinition()) {
      edit::Commit commit(*Editor);
      commit.insertBefore(pd->getLocation(), "CF_CONSUMED ");
      Editor->commit(commit);
    }
    else if (AE == DecRefMsg && !pd->hasAttr<NSConsumedAttr>() &&
             Ctx.Idents.get("NS_CONSUMED").hasMacroDefinition()) {
      edit::Commit commit(*Editor);
      commit.insertBefore(pd->getLocation(), "NS_CONSUMED ");
      Editor->commit(commit);
    }
  }
}


ObjCMigrateASTConsumer::CF_BRIDGING_KIND
  ObjCMigrateASTConsumer::migrateAddFunctionAnnotation(
                                                  ASTContext &Ctx,
                                                  const FunctionDecl *FuncDecl) {
  if (FuncDecl->hasBody())
    return CF_BRIDGING_NONE;
    
  CallEffects CE  = CallEffects::getEffect(FuncDecl);
  bool FuncIsReturnAnnotated = (FuncDecl->hasAttr<CFReturnsRetainedAttr>() ||
                                FuncDecl->hasAttr<CFReturnsNotRetainedAttr>() ||
                                FuncDecl->hasAttr<NSReturnsRetainedAttr>() ||
                                FuncDecl->hasAttr<NSReturnsNotRetainedAttr>() ||
                                FuncDecl->hasAttr<NSReturnsAutoreleasedAttr>());
  
  // Trivial case of when funciton is annotated and has no argument.
  if (FuncIsReturnAnnotated && FuncDecl->getNumParams() == 0)
    return CF_BRIDGING_NONE;
  
  bool ReturnCFAudited = false;
  if (!FuncIsReturnAnnotated) {
    RetEffect Ret = CE.getReturnValue();
    if (Ret.getObjKind() == RetEffect::CF &&
        (Ret.isOwned() || Ret.notOwned()))
      ReturnCFAudited = true;
    else if (!AuditedType(FuncDecl->getReturnType()))
      return CF_BRIDGING_NONE;
  }
  
  // At this point result type is audited for potential inclusion.
  // Now, how about argument types.
  llvm::ArrayRef<ArgEffect> AEArgs = CE.getArgs();
  unsigned i = 0;
  bool ArgCFAudited = false;
  for (FunctionDecl::param_const_iterator pi = FuncDecl->param_begin(),
       pe = FuncDecl->param_end(); pi != pe; ++pi, ++i) {
    const ParmVarDecl *pd = *pi;
    ArgEffect AE = AEArgs[i];
    if (AE == DecRef /*CFConsumed annotated*/ || AE == IncRef) {
      if (AE == DecRef && !pd->hasAttr<CFConsumedAttr>())
        ArgCFAudited = true;
      else if (AE == IncRef)
        ArgCFAudited = true;
    }
    else {
      QualType AT = pd->getType();
      if (!AuditedType(AT)) {
        AddCFAnnotations(Ctx, CE, FuncDecl, FuncIsReturnAnnotated);
        return CF_BRIDGING_NONE;
      }
    }
  }
  if (ReturnCFAudited || ArgCFAudited)
    return CF_BRIDGING_ENABLE;
  
  return CF_BRIDGING_MAY_INCLUDE;
}

void ObjCMigrateASTConsumer::migrateARCSafeAnnotation(ASTContext &Ctx,
                                                 ObjCContainerDecl *CDecl) {
  if (!isa<ObjCInterfaceDecl>(CDecl) || CDecl->isDeprecated())
    return;
  
  // migrate methods which can have instancetype as their result type.
  for (ObjCContainerDecl::method_iterator M = CDecl->meth_begin(),
       MEnd = CDecl->meth_end();
       M != MEnd; ++M) {
    ObjCMethodDecl *Method = (*M);
    migrateCFAnnotation(Ctx, Method);
  }
}

void ObjCMigrateASTConsumer::AddCFAnnotations(ASTContext &Ctx,
                                              const CallEffects &CE,
                                              const ObjCMethodDecl *MethodDecl,
                                              bool ResultAnnotated) {
  // Annotate function.
  if (!ResultAnnotated) {
    RetEffect Ret = CE.getReturnValue();
    const char *AnnotationString = 0;
    if (Ret.getObjKind() == RetEffect::CF) {
      if (Ret.isOwned() &&
          Ctx.Idents.get("CF_RETURNS_RETAINED").hasMacroDefinition())
        AnnotationString = " CF_RETURNS_RETAINED";
      else if (Ret.notOwned() &&
               Ctx.Idents.get("CF_RETURNS_NOT_RETAINED").hasMacroDefinition())
        AnnotationString = " CF_RETURNS_NOT_RETAINED";
    }
    else if (Ret.getObjKind() == RetEffect::ObjC) {
      ObjCMethodFamily OMF = MethodDecl->getMethodFamily();
      switch (OMF) {
        case clang::OMF_alloc:
        case clang::OMF_new:
        case clang::OMF_copy:
        case clang::OMF_init:
        case clang::OMF_mutableCopy:
          break;
          
        default:
          if (Ret.isOwned() &&
              Ctx.Idents.get("NS_RETURNS_RETAINED").hasMacroDefinition())
            AnnotationString = " NS_RETURNS_RETAINED";
          break;
      }
    }
    
    if (AnnotationString) {
      edit::Commit commit(*Editor);
      commit.insertBefore(MethodDecl->getLocEnd(), AnnotationString);
      Editor->commit(commit);
    }
  }
  llvm::ArrayRef<ArgEffect> AEArgs = CE.getArgs();
  unsigned i = 0;
  for (ObjCMethodDecl::param_const_iterator pi = MethodDecl->param_begin(),
       pe = MethodDecl->param_end(); pi != pe; ++pi, ++i) {
    const ParmVarDecl *pd = *pi;
    ArgEffect AE = AEArgs[i];
    if (AE == DecRef && !pd->hasAttr<CFConsumedAttr>() &&
        Ctx.Idents.get("CF_CONSUMED").hasMacroDefinition()) {
      edit::Commit commit(*Editor);
      commit.insertBefore(pd->getLocation(), "CF_CONSUMED ");
      Editor->commit(commit);
    }
  }
}

void ObjCMigrateASTConsumer::migrateAddMethodAnnotation(
                                            ASTContext &Ctx,
                                            const ObjCMethodDecl *MethodDecl) {
  if (MethodDecl->hasBody() || MethodDecl->isImplicit())
    return;
  
  CallEffects CE  = CallEffects::getEffect(MethodDecl);
  bool MethodIsReturnAnnotated = (MethodDecl->hasAttr<CFReturnsRetainedAttr>() ||
                                  MethodDecl->hasAttr<CFReturnsNotRetainedAttr>() ||
                                  MethodDecl->hasAttr<NSReturnsRetainedAttr>() ||
                                  MethodDecl->hasAttr<NSReturnsNotRetainedAttr>() ||
                                  MethodDecl->hasAttr<NSReturnsAutoreleasedAttr>());
  
  if (CE.getReceiver() ==  DecRefMsg &&
      !MethodDecl->hasAttr<NSConsumesSelfAttr>() &&
      MethodDecl->getMethodFamily() != OMF_init &&
      MethodDecl->getMethodFamily() != OMF_release &&
      Ctx.Idents.get("NS_CONSUMES_SELF").hasMacroDefinition()) {
    edit::Commit commit(*Editor);
    commit.insertBefore(MethodDecl->getLocEnd(), " NS_CONSUMES_SELF");
    Editor->commit(commit);
  }
  
  // Trivial case of when funciton is annotated and has no argument.
  if (MethodIsReturnAnnotated &&
      (MethodDecl->param_begin() == MethodDecl->param_end()))
    return;
  
  if (!MethodIsReturnAnnotated) {
    RetEffect Ret = CE.getReturnValue();
    if ((Ret.getObjKind() == RetEffect::CF ||
         Ret.getObjKind() == RetEffect::ObjC) &&
        (Ret.isOwned() || Ret.notOwned())) {
      AddCFAnnotations(Ctx, CE, MethodDecl, false);
      return;
    } else if (!AuditedType(MethodDecl->getReturnType()))
      return;
  }
  
  // At this point result type is either annotated or audited.
  // Now, how about argument types.
  llvm::ArrayRef<ArgEffect> AEArgs = CE.getArgs();
  unsigned i = 0;
  for (ObjCMethodDecl::param_const_iterator pi = MethodDecl->param_begin(),
       pe = MethodDecl->param_end(); pi != pe; ++pi, ++i) {
    const ParmVarDecl *pd = *pi;
    ArgEffect AE = AEArgs[i];
    if ((AE == DecRef && !pd->hasAttr<CFConsumedAttr>()) || AE == IncRef ||
        !AuditedType(pd->getType())) {
      AddCFAnnotations(Ctx, CE, MethodDecl, MethodIsReturnAnnotated);
      return;
    }
  }
  return;
}

namespace {
class SuperInitChecker : public RecursiveASTVisitor<SuperInitChecker> {
public:
  bool shouldVisitTemplateInstantiations() const { return false; }
  bool shouldWalkTypesOfTypeLocs() const { return false; }

  bool VisitObjCMessageExpr(ObjCMessageExpr *E) {
    if (E->getReceiverKind() == ObjCMessageExpr::SuperInstance) {
      if (E->getMethodFamily() == OMF_init)
        return false;
    }
    return true;
  }
};
} // anonymous namespace

static bool hasSuperInitCall(const ObjCMethodDecl *MD) {
  return !SuperInitChecker().TraverseStmt(MD->getBody());
}

void ObjCMigrateASTConsumer::inferDesignatedInitializers(
    ASTContext &Ctx,
    const ObjCImplementationDecl *ImplD) {

  const ObjCInterfaceDecl *IFace = ImplD->getClassInterface();
  if (!IFace || IFace->hasDesignatedInitializers())
    return;
  if (!Ctx.Idents.get("NS_DESIGNATED_INITIALIZER").hasMacroDefinition())
    return;

  for (ObjCImplementationDecl::instmeth_iterator
         I = ImplD->instmeth_begin(), E = ImplD->instmeth_end(); I != E; ++I) {
    const ObjCMethodDecl *MD = *I;
    if (MD->isDeprecated() ||
        MD->getMethodFamily() != OMF_init ||
        MD->isDesignatedInitializerForTheInterface())
      continue;
    const ObjCMethodDecl *IFaceM = IFace->getMethod(MD->getSelector(),
                                                    /*isInstance=*/true);
    if (!IFaceM)
      continue;
    if (hasSuperInitCall(MD)) {
      edit::Commit commit(*Editor);
      commit.insert(IFaceM->getLocEnd(), " NS_DESIGNATED_INITIALIZER");
      Editor->commit(commit);
    }
  }
}

namespace {

class RewritesReceiver : public edit::EditsReceiver {
  Rewriter &Rewrite;

public:
  RewritesReceiver(Rewriter &Rewrite) : Rewrite(Rewrite) { }

  virtual void insert(SourceLocation loc, StringRef text) {
    Rewrite.InsertText(loc, text);
  }
  virtual void replace(CharSourceRange range, StringRef text) {
    Rewrite.ReplaceText(range.getBegin(), Rewrite.getRangeSize(range), text);
  }
};

class JSONEditWriter : public edit::EditsReceiver {
  SourceManager &SourceMgr;
  llvm::raw_ostream &OS;

public:
  JSONEditWriter(SourceManager &SM, llvm::raw_ostream &OS)
    : SourceMgr(SM), OS(OS) {
    OS << "[\n";
  }
  ~JSONEditWriter() {
    OS << "]\n";
  }

private:
  struct EntryWriter {
    SourceManager &SourceMgr;
    llvm::raw_ostream &OS;

    EntryWriter(SourceManager &SM, llvm::raw_ostream &OS)
      : SourceMgr(SM), OS(OS) {
      OS << " {\n";
    }
    ~EntryWriter() {
      OS << " },\n";
    }

    void writeLoc(SourceLocation Loc) {
      FileID FID;
      unsigned Offset;
      llvm::tie(FID, Offset) = SourceMgr.getDecomposedLoc(Loc);
      assert(!FID.isInvalid());
      SmallString<200> Path =
          StringRef(SourceMgr.getFileEntryForID(FID)->getName());
      llvm::sys::fs::make_absolute(Path);
      OS << "  \"file\": \"";
      OS.write_escaped(Path.str()) << "\",\n";
      OS << "  \"offset\": " << Offset << ",\n";
    }

    void writeRemove(CharSourceRange Range) {
      assert(Range.isCharRange());
      std::pair<FileID, unsigned> Begin =
          SourceMgr.getDecomposedLoc(Range.getBegin());
      std::pair<FileID, unsigned> End =
          SourceMgr.getDecomposedLoc(Range.getEnd());
      assert(Begin.first == End.first);
      assert(Begin.second <= End.second);
      unsigned Length = End.second - Begin.second;

      OS << "  \"remove\": " << Length << ",\n";
    }

    void writeText(StringRef Text) {
      OS << "  \"text\": \"";
      OS.write_escaped(Text) << "\",\n";
    }
  };
 
  virtual void insert(SourceLocation Loc, StringRef Text) {
    EntryWriter Writer(SourceMgr, OS);
    Writer.writeLoc(Loc);
    Writer.writeText(Text);
  }

  virtual void replace(CharSourceRange Range, StringRef Text) {
    EntryWriter Writer(SourceMgr, OS);
    Writer.writeLoc(Range.getBegin());
    Writer.writeRemove(Range);
    Writer.writeText(Text);
  }

  virtual void remove(CharSourceRange Range) {
    EntryWriter Writer(SourceMgr, OS);
    Writer.writeLoc(Range.getBegin());
    Writer.writeRemove(Range);
  }
};

}

static bool
IsReallyASystemHeader(ASTContext &Ctx, const FileEntry *file, FileID FID) {
  bool Invalid = false;
  const SrcMgr::SLocEntry &SEntry =
  Ctx.getSourceManager().getSLocEntry(FID, &Invalid);
  if (!Invalid && SEntry.isFile()) {
    const SrcMgr::FileInfo &FI = SEntry.getFile();
    if (!FI.hasLineDirectives()) {
      if (FI.getFileCharacteristic() == SrcMgr::C_ExternCSystem)
        return true;
      if (FI.getFileCharacteristic() == SrcMgr::C_System) {
        // This file is in a system header directory. Continue committing
        // change only if it's a user-specified system directory because user
        // put a .system_framework file in the framework directory.
        StringRef Directory(file->getDir()->getName());
        size_t Ix = Directory.rfind(".framework");
        if (Ix == StringRef::npos)
          return true;
        std::string PatchToSystemFramework = Directory.slice(0, Ix+sizeof(".framework"));
        PatchToSystemFramework += ".system_framework";
        if (!llvm::sys::fs::exists(PatchToSystemFramework.data()))
          return true;
      }
    }
  }
  return false;
}

void ObjCMigrateASTConsumer::HandleTranslationUnit(ASTContext &Ctx) {
  
  TranslationUnitDecl *TU = Ctx.getTranslationUnitDecl();
  if (ASTMigrateActions & FrontendOptions::ObjCMT_MigrateDecls) {
    for (DeclContext::decl_iterator D = TU->decls_begin(), DEnd = TU->decls_end();
         D != DEnd; ++D) {
      FileID FID = PP.getSourceManager().getFileID((*D)->getLocation());
      if (!FID.isInvalid())
        if (!FileId.isInvalid() && FileId != FID) {
          if (ASTMigrateActions & FrontendOptions::ObjCMT_Annotation)
            AnnotateImplicitBridging(Ctx);
        }
      
      if (ObjCInterfaceDecl *CDecl = dyn_cast<ObjCInterfaceDecl>(*D))
        if (canModify(CDecl))
          migrateObjCInterfaceDecl(Ctx, CDecl);
      if (ObjCCategoryDecl *CatDecl = dyn_cast<ObjCCategoryDecl>(*D)) {
        if (canModify(CatDecl))
          migrateObjCInterfaceDecl(Ctx, CatDecl);
      }
      else if (ObjCProtocolDecl *PDecl = dyn_cast<ObjCProtocolDecl>(*D))
        ObjCProtocolDecls.insert(PDecl->getCanonicalDecl());
      else if (const ObjCImplementationDecl *ImpDecl =
               dyn_cast<ObjCImplementationDecl>(*D)) {
        if ((ASTMigrateActions & FrontendOptions::ObjCMT_ProtocolConformance) &&
            canModify(ImpDecl))
          migrateProtocolConformance(Ctx, ImpDecl);
      }
      else if (const EnumDecl *ED = dyn_cast<EnumDecl>(*D)) {
        if (!(ASTMigrateActions & FrontendOptions::ObjCMT_NsMacros))
          continue;
        if (!canModify(ED))
          continue;
        DeclContext::decl_iterator N = D;
        if (++N != DEnd) {
          const TypedefDecl *TD = dyn_cast<TypedefDecl>(*N);
          if (migrateNSEnumDecl(Ctx, ED, TD) && TD)
            D++;
        }
        else
          migrateNSEnumDecl(Ctx, ED, /*TypedefDecl */0);
      }
      else if (const TypedefDecl *TD = dyn_cast<TypedefDecl>(*D)) {
        if (!(ASTMigrateActions & FrontendOptions::ObjCMT_NsMacros))
          continue;
        if (!canModify(TD))
          continue;
        DeclContext::decl_iterator N = D;
        if (++N == DEnd)
          continue;
        if (const EnumDecl *ED = dyn_cast<EnumDecl>(*N)) {
          if (++N != DEnd)
            if (const TypedefDecl *TDF = dyn_cast<TypedefDecl>(*N)) {
              // prefer typedef-follows-enum to enum-follows-typedef pattern.
              if (migrateNSEnumDecl(Ctx, ED, TDF)) {
                ++D; ++D;
                CacheObjCNSIntegerTypedefed(TD);
                continue;
              }
            }
          if (migrateNSEnumDecl(Ctx, ED, TD)) {
            ++D;
            continue;
          }
        }
        CacheObjCNSIntegerTypedefed(TD);
      }
      else if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(*D)) {
        if ((ASTMigrateActions & FrontendOptions::ObjCMT_Annotation) &&
            canModify(FD))
          migrateCFAnnotation(Ctx, FD);
      }
      
      if (ObjCContainerDecl *CDecl = dyn_cast<ObjCContainerDecl>(*D)) {
        bool CanModify = canModify(CDecl);
        // migrate methods which can have instancetype as their result type.
        if ((ASTMigrateActions & FrontendOptions::ObjCMT_Instancetype) &&
            CanModify)
          migrateAllMethodInstaceType(Ctx, CDecl);
        // annotate methods with CF annotations.
        if ((ASTMigrateActions & FrontendOptions::ObjCMT_Annotation) &&
            CanModify)
          migrateARCSafeAnnotation(Ctx, CDecl);
      }

      if (const ObjCImplementationDecl *
            ImplD = dyn_cast<ObjCImplementationDecl>(*D)) {
        if ((ASTMigrateActions & FrontendOptions::ObjCMT_DesignatedInitializer) &&
            canModify(ImplD))
          inferDesignatedInitializers(Ctx, ImplD);
      }
    }
    if (ASTMigrateActions & FrontendOptions::ObjCMT_Annotation)
      AnnotateImplicitBridging(Ctx);
  }
  
 if (IsOutputFile) {
   std::string Error;
   llvm::raw_fd_ostream OS(MigrateDir.c_str(), Error, llvm::sys::fs::F_Binary);
    if (!Error.empty()) {
      DiagnosticsEngine &Diags = Ctx.getDiagnostics();
      Diags.Report(Diags.getCustomDiagID(DiagnosticsEngine::Error, "%0"))
          << Error;
      return;
    }

   JSONEditWriter Writer(Ctx.getSourceManager(), OS);
   Editor->applyRewrites(Writer);
   return;
 }

  Rewriter rewriter(Ctx.getSourceManager(), Ctx.getLangOpts());
  RewritesReceiver Rec(rewriter);
  Editor->applyRewrites(Rec);

  for (Rewriter::buffer_iterator
        I = rewriter.buffer_begin(), E = rewriter.buffer_end(); I != E; ++I) {
    FileID FID = I->first;
    RewriteBuffer &buf = I->second;
    const FileEntry *file = Ctx.getSourceManager().getFileEntryForID(FID);
    assert(file);
    if (IsReallyASystemHeader(Ctx, file, FID))
      continue;
    SmallString<512> newText;
    llvm::raw_svector_ostream vecOS(newText);
    buf.write(vecOS);
    vecOS.flush();
    llvm::MemoryBuffer *memBuf = llvm::MemoryBuffer::getMemBufferCopy(
                   StringRef(newText.data(), newText.size()), file->getName());
    SmallString<64> filePath(file->getName());
    FileMgr.FixupRelativePath(filePath);
    Remapper.remap(filePath.str(), memBuf);
  }

  if (IsOutputFile) {
    Remapper.flushToFile(MigrateDir, Ctx.getDiagnostics());
  } else {
    Remapper.flushToDisk(MigrateDir, Ctx.getDiagnostics());
  }
}

bool MigrateSourceAction::BeginInvocation(CompilerInstance &CI) {
  CI.getDiagnostics().setIgnoreAllWarnings(true);
  return true;
}

static std::vector<std::string> getWhiteListFilenames(StringRef DirPath) {
  using namespace llvm::sys::fs;
  using namespace llvm::sys::path;

  std::vector<std::string> Filenames;
  if (DirPath.empty() || !is_directory(DirPath))
    return Filenames;
  
  llvm::error_code EC;
  directory_iterator DI = directory_iterator(DirPath, EC);
  directory_iterator DE;
  for (; !EC && DI != DE; DI = DI.increment(EC)) {
    if (is_regular_file(DI->path()))
      Filenames.push_back(filename(DI->path()));
  }

  return Filenames;
}

ASTConsumer *MigrateSourceAction::CreateASTConsumer(CompilerInstance &CI,
                                                  StringRef InFile) {
  PPConditionalDirectiveRecord *
    PPRec = new PPConditionalDirectiveRecord(CI.getSourceManager());
  unsigned ObjCMTAction = CI.getFrontendOpts().ObjCMTAction;
  unsigned ObjCMTOpts = ObjCMTAction;
  // These are companion flags, they do not enable transformations.
  ObjCMTOpts &= ~(FrontendOptions::ObjCMT_AtomicProperty |
                  FrontendOptions::ObjCMT_NsAtomicIOSOnlyProperty);
  if (ObjCMTOpts == FrontendOptions::ObjCMT_None) {
    // If no specific option was given, enable literals+subscripting transforms
    // by default.
    ObjCMTAction |= FrontendOptions::ObjCMT_Literals |
                    FrontendOptions::ObjCMT_Subscripting;
  }
  CI.getPreprocessor().addPPCallbacks(PPRec);
  std::vector<std::string> WhiteList =
    getWhiteListFilenames(CI.getFrontendOpts().ObjCMTWhiteListPath);
  return new ObjCMigrateASTConsumer(CI.getFrontendOpts().OutputFile,
                                    ObjCMTAction,
                                    Remapper,
                                    CI.getFileManager(),
                                    PPRec,
                                    CI.getPreprocessor(),
                                    /*isOutputFile=*/true,
                                    WhiteList);
}

namespace {
struct EditEntry {
  const FileEntry *File;
  unsigned Offset;
  unsigned RemoveLen;
  std::string Text;

  EditEntry() : File(), Offset(), RemoveLen() {}
};
}

namespace llvm {
template<> struct DenseMapInfo<EditEntry> {
  static inline EditEntry getEmptyKey() {
    EditEntry Entry;
    Entry.Offset = unsigned(-1);
    return Entry;
  }
  static inline EditEntry getTombstoneKey() {
    EditEntry Entry;
    Entry.Offset = unsigned(-2);
    return Entry;
  }
  static unsigned getHashValue(const EditEntry& Val) {
    llvm::FoldingSetNodeID ID;
    ID.AddPointer(Val.File);
    ID.AddInteger(Val.Offset);
    ID.AddInteger(Val.RemoveLen);
    ID.AddString(Val.Text);
    return ID.ComputeHash();
  }
  static bool isEqual(const EditEntry &LHS, const EditEntry &RHS) {
    return LHS.File == RHS.File &&
        LHS.Offset == RHS.Offset &&
        LHS.RemoveLen == RHS.RemoveLen &&
        LHS.Text == RHS.Text;
  }
};
}

namespace {
class RemapFileParser {
  FileManager &FileMgr;

public:
  RemapFileParser(FileManager &FileMgr) : FileMgr(FileMgr) { }

  bool parse(StringRef File, SmallVectorImpl<EditEntry> &Entries) {
    using namespace llvm::yaml;

    OwningPtr<llvm::MemoryBuffer> FileBuf;
    if (llvm::MemoryBuffer::getFile(File, FileBuf))
      return true;

    llvm::SourceMgr SM;
    Stream YAMLStream(FileBuf.take(), SM);
    document_iterator I = YAMLStream.begin();
    if (I == YAMLStream.end())
      return true;
    Node *Root = I->getRoot();
    if (!Root)
      return true;

    SequenceNode *SeqNode = dyn_cast<SequenceNode>(Root);
    if (!SeqNode)
      return true;

    for (SequenceNode::iterator
           AI = SeqNode->begin(), AE = SeqNode->end(); AI != AE; ++AI) {
      MappingNode *MapNode = dyn_cast<MappingNode>(&*AI);
      if (!MapNode)
        continue;
      parseEdit(MapNode, Entries);
    }

    return false;
  }

private:
  void parseEdit(llvm::yaml::MappingNode *Node,
                 SmallVectorImpl<EditEntry> &Entries) {
    using namespace llvm::yaml;
    EditEntry Entry;
    bool Ignore = false;

    for (MappingNode::iterator
           KVI = Node->begin(), KVE = Node->end(); KVI != KVE; ++KVI) {
      ScalarNode *KeyString = dyn_cast<ScalarNode>((*KVI).getKey());
      if (!KeyString)
        continue;
      SmallString<10> KeyStorage;
      StringRef Key = KeyString->getValue(KeyStorage);

      ScalarNode *ValueString = dyn_cast<ScalarNode>((*KVI).getValue());
      if (!ValueString)
        continue;
      SmallString<64> ValueStorage;
      StringRef Val = ValueString->getValue(ValueStorage);

      if (Key == "file") {
        const FileEntry *FE = FileMgr.getFile(Val);
        if (!FE)
          Ignore = true;
        Entry.File = FE;
      } else if (Key == "offset") {
        if (Val.getAsInteger(10, Entry.Offset))
          Ignore = true;
      } else if (Key == "remove") {
        if (Val.getAsInteger(10, Entry.RemoveLen))
          Ignore = true;
      } else if (Key == "text") {
        Entry.Text = Val;
      }
    }

    if (!Ignore)
      Entries.push_back(Entry);
  }
};
}

static bool reportDiag(const Twine &Err, DiagnosticsEngine &Diag) {
  Diag.Report(Diag.getCustomDiagID(DiagnosticsEngine::Error, "%0"))
      << Err.str();
  return true;
}

static std::string applyEditsToTemp(const FileEntry *FE,
                                    ArrayRef<EditEntry> Edits,
                                    FileManager &FileMgr,
                                    DiagnosticsEngine &Diag) {
  using namespace llvm::sys;

  SourceManager SM(Diag, FileMgr);
  FileID FID = SM.createFileID(FE, SourceLocation(), SrcMgr::C_User);
  LangOptions LangOpts;
  edit::EditedSource Editor(SM, LangOpts);
  for (ArrayRef<EditEntry>::iterator
        I = Edits.begin(), E = Edits.end(); I != E; ++I) {
    const EditEntry &Entry = *I;
    assert(Entry.File == FE);
    SourceLocation Loc =
        SM.getLocForStartOfFile(FID).getLocWithOffset(Entry.Offset);
    CharSourceRange Range;
    if (Entry.RemoveLen != 0) {
      Range = CharSourceRange::getCharRange(Loc,
                                         Loc.getLocWithOffset(Entry.RemoveLen));
    }

    edit::Commit commit(Editor);
    if (Range.isInvalid()) {
      commit.insert(Loc, Entry.Text);
    } else if (Entry.Text.empty()) {
      commit.remove(Range);
    } else {
      commit.replace(Range, Entry.Text);
    }
    Editor.commit(commit);
  }

  Rewriter rewriter(SM, LangOpts);
  RewritesReceiver Rec(rewriter);
  Editor.applyRewrites(Rec);

  const RewriteBuffer *Buf = rewriter.getRewriteBufferFor(FID);
  SmallString<512> NewText;
  llvm::raw_svector_ostream OS(NewText);
  Buf->write(OS);
  OS.flush();

  SmallString<64> TempPath;
  int FD;
  if (fs::createTemporaryFile(path::filename(FE->getName()),
                              path::extension(FE->getName()), FD,
                              TempPath)) {
    reportDiag("Could not create file: " + TempPath.str(), Diag);
    return std::string();
  }

  llvm::raw_fd_ostream TmpOut(FD, /*shouldClose=*/true);
  TmpOut.write(NewText.data(), NewText.size());
  TmpOut.close();

  return TempPath.str();
}

bool arcmt::getFileRemappingsFromFileList(
                        std::vector<std::pair<std::string,std::string> > &remap,
                        ArrayRef<StringRef> remapFiles,
                        DiagnosticConsumer *DiagClient) {
  bool hasErrorOccurred = false;

  FileSystemOptions FSOpts;
  FileManager FileMgr(FSOpts);
  RemapFileParser Parser(FileMgr);

  IntrusiveRefCntPtr<DiagnosticIDs> DiagID(new DiagnosticIDs());
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags(
      new DiagnosticsEngine(DiagID, new DiagnosticOptions,
                            DiagClient, /*ShouldOwnClient=*/false));

  typedef llvm::DenseMap<const FileEntry *, std::vector<EditEntry> >
      FileEditEntriesTy;
  FileEditEntriesTy FileEditEntries;

  llvm::DenseSet<EditEntry> EntriesSet;

  for (ArrayRef<StringRef>::iterator
         I = remapFiles.begin(), E = remapFiles.end(); I != E; ++I) {
    SmallVector<EditEntry, 16> Entries;
    if (Parser.parse(*I, Entries))
      continue;

    for (SmallVectorImpl<EditEntry>::iterator
           EI = Entries.begin(), EE = Entries.end(); EI != EE; ++EI) {
      EditEntry &Entry = *EI;
      if (!Entry.File)
        continue;
      std::pair<llvm::DenseSet<EditEntry>::iterator, bool>
        Insert = EntriesSet.insert(Entry);
      if (!Insert.second)
        continue;

      FileEditEntries[Entry.File].push_back(Entry);
    }
  }

  for (FileEditEntriesTy::iterator
         I = FileEditEntries.begin(), E = FileEditEntries.end(); I != E; ++I) {
    std::string TempFile = applyEditsToTemp(I->first, I->second,
                                            FileMgr, *Diags);
    if (TempFile.empty()) {
      hasErrorOccurred = true;
      continue;
    }

    remap.push_back(std::make_pair(I->first->getName(), TempFile));
  }

  return hasErrorOccurred;
}
