//===--- CGCall.cpp - Encapsulate calling convention details --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// These classes wrap the information about a call or function
// definition used to handle ABI compliancy.
//
//===----------------------------------------------------------------------===//

#include "CGCall.h"
#include "ABIInfo.h"
#include "CGCXXABI.h"
#include "CodeGenFunction.h"
#include "CodeGenModule.h"
#include "TargetInfo.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/CodeGen/CGFunctionInfo.h"
#include "clang/Frontend/CodeGenOptions.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Transforms/Utils/Local.h"
using namespace clang;
using namespace CodeGen;

/***/

static unsigned ClangCallConvToLLVMCallConv(CallingConv CC) {
  switch (CC) {
  default: return llvm::CallingConv::C;
  case CC_X86StdCall: return llvm::CallingConv::X86_StdCall;
  case CC_X86FastCall: return llvm::CallingConv::X86_FastCall;
  case CC_X86ThisCall: return llvm::CallingConv::X86_ThisCall;
  case CC_X86_64Win64: return llvm::CallingConv::X86_64_Win64;
  case CC_X86_64SysV: return llvm::CallingConv::X86_64_SysV;
  case CC_AAPCS: return llvm::CallingConv::ARM_AAPCS;
  case CC_AAPCS_VFP: return llvm::CallingConv::ARM_AAPCS_VFP;
  case CC_IntelOclBicc: return llvm::CallingConv::Intel_OCL_BI;
  // TODO: add support for CC_X86Pascal to llvm
  }
}

/// Derives the 'this' type for codegen purposes, i.e. ignoring method
/// qualification.
/// FIXME: address space qualification?
static CanQualType GetThisType(ASTContext &Context, const CXXRecordDecl *RD) {
  QualType RecTy = Context.getTagDeclType(RD)->getCanonicalTypeInternal();
  return Context.getPointerType(CanQualType::CreateUnsafe(RecTy));
}

/// Returns the canonical formal type of the given C++ method.
static CanQual<FunctionProtoType> GetFormalType(const CXXMethodDecl *MD) {
  return MD->getType()->getCanonicalTypeUnqualified()
           .getAs<FunctionProtoType>();
}

/// Returns the "extra-canonicalized" return type, which discards
/// qualifiers on the return type.  Codegen doesn't care about them,
/// and it makes ABI code a little easier to be able to assume that
/// all parameter and return types are top-level unqualified.
static CanQualType GetReturnType(QualType RetTy) {
  return RetTy->getCanonicalTypeUnqualified().getUnqualifiedType();
}

/// Arrange the argument and result information for a value of the given
/// unprototyped freestanding function type.
const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionType(CanQual<FunctionNoProtoType> FTNP) {
  // When translating an unprototyped function type, always use a
  // variadic type.
  return arrangeLLVMFunctionInfo(FTNP->getResultType().getUnqualifiedType(),
                                 None, FTNP->getExtInfo(), RequiredArgs(0));
}

/// Arrange the LLVM function layout for a value of the given function
/// type, on top of any implicit parameters already stored.  Use the
/// given ExtInfo instead of the ExtInfo from the function type.
static const CGFunctionInfo &arrangeLLVMFunctionInfo(CodeGenTypes &CGT,
                                       SmallVectorImpl<CanQualType> &prefix,
                                             CanQual<FunctionProtoType> FTP,
                                              FunctionType::ExtInfo extInfo) {
  RequiredArgs required = RequiredArgs::forPrototypePlus(FTP, prefix.size());
  // FIXME: Kill copy.
  for (unsigned i = 0, e = FTP->getNumArgs(); i != e; ++i)
    prefix.push_back(FTP->getArgType(i));
  CanQualType resultType = FTP->getResultType().getUnqualifiedType();
  return CGT.arrangeLLVMFunctionInfo(resultType, prefix, extInfo, required);
}

/// Arrange the argument and result information for a free function (i.e.
/// not a C++ or ObjC instance method) of the given type.
static const CGFunctionInfo &arrangeFreeFunctionType(CodeGenTypes &CGT,
                                      SmallVectorImpl<CanQualType> &prefix,
                                            CanQual<FunctionProtoType> FTP) {
  return arrangeLLVMFunctionInfo(CGT, prefix, FTP, FTP->getExtInfo());
}

/// Arrange the argument and result information for a free function (i.e.
/// not a C++ or ObjC instance method) of the given type.
static const CGFunctionInfo &arrangeCXXMethodType(CodeGenTypes &CGT,
                                      SmallVectorImpl<CanQualType> &prefix,
                                            CanQual<FunctionProtoType> FTP) {
  FunctionType::ExtInfo extInfo = FTP->getExtInfo();
  return arrangeLLVMFunctionInfo(CGT, prefix, FTP, extInfo);
}

/// Arrange the argument and result information for a value of the
/// given freestanding function type.
const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionType(CanQual<FunctionProtoType> FTP) {
  SmallVector<CanQualType, 16> argTypes;
  return ::arrangeFreeFunctionType(*this, argTypes, FTP);
}

static CallingConv getCallingConventionForDecl(const Decl *D, bool IsWindows) {
  // Set the appropriate calling convention for the Function.
  if (D->hasAttr<StdCallAttr>())
    return CC_X86StdCall;

  if (D->hasAttr<FastCallAttr>())
    return CC_X86FastCall;

  if (D->hasAttr<ThisCallAttr>())
    return CC_X86ThisCall;

  if (D->hasAttr<PascalAttr>())
    return CC_X86Pascal;

  if (PcsAttr *PCS = D->getAttr<PcsAttr>())
    return (PCS->getPCS() == PcsAttr::AAPCS ? CC_AAPCS : CC_AAPCS_VFP);

  if (D->hasAttr<PnaclCallAttr>())
    return CC_PnaclCall;

  if (D->hasAttr<IntelOclBiccAttr>())
    return CC_IntelOclBicc;

  if (D->hasAttr<MSABIAttr>())
    return IsWindows ? CC_C : CC_X86_64Win64;

  if (D->hasAttr<SysVABIAttr>())
    return IsWindows ? CC_X86_64SysV : CC_C;

  return CC_C;
}

/// Arrange the argument and result information for a call to an
/// unknown C++ non-static member function of the given abstract type.
/// (Zero value of RD means we don't have any meaningful "this" argument type,
///  so fall back to a generic pointer type).
/// The member function must be an ordinary function, i.e. not a
/// constructor or destructor.
const CGFunctionInfo &
CodeGenTypes::arrangeCXXMethodType(const CXXRecordDecl *RD,
                                   const FunctionProtoType *FTP) {
  SmallVector<CanQualType, 16> argTypes;

  // Add the 'this' pointer.
  if (RD)
    argTypes.push_back(GetThisType(Context, RD));
  else
    argTypes.push_back(Context.VoidPtrTy);

  return ::arrangeCXXMethodType(*this, argTypes,
              FTP->getCanonicalTypeUnqualified().getAs<FunctionProtoType>());
}

/// Arrange the argument and result information for a declaration or
/// definition of the given C++ non-static member function.  The
/// member function must be an ordinary function, i.e. not a
/// constructor or destructor.
const CGFunctionInfo &
CodeGenTypes::arrangeCXXMethodDeclaration(const CXXMethodDecl *MD) {
  assert(!isa<CXXConstructorDecl>(MD) && "wrong method for constructors!");
  assert(!isa<CXXDestructorDecl>(MD) && "wrong method for destructors!");

  CanQual<FunctionProtoType> prototype = GetFormalType(MD);

  if (MD->isInstance()) {
    // The abstract case is perfectly fine.
    const CXXRecordDecl *ThisType = TheCXXABI.getThisArgumentTypeForMethod(MD);
    return arrangeCXXMethodType(ThisType, prototype.getTypePtr());
  }

  return arrangeFreeFunctionType(prototype);
}

/// Arrange the argument and result information for a declaration
/// or definition to the given constructor variant.
const CGFunctionInfo &
CodeGenTypes::arrangeCXXConstructorDeclaration(const CXXConstructorDecl *D,
                                               CXXCtorType ctorKind) {
  SmallVector<CanQualType, 16> argTypes;
  argTypes.push_back(GetThisType(Context, D->getParent()));

  GlobalDecl GD(D, ctorKind);
  CanQualType resultType =
    TheCXXABI.HasThisReturn(GD) ? argTypes.front() : Context.VoidTy;

  CanQual<FunctionProtoType> FTP = GetFormalType(D);

  // Add the formal parameters.
  for (unsigned i = 0, e = FTP->getNumArgs(); i != e; ++i)
    argTypes.push_back(FTP->getArgType(i));

  TheCXXABI.BuildConstructorSignature(D, ctorKind, resultType, argTypes);

  RequiredArgs required =
      (D->isVariadic() ? RequiredArgs(argTypes.size()) : RequiredArgs::All);

  FunctionType::ExtInfo extInfo = FTP->getExtInfo();
  return arrangeLLVMFunctionInfo(resultType, argTypes, extInfo, required);
}

/// Arrange the argument and result information for a declaration,
/// definition, or call to the given destructor variant.  It so
/// happens that all three cases produce the same information.
const CGFunctionInfo &
CodeGenTypes::arrangeCXXDestructor(const CXXDestructorDecl *D,
                                   CXXDtorType dtorKind) {
  SmallVector<CanQualType, 2> argTypes;
  argTypes.push_back(GetThisType(Context, D->getParent()));

  GlobalDecl GD(D, dtorKind);
  CanQualType resultType =
    TheCXXABI.HasThisReturn(GD) ? argTypes.front() : Context.VoidTy;

  TheCXXABI.BuildDestructorSignature(D, dtorKind, resultType, argTypes);

  CanQual<FunctionProtoType> FTP = GetFormalType(D);
  assert(FTP->getNumArgs() == 0 && "dtor with formal parameters");
  assert(FTP->isVariadic() == 0 && "dtor with formal parameters");

  FunctionType::ExtInfo extInfo = FTP->getExtInfo();
  return arrangeLLVMFunctionInfo(resultType, argTypes, extInfo,
                                 RequiredArgs::All);
}

/// Arrange the argument and result information for the declaration or
/// definition of the given function.
const CGFunctionInfo &
CodeGenTypes::arrangeFunctionDeclaration(const FunctionDecl *FD) {
  if (const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(FD))
    if (MD->isInstance())
      return arrangeCXXMethodDeclaration(MD);

  CanQualType FTy = FD->getType()->getCanonicalTypeUnqualified();

  assert(isa<FunctionType>(FTy));

  // When declaring a function without a prototype, always use a
  // non-variadic type.
  if (isa<FunctionNoProtoType>(FTy)) {
    CanQual<FunctionNoProtoType> noProto = FTy.getAs<FunctionNoProtoType>();
    return arrangeLLVMFunctionInfo(noProto->getResultType(), None,
                                   noProto->getExtInfo(), RequiredArgs::All);
  }

  assert(isa<FunctionProtoType>(FTy));
  return arrangeFreeFunctionType(FTy.getAs<FunctionProtoType>());
}

/// Arrange the argument and result information for the declaration or
/// definition of an Objective-C method.
const CGFunctionInfo &
CodeGenTypes::arrangeObjCMethodDeclaration(const ObjCMethodDecl *MD) {
  // It happens that this is the same as a call with no optional
  // arguments, except also using the formal 'self' type.
  return arrangeObjCMessageSendSignature(MD, MD->getSelfDecl()->getType());
}

/// Arrange the argument and result information for the function type
/// through which to perform a send to the given Objective-C method,
/// using the given receiver type.  The receiver type is not always
/// the 'self' type of the method or even an Objective-C pointer type.
/// This is *not* the right method for actually performing such a
/// message send, due to the possibility of optional arguments.
const CGFunctionInfo &
CodeGenTypes::arrangeObjCMessageSendSignature(const ObjCMethodDecl *MD,
                                              QualType receiverType) {
  SmallVector<CanQualType, 16> argTys;
  argTys.push_back(Context.getCanonicalParamType(receiverType));
  argTys.push_back(Context.getCanonicalParamType(Context.getObjCSelType()));
  // FIXME: Kill copy?
  for (ObjCMethodDecl::param_const_iterator i = MD->param_begin(),
         e = MD->param_end(); i != e; ++i) {
    argTys.push_back(Context.getCanonicalParamType((*i)->getType()));
  }

  FunctionType::ExtInfo einfo;
  bool IsWindows = getContext().getTargetInfo().getTriple().isOSWindows();
  einfo = einfo.withCallingConv(getCallingConventionForDecl(MD, IsWindows));

  if (getContext().getLangOpts().ObjCAutoRefCount &&
      MD->hasAttr<NSReturnsRetainedAttr>())
    einfo = einfo.withProducesResult(true);

  RequiredArgs required =
    (MD->isVariadic() ? RequiredArgs(argTys.size()) : RequiredArgs::All);

  return arrangeLLVMFunctionInfo(GetReturnType(MD->getResultType()), argTys,
                                 einfo, required);
}

const CGFunctionInfo &
CodeGenTypes::arrangeGlobalDeclaration(GlobalDecl GD) {
  // FIXME: Do we need to handle ObjCMethodDecl?
  const FunctionDecl *FD = cast<FunctionDecl>(GD.getDecl());

  if (const CXXConstructorDecl *CD = dyn_cast<CXXConstructorDecl>(FD))
    return arrangeCXXConstructorDeclaration(CD, GD.getCtorType());

  if (const CXXDestructorDecl *DD = dyn_cast<CXXDestructorDecl>(FD))
    return arrangeCXXDestructor(DD, GD.getDtorType());

  return arrangeFunctionDeclaration(FD);
}

/// Arrange a call as unto a free function, except possibly with an
/// additional number of formal parameters considered required.
static const CGFunctionInfo &
arrangeFreeFunctionLikeCall(CodeGenTypes &CGT,
                            CodeGenModule &CGM,
                            const CallArgList &args,
                            const FunctionType *fnType,
                            unsigned numExtraRequiredArgs) {
  assert(args.size() >= numExtraRequiredArgs);

  // In most cases, there are no optional arguments.
  RequiredArgs required = RequiredArgs::All;

  // If we have a variadic prototype, the required arguments are the
  // extra prefix plus the arguments in the prototype.
  if (const FunctionProtoType *proto = dyn_cast<FunctionProtoType>(fnType)) {
    if (proto->isVariadic())
      required = RequiredArgs(proto->getNumArgs() + numExtraRequiredArgs);

  // If we don't have a prototype at all, but we're supposed to
  // explicitly use the variadic convention for unprototyped calls,
  // treat all of the arguments as required but preserve the nominal
  // possibility of variadics.
  } else if (CGM.getTargetCodeGenInfo()
                .isNoProtoCallVariadic(args,
                                       cast<FunctionNoProtoType>(fnType))) {
    required = RequiredArgs(args.size());
  }

  return CGT.arrangeFreeFunctionCall(fnType->getResultType(), args,
                                     fnType->getExtInfo(), required);
}

/// Figure out the rules for calling a function with the given formal
/// type using the given arguments.  The arguments are necessary
/// because the function might be unprototyped, in which case it's
/// target-dependent in crazy ways.
const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionCall(const CallArgList &args,
                                      const FunctionType *fnType) {
  return arrangeFreeFunctionLikeCall(*this, CGM, args, fnType, 0);
}

/// A block function call is essentially a free-function call with an
/// extra implicit argument.
const CGFunctionInfo &
CodeGenTypes::arrangeBlockFunctionCall(const CallArgList &args,
                                       const FunctionType *fnType) {
  return arrangeFreeFunctionLikeCall(*this, CGM, args, fnType, 1);
}

const CGFunctionInfo &
CodeGenTypes::arrangeFreeFunctionCall(QualType resultType,
                                      const CallArgList &args,
                                      FunctionType::ExtInfo info,
                                      RequiredArgs required) {
  // FIXME: Kill copy.
  SmallVector<CanQualType, 16> argTypes;
  for (CallArgList::const_iterator i = args.begin(), e = args.end();
       i != e; ++i)
    argTypes.push_back(Context.getCanonicalParamType(i->Ty));
  return arrangeLLVMFunctionInfo(GetReturnType(resultType), argTypes, info,
                                 required);
}

/// Arrange a call to a C++ method, passing the given arguments.
const CGFunctionInfo &
CodeGenTypes::arrangeCXXMethodCall(const CallArgList &args,
                                   const FunctionProtoType *FPT,
                                   RequiredArgs required) {
  // FIXME: Kill copy.
  SmallVector<CanQualType, 16> argTypes;
  for (CallArgList::const_iterator i = args.begin(), e = args.end();
       i != e; ++i)
    argTypes.push_back(Context.getCanonicalParamType(i->Ty));

  FunctionType::ExtInfo info = FPT->getExtInfo();
  return arrangeLLVMFunctionInfo(GetReturnType(FPT->getResultType()),
                                 argTypes, info, required);
}

const CGFunctionInfo &
CodeGenTypes::arrangeFunctionDeclaration(QualType resultType,
                                         const FunctionArgList &args,
                                         const FunctionType::ExtInfo &info,
                                         bool isVariadic) {
  // FIXME: Kill copy.
  SmallVector<CanQualType, 16> argTypes;
  for (FunctionArgList::const_iterator i = args.begin(), e = args.end();
       i != e; ++i)
    argTypes.push_back(Context.getCanonicalParamType((*i)->getType()));

  RequiredArgs required =
    (isVariadic ? RequiredArgs(args.size()) : RequiredArgs::All);
  return arrangeLLVMFunctionInfo(GetReturnType(resultType), argTypes, info,
                                 required);
}

const CGFunctionInfo &CodeGenTypes::arrangeNullaryFunction() {
  return arrangeLLVMFunctionInfo(getContext().VoidTy, None,
                                 FunctionType::ExtInfo(), RequiredArgs::All);
}

/// Arrange the argument and result information for an abstract value
/// of a given function type.  This is the method which all of the
/// above functions ultimately defer to.
const CGFunctionInfo &
CodeGenTypes::arrangeLLVMFunctionInfo(CanQualType resultType,
                                      ArrayRef<CanQualType> argTypes,
                                      FunctionType::ExtInfo info,
                                      RequiredArgs required) {
#ifndef NDEBUG
  for (ArrayRef<CanQualType>::const_iterator
         I = argTypes.begin(), E = argTypes.end(); I != E; ++I)
    assert(I->isCanonicalAsParam());
#endif

  unsigned CC = ClangCallConvToLLVMCallConv(info.getCC());

  // Lookup or create unique function info.
  llvm::FoldingSetNodeID ID;
  CGFunctionInfo::Profile(ID, info, required, resultType, argTypes);

  void *insertPos = 0;
  CGFunctionInfo *FI = FunctionInfos.FindNodeOrInsertPos(ID, insertPos);
  if (FI)
    return *FI;

  // Construct the function info.  We co-allocate the ArgInfos.
  FI = CGFunctionInfo::create(CC, info, resultType, argTypes, required);
  FunctionInfos.InsertNode(FI, insertPos);

  bool inserted = FunctionsBeingProcessed.insert(FI); (void)inserted;
  assert(inserted && "Recursively being processed?");
  
  // Compute ABI information.
  getABIInfo().computeInfo(*FI);

  // Loop over all of the computed argument and return value info.  If any of
  // them are direct or extend without a specified coerce type, specify the
  // default now.
  ABIArgInfo &retInfo = FI->getReturnInfo();
  if (retInfo.canHaveCoerceToType() && retInfo.getCoerceToType() == 0)
    retInfo.setCoerceToType(ConvertType(FI->getReturnType()));

  for (CGFunctionInfo::arg_iterator I = FI->arg_begin(), E = FI->arg_end();
       I != E; ++I)
    if (I->info.canHaveCoerceToType() && I->info.getCoerceToType() == 0)
      I->info.setCoerceToType(ConvertType(I->type));

  bool erased = FunctionsBeingProcessed.erase(FI); (void)erased;
  assert(erased && "Not in set?");
  
  return *FI;
}

CGFunctionInfo *CGFunctionInfo::create(unsigned llvmCC,
                                       const FunctionType::ExtInfo &info,
                                       CanQualType resultType,
                                       ArrayRef<CanQualType> argTypes,
                                       RequiredArgs required) {
  void *buffer = operator new(sizeof(CGFunctionInfo) +
                              sizeof(ArgInfo) * (argTypes.size() + 1));
  CGFunctionInfo *FI = new(buffer) CGFunctionInfo();
  FI->CallingConvention = llvmCC;
  FI->EffectiveCallingConvention = llvmCC;
  FI->ASTCallingConvention = info.getCC();
  FI->NoReturn = info.getNoReturn();
  FI->ReturnsRetained = info.getProducesResult();
  FI->Required = required;
  FI->HasRegParm = info.getHasRegParm();
  FI->RegParm = info.getRegParm();
  FI->NumArgs = argTypes.size();
  FI->getArgsBuffer()[0].type = resultType;
  for (unsigned i = 0, e = argTypes.size(); i != e; ++i)
    FI->getArgsBuffer()[i + 1].type = argTypes[i];
  return FI;
}

/***/

void CodeGenTypes::GetExpandedTypes(QualType type,
                     SmallVectorImpl<llvm::Type*> &expandedTypes) {
  if (const ConstantArrayType *AT = Context.getAsConstantArrayType(type)) {
    uint64_t NumElts = AT->getSize().getZExtValue();
    for (uint64_t Elt = 0; Elt < NumElts; ++Elt)
      GetExpandedTypes(AT->getElementType(), expandedTypes);
  } else if (const RecordType *RT = type->getAs<RecordType>()) {
    const RecordDecl *RD = RT->getDecl();
    assert(!RD->hasFlexibleArrayMember() &&
           "Cannot expand structure with flexible array.");
    if (RD->isUnion()) {
      // Unions can be here only in degenerative cases - all the fields are same
      // after flattening. Thus we have to use the "largest" field.
      const FieldDecl *LargestFD = 0;
      CharUnits UnionSize = CharUnits::Zero();

      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        const FieldDecl *FD = *i;
        assert(!FD->isBitField() &&
               "Cannot expand structure with bit-field members.");
        CharUnits FieldSize = getContext().getTypeSizeInChars(FD->getType());
        if (UnionSize < FieldSize) {
          UnionSize = FieldSize;
          LargestFD = FD;
        }
      }
      if (LargestFD)
        GetExpandedTypes(LargestFD->getType(), expandedTypes);
    } else {
      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        assert(!i->isBitField() &&
               "Cannot expand structure with bit-field members.");
        GetExpandedTypes(i->getType(), expandedTypes);
      }
    }
  } else if (const ComplexType *CT = type->getAs<ComplexType>()) {
    llvm::Type *EltTy = ConvertType(CT->getElementType());
    expandedTypes.push_back(EltTy);
    expandedTypes.push_back(EltTy);
  } else
    expandedTypes.push_back(ConvertType(type));
}

llvm::Function::arg_iterator
CodeGenFunction::ExpandTypeFromArgs(QualType Ty, LValue LV,
                                    llvm::Function::arg_iterator AI) {
  assert(LV.isSimple() &&
         "Unexpected non-simple lvalue during struct expansion.");

  if (const ConstantArrayType *AT = getContext().getAsConstantArrayType(Ty)) {
    unsigned NumElts = AT->getSize().getZExtValue();
    QualType EltTy = AT->getElementType();
    for (unsigned Elt = 0; Elt < NumElts; ++Elt) {
      llvm::Value *EltAddr = Builder.CreateConstGEP2_32(LV.getAddress(), 0, Elt);
      LValue LV = MakeAddrLValue(EltAddr, EltTy);
      AI = ExpandTypeFromArgs(EltTy, LV, AI);
    }
  } else if (const RecordType *RT = Ty->getAs<RecordType>()) {
    RecordDecl *RD = RT->getDecl();
    if (RD->isUnion()) {
      // Unions can be here only in degenerative cases - all the fields are same
      // after flattening. Thus we have to use the "largest" field.
      const FieldDecl *LargestFD = 0;
      CharUnits UnionSize = CharUnits::Zero();

      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        const FieldDecl *FD = *i;
        assert(!FD->isBitField() &&
               "Cannot expand structure with bit-field members.");
        CharUnits FieldSize = getContext().getTypeSizeInChars(FD->getType());
        if (UnionSize < FieldSize) {
          UnionSize = FieldSize;
          LargestFD = FD;
        }
      }
      if (LargestFD) {
        // FIXME: What are the right qualifiers here?
        LValue SubLV = EmitLValueForField(LV, LargestFD);
        AI = ExpandTypeFromArgs(LargestFD->getType(), SubLV, AI);
      }
    } else {
      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        FieldDecl *FD = *i;
        QualType FT = FD->getType();

        // FIXME: What are the right qualifiers here?
        LValue SubLV = EmitLValueForField(LV, FD);
        AI = ExpandTypeFromArgs(FT, SubLV, AI);
      }
    }
  } else if (const ComplexType *CT = Ty->getAs<ComplexType>()) {
    QualType EltTy = CT->getElementType();
    llvm::Value *RealAddr = Builder.CreateStructGEP(LV.getAddress(), 0, "real");
    EmitStoreThroughLValue(RValue::get(AI++), MakeAddrLValue(RealAddr, EltTy));
    llvm::Value *ImagAddr = Builder.CreateStructGEP(LV.getAddress(), 1, "imag");
    EmitStoreThroughLValue(RValue::get(AI++), MakeAddrLValue(ImagAddr, EltTy));
  } else {
    EmitStoreThroughLValue(RValue::get(AI), LV);
    ++AI;
  }

  return AI;
}

/// EnterStructPointerForCoercedAccess - Given a struct pointer that we are
/// accessing some number of bytes out of it, try to gep into the struct to get
/// at its inner goodness.  Dive as deep as possible without entering an element
/// with an in-memory size smaller than DstSize.
static llvm::Value *
EnterStructPointerForCoercedAccess(llvm::Value *SrcPtr,
                                   llvm::StructType *SrcSTy,
                                   uint64_t DstSize, CodeGenFunction &CGF) {
  // We can't dive into a zero-element struct.
  if (SrcSTy->getNumElements() == 0) return SrcPtr;

  llvm::Type *FirstElt = SrcSTy->getElementType(0);

  // If the first elt is at least as large as what we're looking for, or if the
  // first element is the same size as the whole struct, we can enter it.
  uint64_t FirstEltSize =
    CGF.CGM.getDataLayout().getTypeAllocSize(FirstElt);
  if (FirstEltSize < DstSize &&
      FirstEltSize < CGF.CGM.getDataLayout().getTypeAllocSize(SrcSTy))
    return SrcPtr;

  // GEP into the first element.
  SrcPtr = CGF.Builder.CreateConstGEP2_32(SrcPtr, 0, 0, "coerce.dive");

  // If the first element is a struct, recurse.
  llvm::Type *SrcTy =
    cast<llvm::PointerType>(SrcPtr->getType())->getElementType();
  if (llvm::StructType *SrcSTy = dyn_cast<llvm::StructType>(SrcTy))
    return EnterStructPointerForCoercedAccess(SrcPtr, SrcSTy, DstSize, CGF);

  return SrcPtr;
}

/// CoerceIntOrPtrToIntOrPtr - Convert a value Val to the specific Ty where both
/// are either integers or pointers.  This does a truncation of the value if it
/// is too large or a zero extension if it is too small.
///
/// This behaves as if the value were coerced through memory, so on big-endian
/// targets the high bits are preserved in a truncation, while little-endian
/// targets preserve the low bits.
static llvm::Value *CoerceIntOrPtrToIntOrPtr(llvm::Value *Val,
                                             llvm::Type *Ty,
                                             CodeGenFunction &CGF) {
  if (Val->getType() == Ty)
    return Val;

  if (isa<llvm::PointerType>(Val->getType())) {
    // If this is Pointer->Pointer avoid conversion to and from int.
    if (isa<llvm::PointerType>(Ty))
      return CGF.Builder.CreateBitCast(Val, Ty, "coerce.val");

    // Convert the pointer to an integer so we can play with its width.
    Val = CGF.Builder.CreatePtrToInt(Val, CGF.IntPtrTy, "coerce.val.pi");
  }

  llvm::Type *DestIntTy = Ty;
  if (isa<llvm::PointerType>(DestIntTy))
    DestIntTy = CGF.IntPtrTy;

  if (Val->getType() != DestIntTy) {
    const llvm::DataLayout &DL = CGF.CGM.getDataLayout();
    if (DL.isBigEndian()) {
      // Preserve the high bits on big-endian targets.
      // That is what memory coercion does.
      uint64_t SrcSize = DL.getTypeAllocSizeInBits(Val->getType());
      uint64_t DstSize = DL.getTypeAllocSizeInBits(DestIntTy);
      if (SrcSize > DstSize) {
        Val = CGF.Builder.CreateLShr(Val, SrcSize - DstSize, "coerce.highbits");
        Val = CGF.Builder.CreateTrunc(Val, DestIntTy, "coerce.val.ii");
      } else {
        Val = CGF.Builder.CreateZExt(Val, DestIntTy, "coerce.val.ii");
        Val = CGF.Builder.CreateShl(Val, DstSize - SrcSize, "coerce.highbits");
      }
    } else {
      // Little-endian targets preserve the low bits. No shifts required.
      Val = CGF.Builder.CreateIntCast(Val, DestIntTy, false, "coerce.val.ii");
    }
  }

  if (isa<llvm::PointerType>(Ty))
    Val = CGF.Builder.CreateIntToPtr(Val, Ty, "coerce.val.ip");
  return Val;
}



/// CreateCoercedLoad - Create a load from \arg SrcPtr interpreted as
/// a pointer to an object of type \arg Ty.
///
/// This safely handles the case when the src type is smaller than the
/// destination type; in this situation the values of bits which not
/// present in the src are undefined.
static llvm::Value *CreateCoercedLoad(llvm::Value *SrcPtr,
                                      llvm::Type *Ty,
                                      CodeGenFunction &CGF) {
  llvm::Type *SrcTy =
    cast<llvm::PointerType>(SrcPtr->getType())->getElementType();

  // If SrcTy and Ty are the same, just do a load.
  if (SrcTy == Ty)
    return CGF.Builder.CreateLoad(SrcPtr);

  uint64_t DstSize = CGF.CGM.getDataLayout().getTypeAllocSize(Ty);

  if (llvm::StructType *SrcSTy = dyn_cast<llvm::StructType>(SrcTy)) {
    SrcPtr = EnterStructPointerForCoercedAccess(SrcPtr, SrcSTy, DstSize, CGF);
    SrcTy = cast<llvm::PointerType>(SrcPtr->getType())->getElementType();
  }

  uint64_t SrcSize = CGF.CGM.getDataLayout().getTypeAllocSize(SrcTy);

  // If the source and destination are integer or pointer types, just do an
  // extension or truncation to the desired type.
  if ((isa<llvm::IntegerType>(Ty) || isa<llvm::PointerType>(Ty)) &&
      (isa<llvm::IntegerType>(SrcTy) || isa<llvm::PointerType>(SrcTy))) {
    llvm::LoadInst *Load = CGF.Builder.CreateLoad(SrcPtr);
    return CoerceIntOrPtrToIntOrPtr(Load, Ty, CGF);
  }

  // If load is legal, just bitcast the src pointer.
  if (SrcSize >= DstSize) {
    // Generally SrcSize is never greater than DstSize, since this means we are
    // losing bits. However, this can happen in cases where the structure has
    // additional padding, for example due to a user specified alignment.
    //
    // FIXME: Assert that we aren't truncating non-padding bits when have access
    // to that information.
    llvm::Value *Casted =
      CGF.Builder.CreateBitCast(SrcPtr, llvm::PointerType::getUnqual(Ty));
    llvm::LoadInst *Load = CGF.Builder.CreateLoad(Casted);
    // FIXME: Use better alignment / avoid requiring aligned load.
    Load->setAlignment(1);
    return Load;
  }

  // Otherwise do coercion through memory. This is stupid, but
  // simple.
  llvm::Value *Tmp = CGF.CreateTempAlloca(Ty);
  llvm::Type *I8PtrTy = CGF.Builder.getInt8PtrTy();
  llvm::Value *Casted = CGF.Builder.CreateBitCast(Tmp, I8PtrTy);
  llvm::Value *SrcCasted = CGF.Builder.CreateBitCast(SrcPtr, I8PtrTy);
  // FIXME: Use better alignment.
  CGF.Builder.CreateMemCpy(Casted, SrcCasted,
      llvm::ConstantInt::get(CGF.IntPtrTy, SrcSize),
      1, false);
  return CGF.Builder.CreateLoad(Tmp);
}

// Function to store a first-class aggregate into memory.  We prefer to
// store the elements rather than the aggregate to be more friendly to
// fast-isel.
// FIXME: Do we need to recurse here?
static void BuildAggStore(CodeGenFunction &CGF, llvm::Value *Val,
                          llvm::Value *DestPtr, bool DestIsVolatile,
                          bool LowAlignment) {
  // Prefer scalar stores to first-class aggregate stores.
  if (llvm::StructType *STy =
        dyn_cast<llvm::StructType>(Val->getType())) {
    for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
      llvm::Value *EltPtr = CGF.Builder.CreateConstGEP2_32(DestPtr, 0, i);
      llvm::Value *Elt = CGF.Builder.CreateExtractValue(Val, i);
      llvm::StoreInst *SI = CGF.Builder.CreateStore(Elt, EltPtr,
                                                    DestIsVolatile);
      if (LowAlignment)
        SI->setAlignment(1);
    }
  } else {
    llvm::StoreInst *SI = CGF.Builder.CreateStore(Val, DestPtr, DestIsVolatile);
    if (LowAlignment)
      SI->setAlignment(1);
  }
}

/// CreateCoercedStore - Create a store to \arg DstPtr from \arg Src,
/// where the source and destination may have different types.
///
/// This safely handles the case when the src type is larger than the
/// destination type; the upper bits of the src will be lost.
static void CreateCoercedStore(llvm::Value *Src,
                               llvm::Value *DstPtr,
                               bool DstIsVolatile,
                               CodeGenFunction &CGF) {
  llvm::Type *SrcTy = Src->getType();
  llvm::Type *DstTy =
    cast<llvm::PointerType>(DstPtr->getType())->getElementType();
  if (SrcTy == DstTy) {
    CGF.Builder.CreateStore(Src, DstPtr, DstIsVolatile);
    return;
  }

  uint64_t SrcSize = CGF.CGM.getDataLayout().getTypeAllocSize(SrcTy);

  if (llvm::StructType *DstSTy = dyn_cast<llvm::StructType>(DstTy)) {
    DstPtr = EnterStructPointerForCoercedAccess(DstPtr, DstSTy, SrcSize, CGF);
    DstTy = cast<llvm::PointerType>(DstPtr->getType())->getElementType();
  }

  // If the source and destination are integer or pointer types, just do an
  // extension or truncation to the desired type.
  if ((isa<llvm::IntegerType>(SrcTy) || isa<llvm::PointerType>(SrcTy)) &&
      (isa<llvm::IntegerType>(DstTy) || isa<llvm::PointerType>(DstTy))) {
    Src = CoerceIntOrPtrToIntOrPtr(Src, DstTy, CGF);
    CGF.Builder.CreateStore(Src, DstPtr, DstIsVolatile);
    return;
  }

  uint64_t DstSize = CGF.CGM.getDataLayout().getTypeAllocSize(DstTy);

  // If store is legal, just bitcast the src pointer.
  if (SrcSize <= DstSize) {
    llvm::Value *Casted =
      CGF.Builder.CreateBitCast(DstPtr, llvm::PointerType::getUnqual(SrcTy));
    // FIXME: Use better alignment / avoid requiring aligned store.
    BuildAggStore(CGF, Src, Casted, DstIsVolatile, true);
  } else {
    // Otherwise do coercion through memory. This is stupid, but
    // simple.

    // Generally SrcSize is never greater than DstSize, since this means we are
    // losing bits. However, this can happen in cases where the structure has
    // additional padding, for example due to a user specified alignment.
    //
    // FIXME: Assert that we aren't truncating non-padding bits when have access
    // to that information.
    llvm::Value *Tmp = CGF.CreateTempAlloca(SrcTy);
    CGF.Builder.CreateStore(Src, Tmp);
    llvm::Type *I8PtrTy = CGF.Builder.getInt8PtrTy();
    llvm::Value *Casted = CGF.Builder.CreateBitCast(Tmp, I8PtrTy);
    llvm::Value *DstCasted = CGF.Builder.CreateBitCast(DstPtr, I8PtrTy);
    // FIXME: Use better alignment.
    CGF.Builder.CreateMemCpy(DstCasted, Casted,
        llvm::ConstantInt::get(CGF.IntPtrTy, DstSize),
        1, false);
  }
}

/***/

bool CodeGenModule::ReturnTypeUsesSRet(const CGFunctionInfo &FI) {
  return FI.getReturnInfo().isIndirect();
}

bool CodeGenModule::ReturnTypeUsesFPRet(QualType ResultType) {
  if (const BuiltinType *BT = ResultType->getAs<BuiltinType>()) {
    switch (BT->getKind()) {
    default:
      return false;
    case BuiltinType::Float:
      return getTarget().useObjCFPRetForRealType(TargetInfo::Float);
    case BuiltinType::Double:
      return getTarget().useObjCFPRetForRealType(TargetInfo::Double);
    case BuiltinType::LongDouble:
      return getTarget().useObjCFPRetForRealType(TargetInfo::LongDouble);
    }
  }

  return false;
}

bool CodeGenModule::ReturnTypeUsesFP2Ret(QualType ResultType) {
  if (const ComplexType *CT = ResultType->getAs<ComplexType>()) {
    if (const BuiltinType *BT = CT->getElementType()->getAs<BuiltinType>()) {
      if (BT->getKind() == BuiltinType::LongDouble)
        return getTarget().useObjCFP2RetForComplexLongDouble();
    }
  }

  return false;
}

llvm::FunctionType *CodeGenTypes::GetFunctionType(GlobalDecl GD) {
  const CGFunctionInfo &FI = arrangeGlobalDeclaration(GD);
  return GetFunctionType(FI);
}

llvm::FunctionType *
CodeGenTypes::GetFunctionType(const CGFunctionInfo &FI) {
  
  bool Inserted = FunctionsBeingProcessed.insert(&FI); (void)Inserted;
  assert(Inserted && "Recursively being processed?");
  
  SmallVector<llvm::Type*, 8> argTypes;
  llvm::Type *resultType = 0;

  const ABIArgInfo &retAI = FI.getReturnInfo();
  switch (retAI.getKind()) {
  case ABIArgInfo::Expand:
    llvm_unreachable("Invalid ABI kind for return argument");

  case ABIArgInfo::Extend:
  case ABIArgInfo::Direct:
    resultType = retAI.getCoerceToType();
    break;

  case ABIArgInfo::Indirect: {
    assert(!retAI.getIndirectAlign() && "Align unused on indirect return.");
    resultType = llvm::Type::getVoidTy(getLLVMContext());

    QualType ret = FI.getReturnType();
    llvm::Type *ty = ConvertType(ret);
    unsigned addressSpace = Context.getTargetAddressSpace(ret);
    argTypes.push_back(llvm::PointerType::get(ty, addressSpace));
    break;
  }

  case ABIArgInfo::Ignore:
    resultType = llvm::Type::getVoidTy(getLLVMContext());
    break;
  }

  // Add in all of the required arguments.
  CGFunctionInfo::const_arg_iterator it = FI.arg_begin(), ie;
  if (FI.isVariadic()) {
    ie = it + FI.getRequiredArgs().getNumRequiredArgs();
  } else {
    ie = FI.arg_end();
  }
  for (; it != ie; ++it) {
    const ABIArgInfo &argAI = it->info;

    // Insert a padding type to ensure proper alignment.
    if (llvm::Type *PaddingType = argAI.getPaddingType())
      argTypes.push_back(PaddingType);

    switch (argAI.getKind()) {
    case ABIArgInfo::Ignore:
      break;

    case ABIArgInfo::Indirect: {
      // indirect arguments are always on the stack, which is addr space #0.
      llvm::Type *LTy = ConvertTypeForMem(it->type);
      argTypes.push_back(LTy->getPointerTo());
      break;
    }

    case ABIArgInfo::Extend:
    case ABIArgInfo::Direct: {
      // If the coerce-to type is a first class aggregate, flatten it.  Either
      // way is semantically identical, but fast-isel and the optimizer
      // generally likes scalar values better than FCAs.
      llvm::Type *argType = argAI.getCoerceToType();
      if (llvm::StructType *st = dyn_cast<llvm::StructType>(argType)) {
        for (unsigned i = 0, e = st->getNumElements(); i != e; ++i)
          argTypes.push_back(st->getElementType(i));
      } else {
        argTypes.push_back(argType);
      }
      break;
    }

    case ABIArgInfo::Expand:
      GetExpandedTypes(it->type, argTypes);
      break;
    }
  }

  bool Erased = FunctionsBeingProcessed.erase(&FI); (void)Erased;
  assert(Erased && "Not in set?");
  
  return llvm::FunctionType::get(resultType, argTypes, FI.isVariadic());
}

llvm::Type *CodeGenTypes::GetFunctionTypeForVTable(GlobalDecl GD) {
  const CXXMethodDecl *MD = cast<CXXMethodDecl>(GD.getDecl());
  const FunctionProtoType *FPT = MD->getType()->getAs<FunctionProtoType>();

  if (!isFuncTypeConvertible(FPT))
    return llvm::StructType::get(getLLVMContext());
    
  const CGFunctionInfo *Info;
  if (isa<CXXDestructorDecl>(MD))
    Info = &arrangeCXXDestructor(cast<CXXDestructorDecl>(MD), GD.getDtorType());
  else
    Info = &arrangeCXXMethodDeclaration(MD);
  return GetFunctionType(*Info);
}

void CodeGenModule::ConstructAttributeList(const CGFunctionInfo &FI,
                                           const Decl *TargetDecl,
                                           AttributeListType &PAL,
                                           unsigned &CallingConv,
                                           bool AttrOnCallSite) {
  llvm::AttrBuilder FuncAttrs;
  llvm::AttrBuilder RetAttrs;

  CallingConv = FI.getEffectiveCallingConvention();

  if (FI.isNoReturn())
    FuncAttrs.addAttribute(llvm::Attribute::NoReturn);

  // FIXME: handle sseregparm someday...
  if (TargetDecl) {
    if (TargetDecl->hasAttr<ReturnsTwiceAttr>())
      FuncAttrs.addAttribute(llvm::Attribute::ReturnsTwice);
    if (TargetDecl->hasAttr<NoThrowAttr>())
      FuncAttrs.addAttribute(llvm::Attribute::NoUnwind);
    if (TargetDecl->hasAttr<NoReturnAttr>())
      FuncAttrs.addAttribute(llvm::Attribute::NoReturn);

    if (const FunctionDecl *Fn = dyn_cast<FunctionDecl>(TargetDecl)) {
      const FunctionProtoType *FPT = Fn->getType()->getAs<FunctionProtoType>();
      if (FPT && FPT->isNothrow(getContext()))
        FuncAttrs.addAttribute(llvm::Attribute::NoUnwind);
      // Don't use [[noreturn]] or _Noreturn for a call to a virtual function.
      // These attributes are not inherited by overloads.
      const CXXMethodDecl *MD = dyn_cast<CXXMethodDecl>(Fn);
      if (Fn->isNoReturn() && !(AttrOnCallSite && MD && MD->isVirtual()))
        FuncAttrs.addAttribute(llvm::Attribute::NoReturn);
    }

    // 'const' and 'pure' attribute functions are also nounwind.
    if (TargetDecl->hasAttr<ConstAttr>()) {
      FuncAttrs.addAttribute(llvm::Attribute::ReadNone);
      FuncAttrs.addAttribute(llvm::Attribute::NoUnwind);
    } else if (TargetDecl->hasAttr<PureAttr>()) {
      FuncAttrs.addAttribute(llvm::Attribute::ReadOnly);
      FuncAttrs.addAttribute(llvm::Attribute::NoUnwind);
    }
    if (TargetDecl->hasAttr<MallocAttr>())
      RetAttrs.addAttribute(llvm::Attribute::NoAlias);
  }

  if (CodeGenOpts.OptimizeSize)
    FuncAttrs.addAttribute(llvm::Attribute::OptimizeForSize);
  if (CodeGenOpts.OptimizeSize == 2)
    FuncAttrs.addAttribute(llvm::Attribute::MinSize);
  if (CodeGenOpts.DisableRedZone)
    FuncAttrs.addAttribute(llvm::Attribute::NoRedZone);
  if (CodeGenOpts.NoImplicitFloat)
    FuncAttrs.addAttribute(llvm::Attribute::NoImplicitFloat);

  if (AttrOnCallSite) {
    // Attributes that should go on the call site only.
    if (!CodeGenOpts.SimplifyLibCalls)
      FuncAttrs.addAttribute(llvm::Attribute::NoBuiltin);
  } else {
    // Attributes that should go on the function, but not the call site.
    if (!CodeGenOpts.DisableFPElim) {
      FuncAttrs.addAttribute("no-frame-pointer-elim", "false");
    } else if (CodeGenOpts.OmitLeafFramePointer) {
      FuncAttrs.addAttribute("no-frame-pointer-elim", "false");
      FuncAttrs.addAttribute("no-frame-pointer-elim-non-leaf");
    } else {
      FuncAttrs.addAttribute("no-frame-pointer-elim", "true");
      FuncAttrs.addAttribute("no-frame-pointer-elim-non-leaf");
    }

    FuncAttrs.addAttribute("less-precise-fpmad",
                           llvm::toStringRef(CodeGenOpts.LessPreciseFPMAD));
    FuncAttrs.addAttribute("no-infs-fp-math",
                           llvm::toStringRef(CodeGenOpts.NoInfsFPMath));
    FuncAttrs.addAttribute("no-nans-fp-math",
                           llvm::toStringRef(CodeGenOpts.NoNaNsFPMath));
    FuncAttrs.addAttribute("unsafe-fp-math",
                           llvm::toStringRef(CodeGenOpts.UnsafeFPMath));
    FuncAttrs.addAttribute("use-soft-float",
                           llvm::toStringRef(CodeGenOpts.SoftFloat));
    FuncAttrs.addAttribute("stack-protector-buffer-size",
                           llvm::utostr(CodeGenOpts.SSPBufferSize));

    if (!CodeGenOpts.StackRealignment)
      FuncAttrs.addAttribute("no-realign-stack");
  }

  QualType RetTy = FI.getReturnType();
  unsigned Index = 1;
  const ABIArgInfo &RetAI = FI.getReturnInfo();
  switch (RetAI.getKind()) {
  case ABIArgInfo::Extend:
    if (RetTy->hasSignedIntegerRepresentation())
      RetAttrs.addAttribute(llvm::Attribute::SExt);
    else if (RetTy->hasUnsignedIntegerRepresentation())
      RetAttrs.addAttribute(llvm::Attribute::ZExt);
    // FALL THROUGH
  case ABIArgInfo::Direct:
    if (RetAI.getInReg())
      RetAttrs.addAttribute(llvm::Attribute::InReg);
    break;
  case ABIArgInfo::Ignore:
    break;

  case ABIArgInfo::Indirect: {
    llvm::AttrBuilder SRETAttrs;
    SRETAttrs.addAttribute(llvm::Attribute::StructRet);
    if (RetAI.getInReg())
      SRETAttrs.addAttribute(llvm::Attribute::InReg);
    PAL.push_back(llvm::
                  AttributeSet::get(getLLVMContext(), Index, SRETAttrs));

    ++Index;
    // sret disables readnone and readonly
    FuncAttrs.removeAttribute(llvm::Attribute::ReadOnly)
      .removeAttribute(llvm::Attribute::ReadNone);
    break;
  }

  case ABIArgInfo::Expand:
    llvm_unreachable("Invalid ABI kind for return argument");
  }

  if (RetAttrs.hasAttributes())
    PAL.push_back(llvm::
                  AttributeSet::get(getLLVMContext(),
                                    llvm::AttributeSet::ReturnIndex,
                                    RetAttrs));

  for (CGFunctionInfo::const_arg_iterator it = FI.arg_begin(),
         ie = FI.arg_end(); it != ie; ++it) {
    QualType ParamType = it->type;
    const ABIArgInfo &AI = it->info;
    llvm::AttrBuilder Attrs;

    if (AI.getPaddingType()) {
      if (AI.getPaddingInReg())
        PAL.push_back(llvm::AttributeSet::get(getLLVMContext(), Index,
                                              llvm::Attribute::InReg));
      // Increment Index if there is padding.
      ++Index;
    }

    // 'restrict' -> 'noalias' is done in EmitFunctionProlog when we
    // have the corresponding parameter variable.  It doesn't make
    // sense to do it here because parameters are so messed up.
    switch (AI.getKind()) {
    case ABIArgInfo::Extend:
      if (ParamType->isSignedIntegerOrEnumerationType())
        Attrs.addAttribute(llvm::Attribute::SExt);
      else if (ParamType->isUnsignedIntegerOrEnumerationType())
        Attrs.addAttribute(llvm::Attribute::ZExt);
      // FALL THROUGH
    case ABIArgInfo::Direct:
      if (AI.getInReg())
        Attrs.addAttribute(llvm::Attribute::InReg);

      // FIXME: handle sseregparm someday...

      if (llvm::StructType *STy =
          dyn_cast<llvm::StructType>(AI.getCoerceToType())) {
        unsigned Extra = STy->getNumElements()-1;  // 1 will be added below.
        if (Attrs.hasAttributes())
          for (unsigned I = 0; I < Extra; ++I)
            PAL.push_back(llvm::AttributeSet::get(getLLVMContext(), Index + I,
                                                  Attrs));
        Index += Extra;
      }
      break;

    case ABIArgInfo::Indirect:
      if (AI.getInReg())
        Attrs.addAttribute(llvm::Attribute::InReg);

      if (AI.getIndirectByVal())
        Attrs.addAttribute(llvm::Attribute::ByVal);

      Attrs.addAlignmentAttr(AI.getIndirectAlign());

      // byval disables readnone and readonly.
      FuncAttrs.removeAttribute(llvm::Attribute::ReadOnly)
        .removeAttribute(llvm::Attribute::ReadNone);
      break;

    case ABIArgInfo::Ignore:
      // Skip increment, no matching LLVM parameter.
      continue;

    case ABIArgInfo::Expand: {
      SmallVector<llvm::Type*, 8> types;
      // FIXME: This is rather inefficient. Do we ever actually need to do
      // anything here? The result should be just reconstructed on the other
      // side, so extension should be a non-issue.
      getTypes().GetExpandedTypes(ParamType, types);
      Index += types.size();
      continue;
    }
    }

    if (Attrs.hasAttributes())
      PAL.push_back(llvm::AttributeSet::get(getLLVMContext(), Index, Attrs));
    ++Index;
  }
  if (FuncAttrs.hasAttributes())
    PAL.push_back(llvm::
                  AttributeSet::get(getLLVMContext(),
                                    llvm::AttributeSet::FunctionIndex,
                                    FuncAttrs));
}

/// An argument came in as a promoted argument; demote it back to its
/// declared type.
static llvm::Value *emitArgumentDemotion(CodeGenFunction &CGF,
                                         const VarDecl *var,
                                         llvm::Value *value) {
  llvm::Type *varType = CGF.ConvertType(var->getType());

  // This can happen with promotions that actually don't change the
  // underlying type, like the enum promotions.
  if (value->getType() == varType) return value;

  assert((varType->isIntegerTy() || varType->isFloatingPointTy())
         && "unexpected promotion type");

  if (isa<llvm::IntegerType>(varType))
    return CGF.Builder.CreateTrunc(value, varType, "arg.unpromote");

  return CGF.Builder.CreateFPCast(value, varType, "arg.unpromote");
}

void CodeGenFunction::EmitFunctionProlog(const CGFunctionInfo &FI,
                                         llvm::Function *Fn,
                                         const FunctionArgList &Args) {
  // If this is an implicit-return-zero function, go ahead and
  // initialize the return value.  TODO: it might be nice to have
  // a more general mechanism for this that didn't require synthesized
  // return statements.
  if (const FunctionDecl *FD = dyn_cast_or_null<FunctionDecl>(CurCodeDecl)) {
    if (FD->hasImplicitReturnZero()) {
      QualType RetTy = FD->getResultType().getUnqualifiedType();
      llvm::Type* LLVMTy = CGM.getTypes().ConvertType(RetTy);
      llvm::Constant* Zero = llvm::Constant::getNullValue(LLVMTy);
      Builder.CreateStore(Zero, ReturnValue);
    }
  }

  // FIXME: We no longer need the types from FunctionArgList; lift up and
  // simplify.

  // Emit allocs for param decls.  Give the LLVM Argument nodes names.
  llvm::Function::arg_iterator AI = Fn->arg_begin();

  // Name the struct return argument.
  if (CGM.ReturnTypeUsesSRet(FI)) {
    AI->setName("agg.result");
    AI->addAttr(llvm::AttributeSet::get(getLLVMContext(),
                                        AI->getArgNo() + 1,
                                        llvm::Attribute::NoAlias));
    ++AI;
  }

  // Create a pointer value for every parameter declaration.  This usually
  // entails copying one or more LLVM IR arguments into an alloca.  Don't push
  // any cleanups or do anything that might unwind.  We do that separately, so
  // we can push the cleanups in the correct order for the ABI.
  SmallVector<llvm::Value *, 16> ArgVals;
  ArgVals.reserve(Args.size());
  assert(FI.arg_size() == Args.size() &&
         "Mismatch between function signature & arguments.");
  unsigned ArgNo = 1;
  CGFunctionInfo::const_arg_iterator info_it = FI.arg_begin();
  for (FunctionArgList::const_iterator i = Args.begin(), e = Args.end(); 
       i != e; ++i, ++info_it, ++ArgNo) {
    const VarDecl *Arg = *i;
    QualType Ty = info_it->type;
    const ABIArgInfo &ArgI = info_it->info;

    bool isPromoted =
      isa<ParmVarDecl>(Arg) && cast<ParmVarDecl>(Arg)->isKNRPromoted();

    // Skip the dummy padding argument.
    if (ArgI.getPaddingType())
      ++AI;

    switch (ArgI.getKind()) {
    case ABIArgInfo::Indirect: {
      llvm::Value *V = AI;

      if (!hasScalarEvaluationKind(Ty)) {
        // Aggregates and complex variables are accessed by reference.  All we
        // need to do is realign the value, if requested
        if (ArgI.getIndirectRealign()) {
          llvm::Value *AlignedTemp = CreateMemTemp(Ty, "coerce");

          // Copy from the incoming argument pointer to the temporary with the
          // appropriate alignment.
          //
          // FIXME: We should have a common utility for generating an aggregate
          // copy.
          llvm::Type *I8PtrTy = Builder.getInt8PtrTy();
          CharUnits Size = getContext().getTypeSizeInChars(Ty);
          llvm::Value *Dst = Builder.CreateBitCast(AlignedTemp, I8PtrTy);
          llvm::Value *Src = Builder.CreateBitCast(V, I8PtrTy);
          Builder.CreateMemCpy(Dst,
                               Src,
                               llvm::ConstantInt::get(IntPtrTy, 
                                                      Size.getQuantity()),
                               ArgI.getIndirectAlign(),
                               false);
          V = AlignedTemp;
        }
      } else {
        // Load scalar value from indirect argument.
        CharUnits Alignment = getContext().getTypeAlignInChars(Ty);
        V = EmitLoadOfScalar(V, false, Alignment.getQuantity(), Ty,
                             Arg->getLocStart());

        if (isPromoted)
          V = emitArgumentDemotion(*this, Arg, V);
      }
      ArgVals.push_back(V);
      break;
    }

    case ABIArgInfo::Extend:
    case ABIArgInfo::Direct: {

      // If we have the trivial case, handle it with no muss and fuss.
      if (!isa<llvm::StructType>(ArgI.getCoerceToType()) &&
          ArgI.getCoerceToType() == ConvertType(Ty) &&
          ArgI.getDirectOffset() == 0) {
        assert(AI != Fn->arg_end() && "Argument mismatch!");
        llvm::Value *V = AI;

        if (Arg->getType().isRestrictQualified())
          AI->addAttr(llvm::AttributeSet::get(getLLVMContext(),
                                              AI->getArgNo() + 1,
                                              llvm::Attribute::NoAlias));

        // Ensure the argument is the correct type.
        if (V->getType() != ArgI.getCoerceToType())
          V = Builder.CreateBitCast(V, ArgI.getCoerceToType());

        if (isPromoted)
          V = emitArgumentDemotion(*this, Arg, V);

        if (const CXXMethodDecl *MD =
            dyn_cast_or_null<CXXMethodDecl>(CurCodeDecl)) {
          if (MD->isVirtual() && Arg == CXXABIThisDecl)
            V = CGM.getCXXABI().
                adjustThisParameterInVirtualFunctionPrologue(*this, CurGD, V);
        }

        // Because of merging of function types from multiple decls it is
        // possible for the type of an argument to not match the corresponding
        // type in the function type. Since we are codegening the callee
        // in here, add a cast to the argument type.
        llvm::Type *LTy = ConvertType(Arg->getType());
        if (V->getType() != LTy)
          V = Builder.CreateBitCast(V, LTy);

        ArgVals.push_back(V);
        break;
      }

      llvm::AllocaInst *Alloca = CreateMemTemp(Ty, Arg->getName());

      // The alignment we need to use is the max of the requested alignment for
      // the argument plus the alignment required by our access code below.
      unsigned AlignmentToUse =
        CGM.getDataLayout().getABITypeAlignment(ArgI.getCoerceToType());
      AlignmentToUse = std::max(AlignmentToUse,
                        (unsigned)getContext().getDeclAlign(Arg).getQuantity());

      Alloca->setAlignment(AlignmentToUse);
      llvm::Value *V = Alloca;
      llvm::Value *Ptr = V;    // Pointer to store into.

      // If the value is offset in memory, apply the offset now.
      if (unsigned Offs = ArgI.getDirectOffset()) {
        Ptr = Builder.CreateBitCast(Ptr, Builder.getInt8PtrTy());
        Ptr = Builder.CreateConstGEP1_32(Ptr, Offs);
        Ptr = Builder.CreateBitCast(Ptr,
                          llvm::PointerType::getUnqual(ArgI.getCoerceToType()));
      }

      // If the coerce-to type is a first class aggregate, we flatten it and
      // pass the elements. Either way is semantically identical, but fast-isel
      // and the optimizer generally likes scalar values better than FCAs.
      llvm::StructType *STy = dyn_cast<llvm::StructType>(ArgI.getCoerceToType());
      if (STy && STy->getNumElements() > 1) {
        uint64_t SrcSize = CGM.getDataLayout().getTypeAllocSize(STy);
        llvm::Type *DstTy =
          cast<llvm::PointerType>(Ptr->getType())->getElementType();
        uint64_t DstSize = CGM.getDataLayout().getTypeAllocSize(DstTy);

        if (SrcSize <= DstSize) {
          Ptr = Builder.CreateBitCast(Ptr, llvm::PointerType::getUnqual(STy));

          for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
            assert(AI != Fn->arg_end() && "Argument mismatch!");
            AI->setName(Arg->getName() + ".coerce" + Twine(i));
            llvm::Value *EltPtr = Builder.CreateConstGEP2_32(Ptr, 0, i);
            Builder.CreateStore(AI++, EltPtr);
          }
        } else {
          llvm::AllocaInst *TempAlloca =
            CreateTempAlloca(ArgI.getCoerceToType(), "coerce");
          TempAlloca->setAlignment(AlignmentToUse);
          llvm::Value *TempV = TempAlloca;

          for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
            assert(AI != Fn->arg_end() && "Argument mismatch!");
            AI->setName(Arg->getName() + ".coerce" + Twine(i));
            llvm::Value *EltPtr = Builder.CreateConstGEP2_32(TempV, 0, i);
            Builder.CreateStore(AI++, EltPtr);
          }

          Builder.CreateMemCpy(Ptr, TempV, DstSize, AlignmentToUse);
        }
      } else {
        // Simple case, just do a coerced store of the argument into the alloca.
        assert(AI != Fn->arg_end() && "Argument mismatch!");
        AI->setName(Arg->getName() + ".coerce");
        CreateCoercedStore(AI++, Ptr, /*DestIsVolatile=*/false, *this);
      }


      // Match to what EmitParmDecl is expecting for this type.
      if (CodeGenFunction::hasScalarEvaluationKind(Ty)) {
        V = EmitLoadOfScalar(V, false, AlignmentToUse, Ty, Arg->getLocStart());
        if (isPromoted)
          V = emitArgumentDemotion(*this, Arg, V);
      }
      ArgVals.push_back(V);
      continue;  // Skip ++AI increment, already done.
    }

    case ABIArgInfo::Expand: {
      // If this structure was expanded into multiple arguments then
      // we need to create a temporary and reconstruct it from the
      // arguments.
      llvm::AllocaInst *Alloca = CreateMemTemp(Ty);
      CharUnits Align = getContext().getDeclAlign(Arg);
      Alloca->setAlignment(Align.getQuantity());
      LValue LV = MakeAddrLValue(Alloca, Ty, Align);
      llvm::Function::arg_iterator End = ExpandTypeFromArgs(Ty, LV, AI);
      ArgVals.push_back(Alloca);

      // Name the arguments used in expansion and increment AI.
      unsigned Index = 0;
      for (; AI != End; ++AI, ++Index)
        AI->setName(Arg->getName() + "." + Twine(Index));
      continue;
    }

    case ABIArgInfo::Ignore:
      // Initialize the local variable appropriately.
      if (!hasScalarEvaluationKind(Ty))
        ArgVals.push_back(CreateMemTemp(Ty));
      else
        ArgVals.push_back(llvm::UndefValue::get(ConvertType(Arg->getType())));

      // Skip increment, no matching LLVM parameter.
      continue;
    }

    ++AI;
  }
  assert(AI == Fn->arg_end() && "Argument mismatch!");

  if (getTarget().getCXXABI().areArgsDestroyedLeftToRightInCallee()) {
    for (int I = Args.size() - 1; I >= 0; --I)
      EmitParmDecl(*Args[I], ArgVals[I], I + 1);
  } else {
    for (unsigned I = 0, E = Args.size(); I != E; ++I)
      EmitParmDecl(*Args[I], ArgVals[I], I + 1);
  }
}

static void eraseUnusedBitCasts(llvm::Instruction *insn) {
  while (insn->use_empty()) {
    llvm::BitCastInst *bitcast = dyn_cast<llvm::BitCastInst>(insn);
    if (!bitcast) return;

    // This is "safe" because we would have used a ConstantExpr otherwise.
    insn = cast<llvm::Instruction>(bitcast->getOperand(0));
    bitcast->eraseFromParent();
  }
}

/// Try to emit a fused autorelease of a return result.
static llvm::Value *tryEmitFusedAutoreleaseOfResult(CodeGenFunction &CGF,
                                                    llvm::Value *result) {
  // We must be immediately followed the cast.
  llvm::BasicBlock *BB = CGF.Builder.GetInsertBlock();
  if (BB->empty()) return 0;
  if (&BB->back() != result) return 0;

  llvm::Type *resultType = result->getType();

  // result is in a BasicBlock and is therefore an Instruction.
  llvm::Instruction *generator = cast<llvm::Instruction>(result);

  SmallVector<llvm::Instruction*,4> insnsToKill;

  // Look for:
  //  %generator = bitcast %type1* %generator2 to %type2*
  while (llvm::BitCastInst *bitcast = dyn_cast<llvm::BitCastInst>(generator)) {
    // We would have emitted this as a constant if the operand weren't
    // an Instruction.
    generator = cast<llvm::Instruction>(bitcast->getOperand(0));

    // Require the generator to be immediately followed by the cast.
    if (generator->getNextNode() != bitcast)
      return 0;

    insnsToKill.push_back(bitcast);
  }

  // Look for:
  //   %generator = call i8* @objc_retain(i8* %originalResult)
  // or
  //   %generator = call i8* @objc_retainAutoreleasedReturnValue(i8* %originalResult)
  llvm::CallInst *call = dyn_cast<llvm::CallInst>(generator);
  if (!call) return 0;

  bool doRetainAutorelease;

  if (call->getCalledValue() == CGF.CGM.getARCEntrypoints().objc_retain) {
    doRetainAutorelease = true;
  } else if (call->getCalledValue() == CGF.CGM.getARCEntrypoints()
                                          .objc_retainAutoreleasedReturnValue) {
    doRetainAutorelease = false;

    // If we emitted an assembly marker for this call (and the
    // ARCEntrypoints field should have been set if so), go looking
    // for that call.  If we can't find it, we can't do this
    // optimization.  But it should always be the immediately previous
    // instruction, unless we needed bitcasts around the call.
    if (CGF.CGM.getARCEntrypoints().retainAutoreleasedReturnValueMarker) {
      llvm::Instruction *prev = call->getPrevNode();
      assert(prev);
      if (isa<llvm::BitCastInst>(prev)) {
        prev = prev->getPrevNode();
        assert(prev);
      }
      assert(isa<llvm::CallInst>(prev));
      assert(cast<llvm::CallInst>(prev)->getCalledValue() ==
               CGF.CGM.getARCEntrypoints().retainAutoreleasedReturnValueMarker);
      insnsToKill.push_back(prev);
    }
  } else {
    return 0;
  }

  result = call->getArgOperand(0);
  insnsToKill.push_back(call);

  // Keep killing bitcasts, for sanity.  Note that we no longer care
  // about precise ordering as long as there's exactly one use.
  while (llvm::BitCastInst *bitcast = dyn_cast<llvm::BitCastInst>(result)) {
    if (!bitcast->hasOneUse()) break;
    insnsToKill.push_back(bitcast);
    result = bitcast->getOperand(0);
  }

  // Delete all the unnecessary instructions, from latest to earliest.
  for (SmallVectorImpl<llvm::Instruction*>::iterator
         i = insnsToKill.begin(), e = insnsToKill.end(); i != e; ++i)
    (*i)->eraseFromParent();

  // Do the fused retain/autorelease if we were asked to.
  if (doRetainAutorelease)
    result = CGF.EmitARCRetainAutoreleaseReturnValue(result);

  // Cast back to the result type.
  return CGF.Builder.CreateBitCast(result, resultType);
}

/// If this is a +1 of the value of an immutable 'self', remove it.
static llvm::Value *tryRemoveRetainOfSelf(CodeGenFunction &CGF,
                                          llvm::Value *result) {
  // This is only applicable to a method with an immutable 'self'.
  const ObjCMethodDecl *method =
    dyn_cast_or_null<ObjCMethodDecl>(CGF.CurCodeDecl);
  if (!method) return 0;
  const VarDecl *self = method->getSelfDecl();
  if (!self->getType().isConstQualified()) return 0;

  // Look for a retain call.
  llvm::CallInst *retainCall =
    dyn_cast<llvm::CallInst>(result->stripPointerCasts());
  if (!retainCall ||
      retainCall->getCalledValue() != CGF.CGM.getARCEntrypoints().objc_retain)
    return 0;

  // Look for an ordinary load of 'self'.
  llvm::Value *retainedValue = retainCall->getArgOperand(0);
  llvm::LoadInst *load =
    dyn_cast<llvm::LoadInst>(retainedValue->stripPointerCasts());
  if (!load || load->isAtomic() || load->isVolatile() || 
      load->getPointerOperand() != CGF.GetAddrOfLocalVar(self))
    return 0;

  // Okay!  Burn it all down.  This relies for correctness on the
  // assumption that the retain is emitted as part of the return and
  // that thereafter everything is used "linearly".
  llvm::Type *resultType = result->getType();
  eraseUnusedBitCasts(cast<llvm::Instruction>(result));
  assert(retainCall->use_empty());
  retainCall->eraseFromParent();
  eraseUnusedBitCasts(cast<llvm::Instruction>(retainedValue));

  return CGF.Builder.CreateBitCast(load, resultType);
}

/// Emit an ARC autorelease of the result of a function.
///
/// \return the value to actually return from the function
static llvm::Value *emitAutoreleaseOfResult(CodeGenFunction &CGF,
                                            llvm::Value *result) {
  // If we're returning 'self', kill the initial retain.  This is a
  // heuristic attempt to "encourage correctness" in the really unfortunate
  // case where we have a return of self during a dealloc and we desperately
  // need to avoid the possible autorelease.
  if (llvm::Value *self = tryRemoveRetainOfSelf(CGF, result))
    return self;

  // At -O0, try to emit a fused retain/autorelease.
  if (CGF.shouldUseFusedARCCalls())
    if (llvm::Value *fused = tryEmitFusedAutoreleaseOfResult(CGF, result))
      return fused;

  return CGF.EmitARCAutoreleaseReturnValue(result);
}

/// Heuristically search for a dominating store to the return-value slot.
static llvm::StoreInst *findDominatingStoreToReturnValue(CodeGenFunction &CGF) {
  // If there are multiple uses of the return-value slot, just check
  // for something immediately preceding the IP.  Sometimes this can
  // happen with how we generate implicit-returns; it can also happen
  // with noreturn cleanups.
  if (!CGF.ReturnValue->hasOneUse()) {
    llvm::BasicBlock *IP = CGF.Builder.GetInsertBlock();
    if (IP->empty()) return 0;
    llvm::StoreInst *store = dyn_cast<llvm::StoreInst>(&IP->back());
    if (!store) return 0;
    if (store->getPointerOperand() != CGF.ReturnValue) return 0;
    assert(!store->isAtomic() && !store->isVolatile()); // see below
    return store;
  }

  llvm::StoreInst *store =
    dyn_cast<llvm::StoreInst>(CGF.ReturnValue->use_back());
  if (!store) return 0;

  // These aren't actually possible for non-coerced returns, and we
  // only care about non-coerced returns on this code path.
  assert(!store->isAtomic() && !store->isVolatile());

  // Now do a first-and-dirty dominance check: just walk up the
  // single-predecessors chain from the current insertion point.
  llvm::BasicBlock *StoreBB = store->getParent();
  llvm::BasicBlock *IP = CGF.Builder.GetInsertBlock();
  while (IP != StoreBB) {
    if (!(IP = IP->getSinglePredecessor()))
      return 0;
  }

  // Okay, the store's basic block dominates the insertion point; we
  // can do our thing.
  return store;
}

void CodeGenFunction::EmitFunctionEpilog(const CGFunctionInfo &FI,
                                         bool EmitRetDbgLoc,
                                         SourceLocation EndLoc) {
  // Functions with no result always return void.
  if (ReturnValue == 0) {
    Builder.CreateRetVoid();
    return;
  }

  llvm::DebugLoc RetDbgLoc;
  llvm::Value *RV = 0;
  QualType RetTy = FI.getReturnType();
  const ABIArgInfo &RetAI = FI.getReturnInfo();

  switch (RetAI.getKind()) {
  case ABIArgInfo::Indirect: {
    switch (getEvaluationKind(RetTy)) {
    case TEK_Complex: {
      ComplexPairTy RT =
        EmitLoadOfComplex(MakeNaturalAlignAddrLValue(ReturnValue, RetTy),
                          EndLoc);
      EmitStoreOfComplex(RT,
                       MakeNaturalAlignAddrLValue(CurFn->arg_begin(), RetTy),
                         /*isInit*/ true);
      break;
    }
    case TEK_Aggregate:
      // Do nothing; aggregrates get evaluated directly into the destination.
      break;
    case TEK_Scalar:
      EmitStoreOfScalar(Builder.CreateLoad(ReturnValue),
                        MakeNaturalAlignAddrLValue(CurFn->arg_begin(), RetTy),
                        /*isInit*/ true);
      break;
    }
    break;
  }

  case ABIArgInfo::Extend:
  case ABIArgInfo::Direct:
    if (RetAI.getCoerceToType() == ConvertType(RetTy) &&
        RetAI.getDirectOffset() == 0) {
      // The internal return value temp always will have pointer-to-return-type
      // type, just do a load.

      // If there is a dominating store to ReturnValue, we can elide
      // the load, zap the store, and usually zap the alloca.
      if (llvm::StoreInst *SI = findDominatingStoreToReturnValue(*this)) {
        // Reuse the debug location from the store unless there is
        // cleanup code to be emitted between the store and return
        // instruction.
        if (EmitRetDbgLoc && !AutoreleaseResult)
          RetDbgLoc = SI->getDebugLoc();
        // Get the stored value and nuke the now-dead store.
        RV = SI->getValueOperand();
        SI->eraseFromParent();

        // If that was the only use of the return value, nuke it as well now.
        if (ReturnValue->use_empty() && isa<llvm::AllocaInst>(ReturnValue)) {
          cast<llvm::AllocaInst>(ReturnValue)->eraseFromParent();
          ReturnValue = 0;
        }

      // Otherwise, we have to do a simple load.
      } else {
        RV = Builder.CreateLoad(ReturnValue);
      }
    } else {
      llvm::Value *V = ReturnValue;
      // If the value is offset in memory, apply the offset now.
      if (unsigned Offs = RetAI.getDirectOffset()) {
        V = Builder.CreateBitCast(V, Builder.getInt8PtrTy());
        V = Builder.CreateConstGEP1_32(V, Offs);
        V = Builder.CreateBitCast(V,
                         llvm::PointerType::getUnqual(RetAI.getCoerceToType()));
      }

      RV = CreateCoercedLoad(V, RetAI.getCoerceToType(), *this);
    }

    // In ARC, end functions that return a retainable type with a call
    // to objc_autoreleaseReturnValue.
    if (AutoreleaseResult) {
      assert(getLangOpts().ObjCAutoRefCount &&
             !FI.isReturnsRetained() &&
             RetTy->isObjCRetainableType());
      RV = emitAutoreleaseOfResult(*this, RV);
    }

    break;

  case ABIArgInfo::Ignore:
    break;

  case ABIArgInfo::Expand:
    llvm_unreachable("Invalid ABI kind for return argument");
  }

  llvm::Instruction *Ret = RV ? Builder.CreateRet(RV) : Builder.CreateRetVoid();
  if (!RetDbgLoc.isUnknown())
    Ret->setDebugLoc(RetDbgLoc);
}

void CodeGenFunction::EmitDelegateCallArg(CallArgList &args,
                                          const VarDecl *param,
                                          SourceLocation loc) {
  // StartFunction converted the ABI-lowered parameter(s) into a
  // local alloca.  We need to turn that into an r-value suitable
  // for EmitCall.
  llvm::Value *local = GetAddrOfLocalVar(param);

  QualType type = param->getType();

  // For the most part, we just need to load the alloca, except:
  // 1) aggregate r-values are actually pointers to temporaries, and
  // 2) references to non-scalars are pointers directly to the aggregate.
  // I don't know why references to scalars are different here.
  if (const ReferenceType *ref = type->getAs<ReferenceType>()) {
    if (!hasScalarEvaluationKind(ref->getPointeeType()))
      return args.add(RValue::getAggregate(local), type);

    // Locals which are references to scalars are represented
    // with allocas holding the pointer.
    return args.add(RValue::get(Builder.CreateLoad(local)), type);
  }

  args.add(convertTempToRValue(local, type, loc), type);
}

static bool isProvablyNull(llvm::Value *addr) {
  return isa<llvm::ConstantPointerNull>(addr);
}

static bool isProvablyNonNull(llvm::Value *addr) {
  return isa<llvm::AllocaInst>(addr);
}

/// Emit the actual writing-back of a writeback.
static void emitWriteback(CodeGenFunction &CGF,
                          const CallArgList::Writeback &writeback) {
  const LValue &srcLV = writeback.Source;
  llvm::Value *srcAddr = srcLV.getAddress();
  assert(!isProvablyNull(srcAddr) &&
         "shouldn't have writeback for provably null argument");

  llvm::BasicBlock *contBB = 0;

  // If the argument wasn't provably non-null, we need to null check
  // before doing the store.
  bool provablyNonNull = isProvablyNonNull(srcAddr);
  if (!provablyNonNull) {
    llvm::BasicBlock *writebackBB = CGF.createBasicBlock("icr.writeback");
    contBB = CGF.createBasicBlock("icr.done");

    llvm::Value *isNull = CGF.Builder.CreateIsNull(srcAddr, "icr.isnull");
    CGF.Builder.CreateCondBr(isNull, contBB, writebackBB);
    CGF.EmitBlock(writebackBB);
  }

  // Load the value to writeback.
  llvm::Value *value = CGF.Builder.CreateLoad(writeback.Temporary);

  // Cast it back, in case we're writing an id to a Foo* or something.
  value = CGF.Builder.CreateBitCast(value,
               cast<llvm::PointerType>(srcAddr->getType())->getElementType(),
                            "icr.writeback-cast");
  
  // Perform the writeback.

  // If we have a "to use" value, it's something we need to emit a use
  // of.  This has to be carefully threaded in: if it's done after the
  // release it's potentially undefined behavior (and the optimizer
  // will ignore it), and if it happens before the retain then the
  // optimizer could move the release there.
  if (writeback.ToUse) {
    assert(srcLV.getObjCLifetime() == Qualifiers::OCL_Strong);

    // Retain the new value.  No need to block-copy here:  the block's
    // being passed up the stack.
    value = CGF.EmitARCRetainNonBlock(value);

    // Emit the intrinsic use here.
    CGF.EmitARCIntrinsicUse(writeback.ToUse);

    // Load the old value (primitively).
    llvm::Value *oldValue = CGF.EmitLoadOfScalar(srcLV, SourceLocation());

    // Put the new value in place (primitively).
    CGF.EmitStoreOfScalar(value, srcLV, /*init*/ false);

    // Release the old value.
    CGF.EmitARCRelease(oldValue, srcLV.isARCPreciseLifetime());

  // Otherwise, we can just do a normal lvalue store.
  } else {
    CGF.EmitStoreThroughLValue(RValue::get(value), srcLV);
  }

  // Jump to the continuation block.
  if (!provablyNonNull)
    CGF.EmitBlock(contBB);
}

static void emitWritebacks(CodeGenFunction &CGF,
                           const CallArgList &args) {
  for (CallArgList::writeback_iterator
         i = args.writeback_begin(), e = args.writeback_end(); i != e; ++i)
    emitWriteback(CGF, *i);
}

static void deactivateArgCleanupsBeforeCall(CodeGenFunction &CGF,
                                            const CallArgList &CallArgs) {
  assert(CGF.getTarget().getCXXABI().areArgsDestroyedLeftToRightInCallee());
  ArrayRef<CallArgList::CallArgCleanup> Cleanups =
    CallArgs.getCleanupsToDeactivate();
  // Iterate in reverse to increase the likelihood of popping the cleanup.
  for (ArrayRef<CallArgList::CallArgCleanup>::reverse_iterator
         I = Cleanups.rbegin(), E = Cleanups.rend(); I != E; ++I) {
    CGF.DeactivateCleanupBlock(I->Cleanup, I->IsActiveIP);
    I->IsActiveIP->eraseFromParent();
  }
}

static const Expr *maybeGetUnaryAddrOfOperand(const Expr *E) {
  if (const UnaryOperator *uop = dyn_cast<UnaryOperator>(E->IgnoreParens()))
    if (uop->getOpcode() == UO_AddrOf)
      return uop->getSubExpr();
  return 0;
}

/// Emit an argument that's being passed call-by-writeback.  That is,
/// we are passing the address of 
static void emitWritebackArg(CodeGenFunction &CGF, CallArgList &args,
                             const ObjCIndirectCopyRestoreExpr *CRE) {
  LValue srcLV;

  // Make an optimistic effort to emit the address as an l-value.
  // This can fail if the the argument expression is more complicated.
  if (const Expr *lvExpr = maybeGetUnaryAddrOfOperand(CRE->getSubExpr())) {
    srcLV = CGF.EmitLValue(lvExpr);

  // Otherwise, just emit it as a scalar.
  } else {
    llvm::Value *srcAddr = CGF.EmitScalarExpr(CRE->getSubExpr());

    QualType srcAddrType =
      CRE->getSubExpr()->getType()->castAs<PointerType>()->getPointeeType();
    srcLV = CGF.MakeNaturalAlignAddrLValue(srcAddr, srcAddrType);
  }
  llvm::Value *srcAddr = srcLV.getAddress();

  // The dest and src types don't necessarily match in LLVM terms
  // because of the crazy ObjC compatibility rules.

  llvm::PointerType *destType =
    cast<llvm::PointerType>(CGF.ConvertType(CRE->getType()));

  // If the address is a constant null, just pass the appropriate null.
  if (isProvablyNull(srcAddr)) {
    args.add(RValue::get(llvm::ConstantPointerNull::get(destType)),
             CRE->getType());
    return;
  }

  // Create the temporary.
  llvm::Value *temp = CGF.CreateTempAlloca(destType->getElementType(),
                                           "icr.temp");
  // Loading an l-value can introduce a cleanup if the l-value is __weak,
  // and that cleanup will be conditional if we can't prove that the l-value
  // isn't null, so we need to register a dominating point so that the cleanups
  // system will make valid IR.
  CodeGenFunction::ConditionalEvaluation condEval(CGF);
  
  // Zero-initialize it if we're not doing a copy-initialization.
  bool shouldCopy = CRE->shouldCopy();
  if (!shouldCopy) {
    llvm::Value *null =
      llvm::ConstantPointerNull::get(
        cast<llvm::PointerType>(destType->getElementType()));
    CGF.Builder.CreateStore(null, temp);
  }
  
  llvm::BasicBlock *contBB = 0;
  llvm::BasicBlock *originBB = 0;

  // If the address is *not* known to be non-null, we need to switch.
  llvm::Value *finalArgument;

  bool provablyNonNull = isProvablyNonNull(srcAddr);
  if (provablyNonNull) {
    finalArgument = temp;
  } else {
    llvm::Value *isNull = CGF.Builder.CreateIsNull(srcAddr, "icr.isnull");

    finalArgument = CGF.Builder.CreateSelect(isNull, 
                                   llvm::ConstantPointerNull::get(destType),
                                             temp, "icr.argument");

    // If we need to copy, then the load has to be conditional, which
    // means we need control flow.
    if (shouldCopy) {
      originBB = CGF.Builder.GetInsertBlock();
      contBB = CGF.createBasicBlock("icr.cont");
      llvm::BasicBlock *copyBB = CGF.createBasicBlock("icr.copy");
      CGF.Builder.CreateCondBr(isNull, contBB, copyBB);
      CGF.EmitBlock(copyBB);
      condEval.begin(CGF);
    }
  }

  llvm::Value *valueToUse = 0;

  // Perform a copy if necessary.
  if (shouldCopy) {
    RValue srcRV = CGF.EmitLoadOfLValue(srcLV, SourceLocation());
    assert(srcRV.isScalar());

    llvm::Value *src = srcRV.getScalarVal();
    src = CGF.Builder.CreateBitCast(src, destType->getElementType(),
                                    "icr.cast");

    // Use an ordinary store, not a store-to-lvalue.
    CGF.Builder.CreateStore(src, temp);

    // If optimization is enabled, and the value was held in a
    // __strong variable, we need to tell the optimizer that this
    // value has to stay alive until we're doing the store back.
    // This is because the temporary is effectively unretained,
    // and so otherwise we can violate the high-level semantics.
    if (CGF.CGM.getCodeGenOpts().OptimizationLevel != 0 &&
        srcLV.getObjCLifetime() == Qualifiers::OCL_Strong) {
      valueToUse = src;
    }
  }
  
  // Finish the control flow if we needed it.
  if (shouldCopy && !provablyNonNull) {
    llvm::BasicBlock *copyBB = CGF.Builder.GetInsertBlock();
    CGF.EmitBlock(contBB);

    // Make a phi for the value to intrinsically use.
    if (valueToUse) {
      llvm::PHINode *phiToUse = CGF.Builder.CreatePHI(valueToUse->getType(), 2,
                                                      "icr.to-use");
      phiToUse->addIncoming(valueToUse, copyBB);
      phiToUse->addIncoming(llvm::UndefValue::get(valueToUse->getType()),
                            originBB);
      valueToUse = phiToUse;
    }

    condEval.end(CGF);
  }

  args.addWriteback(srcLV, temp, valueToUse);
  args.add(RValue::get(finalArgument), CRE->getType());
}

void CodeGenFunction::EmitCallArgs(CallArgList &Args,
                                   ArrayRef<QualType> ArgTypes,
                                   CallExpr::const_arg_iterator ArgBeg,
                                   CallExpr::const_arg_iterator ArgEnd,
                                   bool ForceColumnInfo) {
  CGDebugInfo *DI = getDebugInfo();
  SourceLocation CallLoc;
  if (DI) CallLoc = DI->getLocation();

  // We *have* to evaluate arguments from right to left in the MS C++ ABI,
  // because arguments are destroyed left to right in the callee.
  if (CGM.getTarget().getCXXABI().areArgsDestroyedLeftToRightInCallee()) {
    size_t CallArgsStart = Args.size();
    for (int I = ArgTypes.size() - 1; I >= 0; --I) {
      CallExpr::const_arg_iterator Arg = ArgBeg + I;
      EmitCallArg(Args, *Arg, ArgTypes[I]);
      // Restore the debug location.
      if (DI) DI->EmitLocation(Builder, CallLoc, ForceColumnInfo);
    }

    // Un-reverse the arguments we just evaluated so they match up with the LLVM
    // IR function.
    std::reverse(Args.begin() + CallArgsStart, Args.end());
    return;
  }

  for (unsigned I = 0, E = ArgTypes.size(); I != E; ++I) {
    CallExpr::const_arg_iterator Arg = ArgBeg + I;
    assert(Arg != ArgEnd);
    EmitCallArg(Args, *Arg, ArgTypes[I]);
    // Restore the debug location.
    if (DI) DI->EmitLocation(Builder, CallLoc, ForceColumnInfo);
  }
}

void CodeGenFunction::EmitCallArg(CallArgList &args, const Expr *E,
                                  QualType type) {
  if (const ObjCIndirectCopyRestoreExpr *CRE
        = dyn_cast<ObjCIndirectCopyRestoreExpr>(E)) {
    assert(getLangOpts().ObjCAutoRefCount);
    assert(getContext().hasSameType(E->getType(), type));
    return emitWritebackArg(*this, args, CRE);
  }

  assert(type->isReferenceType() == E->isGLValue() &&
         "reference binding to unmaterialized r-value!");

  if (E->isGLValue()) {
    assert(E->getObjectKind() == OK_Ordinary);
    return args.add(EmitReferenceBindingToExpr(E), type);
  }

  bool HasAggregateEvalKind = hasAggregateEvaluationKind(type);

  // In the Microsoft C++ ABI, aggregate arguments are destructed by the callee.
  // However, we still have to push an EH-only cleanup in case we unwind before
  // we make it to the call.
  if (HasAggregateEvalKind &&
      CGM.getTarget().getCXXABI().areArgsDestroyedLeftToRightInCallee()) {
    const CXXRecordDecl *RD = type->getAsCXXRecordDecl();
    if (RD && RD->hasNonTrivialDestructor()) {
      AggValueSlot Slot = CreateAggTemp(type, "agg.arg.tmp");
      Slot.setExternallyDestructed();
      EmitAggExpr(E, Slot);
      RValue RV = Slot.asRValue();
      args.add(RV, type);

      pushDestroy(EHCleanup, RV.getAggregateAddr(), type, destroyCXXObject,
                  /*useEHCleanupForArray*/ true);
      // This unreachable is a temporary marker which will be removed later.
      llvm::Instruction *IsActive = Builder.CreateUnreachable();
      args.addArgCleanupDeactivation(EHStack.getInnermostEHScope(), IsActive);
      return;
    }
  }

  if (HasAggregateEvalKind && isa<ImplicitCastExpr>(E) &&
      cast<CastExpr>(E)->getCastKind() == CK_LValueToRValue) {
    LValue L = EmitLValue(cast<CastExpr>(E)->getSubExpr());
    assert(L.isSimple());
    if (L.getAlignment() >= getContext().getTypeAlignInChars(type)) {
      args.add(L.asAggregateRValue(), type, /*NeedsCopy*/true);
    } else {
      // We can't represent a misaligned lvalue in the CallArgList, so copy
      // to an aligned temporary now.
      llvm::Value *tmp = CreateMemTemp(type);
      EmitAggregateCopy(tmp, L.getAddress(), type, L.isVolatile(),
                        L.getAlignment());
      args.add(RValue::getAggregate(tmp), type);
    }
    return;
  }

  args.add(EmitAnyExprToTemp(E), type);
}

// In ObjC ARC mode with no ObjC ARC exception safety, tell the ARC
// optimizer it can aggressively ignore unwind edges.
void
CodeGenFunction::AddObjCARCExceptionMetadata(llvm::Instruction *Inst) {
  if (CGM.getCodeGenOpts().OptimizationLevel != 0 &&
      !CGM.getCodeGenOpts().ObjCAutoRefCountExceptions)
    Inst->setMetadata("clang.arc.no_objc_arc_exceptions",
                      CGM.getNoObjCARCExceptionsMetadata());
}

/// Emits a call to the given no-arguments nounwind runtime function.
llvm::CallInst *
CodeGenFunction::EmitNounwindRuntimeCall(llvm::Value *callee,
                                         const llvm::Twine &name) {
  return EmitNounwindRuntimeCall(callee, ArrayRef<llvm::Value*>(), name);
}

/// Emits a call to the given nounwind runtime function.
llvm::CallInst *
CodeGenFunction::EmitNounwindRuntimeCall(llvm::Value *callee,
                                         ArrayRef<llvm::Value*> args,
                                         const llvm::Twine &name) {
  llvm::CallInst *call = EmitRuntimeCall(callee, args, name);
  call->setDoesNotThrow();
  return call;
}

/// Emits a simple call (never an invoke) to the given no-arguments
/// runtime function.
llvm::CallInst *
CodeGenFunction::EmitRuntimeCall(llvm::Value *callee,
                                 const llvm::Twine &name) {
  return EmitRuntimeCall(callee, ArrayRef<llvm::Value*>(), name);
}

/// Emits a simple call (never an invoke) to the given runtime
/// function.
llvm::CallInst *
CodeGenFunction::EmitRuntimeCall(llvm::Value *callee,
                                 ArrayRef<llvm::Value*> args,
                                 const llvm::Twine &name) {
  llvm::CallInst *call = Builder.CreateCall(callee, args, name);
  call->setCallingConv(getRuntimeCC());
  return call;
}

/// Emits a call or invoke to the given noreturn runtime function.
void CodeGenFunction::EmitNoreturnRuntimeCallOrInvoke(llvm::Value *callee,
                                               ArrayRef<llvm::Value*> args) {
  if (getInvokeDest()) {
    llvm::InvokeInst *invoke = 
      Builder.CreateInvoke(callee,
                           getUnreachableBlock(),
                           getInvokeDest(),
                           args);
    invoke->setDoesNotReturn();
    invoke->setCallingConv(getRuntimeCC());
  } else {
    llvm::CallInst *call = Builder.CreateCall(callee, args);
    call->setDoesNotReturn();
    call->setCallingConv(getRuntimeCC());
    Builder.CreateUnreachable();
  }
  PGO.setCurrentRegionUnreachable();
}

/// Emits a call or invoke instruction to the given nullary runtime
/// function.
llvm::CallSite
CodeGenFunction::EmitRuntimeCallOrInvoke(llvm::Value *callee,
                                         const Twine &name) {
  return EmitRuntimeCallOrInvoke(callee, ArrayRef<llvm::Value*>(), name);
}

/// Emits a call or invoke instruction to the given runtime function.
llvm::CallSite
CodeGenFunction::EmitRuntimeCallOrInvoke(llvm::Value *callee,
                                         ArrayRef<llvm::Value*> args,
                                         const Twine &name) {
  llvm::CallSite callSite = EmitCallOrInvoke(callee, args, name);
  callSite.setCallingConv(getRuntimeCC());
  return callSite;
}

llvm::CallSite
CodeGenFunction::EmitCallOrInvoke(llvm::Value *Callee,
                                  const Twine &Name) {
  return EmitCallOrInvoke(Callee, ArrayRef<llvm::Value *>(), Name);
}

/// Emits a call or invoke instruction to the given function, depending
/// on the current state of the EH stack.
llvm::CallSite
CodeGenFunction::EmitCallOrInvoke(llvm::Value *Callee,
                                  ArrayRef<llvm::Value *> Args,
                                  const Twine &Name) {
  llvm::BasicBlock *InvokeDest = getInvokeDest();

  llvm::Instruction *Inst;
  if (!InvokeDest)
    Inst = Builder.CreateCall(Callee, Args, Name);
  else {
    llvm::BasicBlock *ContBB = createBasicBlock("invoke.cont");
    Inst = Builder.CreateInvoke(Callee, ContBB, InvokeDest, Args, Name);
    EmitBlock(ContBB);
  }

  // In ObjC ARC mode with no ObjC ARC exception safety, tell the ARC
  // optimizer it can aggressively ignore unwind edges.
  if (CGM.getLangOpts().ObjCAutoRefCount)
    AddObjCARCExceptionMetadata(Inst);

  return Inst;
}

static void checkArgMatches(llvm::Value *Elt, unsigned &ArgNo,
                            llvm::FunctionType *FTy) {
  if (ArgNo < FTy->getNumParams())
    assert(Elt->getType() == FTy->getParamType(ArgNo));
  else
    assert(FTy->isVarArg());
  ++ArgNo;
}

void CodeGenFunction::ExpandTypeToArgs(QualType Ty, RValue RV,
                                       SmallVectorImpl<llvm::Value *> &Args,
                                       llvm::FunctionType *IRFuncTy) {
  if (const ConstantArrayType *AT = getContext().getAsConstantArrayType(Ty)) {
    unsigned NumElts = AT->getSize().getZExtValue();
    QualType EltTy = AT->getElementType();
    llvm::Value *Addr = RV.getAggregateAddr();
    for (unsigned Elt = 0; Elt < NumElts; ++Elt) {
      llvm::Value *EltAddr = Builder.CreateConstGEP2_32(Addr, 0, Elt);
      RValue EltRV = convertTempToRValue(EltAddr, EltTy, SourceLocation());
      ExpandTypeToArgs(EltTy, EltRV, Args, IRFuncTy);
    }
  } else if (const RecordType *RT = Ty->getAs<RecordType>()) {
    RecordDecl *RD = RT->getDecl();
    assert(RV.isAggregate() && "Unexpected rvalue during struct expansion");
    LValue LV = MakeAddrLValue(RV.getAggregateAddr(), Ty);

    if (RD->isUnion()) {
      const FieldDecl *LargestFD = 0;
      CharUnits UnionSize = CharUnits::Zero();

      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        const FieldDecl *FD = *i;
        assert(!FD->isBitField() &&
               "Cannot expand structure with bit-field members.");
        CharUnits FieldSize = getContext().getTypeSizeInChars(FD->getType());
        if (UnionSize < FieldSize) {
          UnionSize = FieldSize;
          LargestFD = FD;
        }
      }
      if (LargestFD) {
        RValue FldRV = EmitRValueForField(LV, LargestFD, SourceLocation());
        ExpandTypeToArgs(LargestFD->getType(), FldRV, Args, IRFuncTy);
      }
    } else {
      for (RecordDecl::field_iterator i = RD->field_begin(), e = RD->field_end();
           i != e; ++i) {
        FieldDecl *FD = *i;

        RValue FldRV = EmitRValueForField(LV, FD, SourceLocation());
        ExpandTypeToArgs(FD->getType(), FldRV, Args, IRFuncTy);
      }
    }
  } else if (Ty->isAnyComplexType()) {
    ComplexPairTy CV = RV.getComplexVal();
    Args.push_back(CV.first);
    Args.push_back(CV.second);
  } else {
    assert(RV.isScalar() &&
           "Unexpected non-scalar rvalue during struct expansion.");

    // Insert a bitcast as needed.
    llvm::Value *V = RV.getScalarVal();
    if (Args.size() < IRFuncTy->getNumParams() &&
        V->getType() != IRFuncTy->getParamType(Args.size()))
      V = Builder.CreateBitCast(V, IRFuncTy->getParamType(Args.size()));

    Args.push_back(V);
  }
}


RValue CodeGenFunction::EmitCall(const CGFunctionInfo &CallInfo,
                                 llvm::Value *Callee,
                                 ReturnValueSlot ReturnValue,
                                 const CallArgList &CallArgs,
                                 const Decl *TargetDecl,
                                 llvm::Instruction **callOrInvoke) {
  // FIXME: We no longer need the types from CallArgs; lift up and simplify.
  SmallVector<llvm::Value*, 16> Args;

  // Handle struct-return functions by passing a pointer to the
  // location that we would like to return into.
  QualType RetTy = CallInfo.getReturnType();
  const ABIArgInfo &RetAI = CallInfo.getReturnInfo();

  // IRArgNo - Keep track of the argument number in the callee we're looking at.
  unsigned IRArgNo = 0;
  llvm::FunctionType *IRFuncTy =
    cast<llvm::FunctionType>(
                  cast<llvm::PointerType>(Callee->getType())->getElementType());

  // If the call returns a temporary with struct return, create a temporary
  // alloca to hold the result, unless one is given to us.
  if (CGM.ReturnTypeUsesSRet(CallInfo)) {
    llvm::Value *Value = ReturnValue.getValue();
    if (!Value)
      Value = CreateMemTemp(RetTy);
    Args.push_back(Value);
    checkArgMatches(Value, IRArgNo, IRFuncTy);
  }

  assert(CallInfo.arg_size() == CallArgs.size() &&
         "Mismatch between function signature & arguments.");
  CGFunctionInfo::const_arg_iterator info_it = CallInfo.arg_begin();
  for (CallArgList::const_iterator I = CallArgs.begin(), E = CallArgs.end();
       I != E; ++I, ++info_it) {
    const ABIArgInfo &ArgInfo = info_it->info;
    RValue RV = I->RV;

    CharUnits TypeAlign = getContext().getTypeAlignInChars(I->Ty);

    // Insert a padding argument to ensure proper alignment.
    if (llvm::Type *PaddingType = ArgInfo.getPaddingType()) {
      Args.push_back(llvm::UndefValue::get(PaddingType));
      ++IRArgNo;
    }

    switch (ArgInfo.getKind()) {
    case ABIArgInfo::Indirect: {
      if (RV.isScalar() || RV.isComplex()) {
        // Make a temporary alloca to pass the argument.
        llvm::AllocaInst *AI = CreateMemTemp(I->Ty);
        if (ArgInfo.getIndirectAlign() > AI->getAlignment())
          AI->setAlignment(ArgInfo.getIndirectAlign());
        Args.push_back(AI);

        LValue argLV =
          MakeAddrLValue(Args.back(), I->Ty, TypeAlign);
        
        if (RV.isScalar())
          EmitStoreOfScalar(RV.getScalarVal(), argLV, /*init*/ true);
        else
          EmitStoreOfComplex(RV.getComplexVal(), argLV, /*init*/ true);
        
        // Validate argument match.
        checkArgMatches(AI, IRArgNo, IRFuncTy);
      } else {
        // We want to avoid creating an unnecessary temporary+copy here;
        // however, we need one in three cases:
        // 1. If the argument is not byval, and we are required to copy the
        //    source.  (This case doesn't occur on any common architecture.)
        // 2. If the argument is byval, RV is not sufficiently aligned, and
        //    we cannot force it to be sufficiently aligned.
        // 3. If the argument is byval, but RV is located in an address space
        //    different than that of the argument (0).
        llvm::Value *Addr = RV.getAggregateAddr();
        unsigned Align = ArgInfo.getIndirectAlign();
        const llvm::DataLayout *TD = &CGM.getDataLayout();
        const unsigned RVAddrSpace = Addr->getType()->getPointerAddressSpace();
        const unsigned ArgAddrSpace = (IRArgNo < IRFuncTy->getNumParams() ?
          IRFuncTy->getParamType(IRArgNo)->getPointerAddressSpace() : 0);
        if ((!ArgInfo.getIndirectByVal() && I->NeedsCopy) ||
            (ArgInfo.getIndirectByVal() && TypeAlign.getQuantity() < Align &&
             llvm::getOrEnforceKnownAlignment(Addr, Align, TD) < Align) ||
             (ArgInfo.getIndirectByVal() && (RVAddrSpace != ArgAddrSpace))) {
          // Create an aligned temporary, and copy to it.
          llvm::AllocaInst *AI = CreateMemTemp(I->Ty);
          if (Align > AI->getAlignment())
            AI->setAlignment(Align);
          Args.push_back(AI);
          EmitAggregateCopy(AI, Addr, I->Ty, RV.isVolatileQualified());
              
          // Validate argument match.
          checkArgMatches(AI, IRArgNo, IRFuncTy);
        } else {
          // Skip the extra memcpy call.
          Args.push_back(Addr);
          
          // Validate argument match.
          checkArgMatches(Addr, IRArgNo, IRFuncTy);
        }
      }
      break;
    }

    case ABIArgInfo::Ignore:
      break;

    case ABIArgInfo::Extend:
    case ABIArgInfo::Direct: {
      if (!isa<llvm::StructType>(ArgInfo.getCoerceToType()) &&
          ArgInfo.getCoerceToType() == ConvertType(info_it->type) &&
          ArgInfo.getDirectOffset() == 0) {
        llvm::Value *V;
        if (RV.isScalar())
          V = RV.getScalarVal();
        else
          V = Builder.CreateLoad(RV.getAggregateAddr());
        
        // If the argument doesn't match, perform a bitcast to coerce it.  This
        // can happen due to trivial type mismatches.
        if (IRArgNo < IRFuncTy->getNumParams() &&
            V->getType() != IRFuncTy->getParamType(IRArgNo))
          V = Builder.CreateBitCast(V, IRFuncTy->getParamType(IRArgNo));
        Args.push_back(V);
        
        checkArgMatches(V, IRArgNo, IRFuncTy);
        break;
      }

      // FIXME: Avoid the conversion through memory if possible.
      llvm::Value *SrcPtr;
      if (RV.isScalar() || RV.isComplex()) {
        SrcPtr = CreateMemTemp(I->Ty, "coerce");
        LValue SrcLV = MakeAddrLValue(SrcPtr, I->Ty, TypeAlign);
        if (RV.isScalar()) {
          EmitStoreOfScalar(RV.getScalarVal(), SrcLV, /*init*/ true);
        } else {
          EmitStoreOfComplex(RV.getComplexVal(), SrcLV, /*init*/ true);
        }
      } else
        SrcPtr = RV.getAggregateAddr();

      // If the value is offset in memory, apply the offset now.
      if (unsigned Offs = ArgInfo.getDirectOffset()) {
        SrcPtr = Builder.CreateBitCast(SrcPtr, Builder.getInt8PtrTy());
        SrcPtr = Builder.CreateConstGEP1_32(SrcPtr, Offs);
        SrcPtr = Builder.CreateBitCast(SrcPtr,
                       llvm::PointerType::getUnqual(ArgInfo.getCoerceToType()));

      }

      // If the coerce-to type is a first class aggregate, we flatten it and
      // pass the elements. Either way is semantically identical, but fast-isel
      // and the optimizer generally likes scalar values better than FCAs.
      if (llvm::StructType *STy =
            dyn_cast<llvm::StructType>(ArgInfo.getCoerceToType())) {
        llvm::Type *SrcTy =
          cast<llvm::PointerType>(SrcPtr->getType())->getElementType();
        uint64_t SrcSize = CGM.getDataLayout().getTypeAllocSize(SrcTy);
        uint64_t DstSize = CGM.getDataLayout().getTypeAllocSize(STy);

        // If the source type is smaller than the destination type of the
        // coerce-to logic, copy the source value into a temp alloca the size
        // of the destination type to allow loading all of it. The bits past
        // the source value are left undef.
        if (SrcSize < DstSize) {
          llvm::AllocaInst *TempAlloca
            = CreateTempAlloca(STy, SrcPtr->getName() + ".coerce");
          Builder.CreateMemCpy(TempAlloca, SrcPtr, SrcSize, 0);
          SrcPtr = TempAlloca;
        } else {
          SrcPtr = Builder.CreateBitCast(SrcPtr,
                                         llvm::PointerType::getUnqual(STy));
        }

        for (unsigned i = 0, e = STy->getNumElements(); i != e; ++i) {
          llvm::Value *EltPtr = Builder.CreateConstGEP2_32(SrcPtr, 0, i);
          llvm::LoadInst *LI = Builder.CreateLoad(EltPtr);
          // We don't know what we're loading from.
          LI->setAlignment(1);
          Args.push_back(LI);
          
          // Validate argument match.
          checkArgMatches(LI, IRArgNo, IRFuncTy);
        }
      } else {
        // In the simple case, just pass the coerced loaded value.
        Args.push_back(CreateCoercedLoad(SrcPtr, ArgInfo.getCoerceToType(),
                                         *this));
        
        // Validate argument match.
        checkArgMatches(Args.back(), IRArgNo, IRFuncTy);
      }

      break;
    }

    case ABIArgInfo::Expand:
      ExpandTypeToArgs(I->Ty, RV, Args, IRFuncTy);
      IRArgNo = Args.size();
      break;
    }
  }

  if (!CallArgs.getCleanupsToDeactivate().empty())
    deactivateArgCleanupsBeforeCall(*this, CallArgs);

  // If the callee is a bitcast of a function to a varargs pointer to function
  // type, check to see if we can remove the bitcast.  This handles some cases
  // with unprototyped functions.
  if (llvm::ConstantExpr *CE = dyn_cast<llvm::ConstantExpr>(Callee))
    if (llvm::Function *CalleeF = dyn_cast<llvm::Function>(CE->getOperand(0))) {
      llvm::PointerType *CurPT=cast<llvm::PointerType>(Callee->getType());
      llvm::FunctionType *CurFT =
        cast<llvm::FunctionType>(CurPT->getElementType());
      llvm::FunctionType *ActualFT = CalleeF->getFunctionType();

      if (CE->getOpcode() == llvm::Instruction::BitCast &&
          ActualFT->getReturnType() == CurFT->getReturnType() &&
          ActualFT->getNumParams() == CurFT->getNumParams() &&
          ActualFT->getNumParams() == Args.size() &&
          (CurFT->isVarArg() || !ActualFT->isVarArg())) {
        bool ArgsMatch = true;
        for (unsigned i = 0, e = ActualFT->getNumParams(); i != e; ++i)
          if (ActualFT->getParamType(i) != CurFT->getParamType(i)) {
            ArgsMatch = false;
            break;
          }

        // Strip the cast if we can get away with it.  This is a nice cleanup,
        // but also allows us to inline the function at -O0 if it is marked
        // always_inline.
        if (ArgsMatch)
          Callee = CalleeF;
      }
    }

  unsigned CallingConv;
  CodeGen::AttributeListType AttributeList;
  CGM.ConstructAttributeList(CallInfo, TargetDecl, AttributeList,
                             CallingConv, true);
  llvm::AttributeSet Attrs = llvm::AttributeSet::get(getLLVMContext(),
                                                     AttributeList);

  llvm::BasicBlock *InvokeDest = 0;
  if (!Attrs.hasAttribute(llvm::AttributeSet::FunctionIndex,
                          llvm::Attribute::NoUnwind))
    InvokeDest = getInvokeDest();

  llvm::CallSite CS;
  if (!InvokeDest) {
    CS = Builder.CreateCall(Callee, Args);
  } else {
    llvm::BasicBlock *Cont = createBasicBlock("invoke.cont");
    CS = Builder.CreateInvoke(Callee, Cont, InvokeDest, Args);
    EmitBlock(Cont);
  }
  if (callOrInvoke)
    *callOrInvoke = CS.getInstruction();

  CS.setAttributes(Attrs);
  CS.setCallingConv(static_cast<llvm::CallingConv::ID>(CallingConv));

  // In ObjC ARC mode with no ObjC ARC exception safety, tell the ARC
  // optimizer it can aggressively ignore unwind edges.
  if (CGM.getLangOpts().ObjCAutoRefCount)
    AddObjCARCExceptionMetadata(CS.getInstruction());

  // If the call doesn't return, finish the basic block and clear the
  // insertion point; this allows the rest of IRgen to discard
  // unreachable code.
  if (CS.doesNotReturn()) {
    Builder.CreateUnreachable();
    Builder.ClearInsertionPoint();

    // FIXME: For now, emit a dummy basic block because expr emitters in
    // generally are not ready to handle emitting expressions at unreachable
    // points.
    EnsureInsertPoint();

    // Return a reasonable RValue.
    return GetUndefRValue(RetTy);
  }

  llvm::Instruction *CI = CS.getInstruction();
  if (Builder.isNamePreserving() && !CI->getType()->isVoidTy())
    CI->setName("call");

  // Emit any writebacks immediately.  Arguably this should happen
  // after any return-value munging.
  if (CallArgs.hasWritebacks())
    emitWritebacks(*this, CallArgs);

  switch (RetAI.getKind()) {
  case ABIArgInfo::Indirect:
    return convertTempToRValue(Args[0], RetTy, SourceLocation());

  case ABIArgInfo::Ignore:
    // If we are ignoring an argument that had a result, make sure to
    // construct the appropriate return value for our caller.
    return GetUndefRValue(RetTy);

  case ABIArgInfo::Extend:
  case ABIArgInfo::Direct: {
    llvm::Type *RetIRTy = ConvertType(RetTy);
    if (RetAI.getCoerceToType() == RetIRTy && RetAI.getDirectOffset() == 0) {
      switch (getEvaluationKind(RetTy)) {
      case TEK_Complex: {
        llvm::Value *Real = Builder.CreateExtractValue(CI, 0);
        llvm::Value *Imag = Builder.CreateExtractValue(CI, 1);
        return RValue::getComplex(std::make_pair(Real, Imag));
      }
      case TEK_Aggregate: {
        llvm::Value *DestPtr = ReturnValue.getValue();
        bool DestIsVolatile = ReturnValue.isVolatile();

        if (!DestPtr) {
          DestPtr = CreateMemTemp(RetTy, "agg.tmp");
          DestIsVolatile = false;
        }
        BuildAggStore(*this, CI, DestPtr, DestIsVolatile, false);
        return RValue::getAggregate(DestPtr);
      }
      case TEK_Scalar: {
        // If the argument doesn't match, perform a bitcast to coerce it.  This
        // can happen due to trivial type mismatches.
        llvm::Value *V = CI;
        if (V->getType() != RetIRTy)
          V = Builder.CreateBitCast(V, RetIRTy);
        return RValue::get(V);
      }
      }
      llvm_unreachable("bad evaluation kind");
    }

    llvm::Value *DestPtr = ReturnValue.getValue();
    bool DestIsVolatile = ReturnValue.isVolatile();

    if (!DestPtr) {
      DestPtr = CreateMemTemp(RetTy, "coerce");
      DestIsVolatile = false;
    }

    // If the value is offset in memory, apply the offset now.
    llvm::Value *StorePtr = DestPtr;
    if (unsigned Offs = RetAI.getDirectOffset()) {
      StorePtr = Builder.CreateBitCast(StorePtr, Builder.getInt8PtrTy());
      StorePtr = Builder.CreateConstGEP1_32(StorePtr, Offs);
      StorePtr = Builder.CreateBitCast(StorePtr,
                         llvm::PointerType::getUnqual(RetAI.getCoerceToType()));
    }
    CreateCoercedStore(CI, StorePtr, DestIsVolatile, *this);

    return convertTempToRValue(DestPtr, RetTy, SourceLocation());
  }

  case ABIArgInfo::Expand:
    llvm_unreachable("Invalid ABI kind for return argument");
  }

  llvm_unreachable("Unhandled ABIArgInfo::Kind");
}

/* VarArg handling */

llvm::Value *CodeGenFunction::EmitVAArg(llvm::Value *VAListAddr, QualType Ty) {
  return CGM.getTypes().getABIInfo().EmitVAArg(VAListAddr, Ty, *this);
}
