//===--- CGCXXRTTI.cpp - Emit LLVM Code for C++ RTTI descriptors ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This contains code dealing with C++ code generation of RTTI descriptors.
//
//===----------------------------------------------------------------------===//

#include "CodeGenModule.h"
#include "CGCXXABI.h"
#include "CGObjCRuntime.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Type.h"
#include "clang/Frontend/CodeGenOptions.h"

using namespace clang;
using namespace CodeGen;

namespace {
class ItaniumRTTIBuilder {
  CodeGenModule &CGM;  // Per-module state.
  llvm::LLVMContext &VMContext;
  
  /// Fields - The fields of the RTTI descriptor currently being built.
  SmallVector<llvm::Constant *, 16> Fields;

  /// GetAddrOfTypeName - Returns the mangled type name of the given type.
  llvm::GlobalVariable *
  GetAddrOfTypeName(QualType Ty, llvm::GlobalVariable::LinkageTypes Linkage);

  /// GetAddrOfExternalRTTIDescriptor - Returns the constant for the RTTI 
  /// descriptor of the given type.
  llvm::Constant *GetAddrOfExternalRTTIDescriptor(QualType Ty);
  
  /// BuildVTablePointer - Build the vtable pointer for the given type.
  void BuildVTablePointer(const Type *Ty);
  
  /// BuildSIClassTypeInfo - Build an abi::__si_class_type_info, used for single
  /// inheritance, according to the Itanium C++ ABI, 2.9.5p6b.
  void BuildSIClassTypeInfo(const CXXRecordDecl *RD);
  
  /// BuildVMIClassTypeInfo - Build an abi::__vmi_class_type_info, used for
  /// classes with bases that do not satisfy the abi::__si_class_type_info 
  /// constraints, according ti the Itanium C++ ABI, 2.9.5p5c.
  void BuildVMIClassTypeInfo(const CXXRecordDecl *RD);
  
  /// BuildPointerTypeInfo - Build an abi::__pointer_type_info struct, used
  /// for pointer types.
  void BuildPointerTypeInfo(QualType PointeeTy);

  /// BuildObjCObjectTypeInfo - Build the appropriate kind of
  /// type_info for an object type.
  void BuildObjCObjectTypeInfo(const ObjCObjectType *Ty);
  
  /// BuildPointerToMemberTypeInfo - Build an abi::__pointer_to_member_type_info 
  /// struct, used for member pointer types.
  void BuildPointerToMemberTypeInfo(const MemberPointerType *Ty);
  
public:
  ItaniumRTTIBuilder(CodeGenModule &CGM) : CGM(CGM), 
    VMContext(CGM.getModule().getContext()) { }

  // Pointer type info flags.
  enum {
    /// PTI_Const - Type has const qualifier.
    PTI_Const = 0x1,
    
    /// PTI_Volatile - Type has volatile qualifier.
    PTI_Volatile = 0x2,
    
    /// PTI_Restrict - Type has restrict qualifier.
    PTI_Restrict = 0x4,
    
    /// PTI_Incomplete - Type is incomplete.
    PTI_Incomplete = 0x8,
    
    /// PTI_ContainingClassIncomplete - Containing class is incomplete.
    /// (in pointer to member).
    PTI_ContainingClassIncomplete = 0x10
  };
  
  // VMI type info flags.
  enum {
    /// VMI_NonDiamondRepeat - Class has non-diamond repeated inheritance.
    VMI_NonDiamondRepeat = 0x1,
    
    /// VMI_DiamondShaped - Class is diamond shaped.
    VMI_DiamondShaped = 0x2
  };
  
  // Base class type info flags.
  enum {
    /// BCTI_Virtual - Base class is virtual.
    BCTI_Virtual = 0x1,
    
    /// BCTI_Public - Base class is public.
    BCTI_Public = 0x2
  };
  
  /// BuildTypeInfo - Build the RTTI type info struct for the given type.
  ///
  /// \param Force - true to force the creation of this RTTI value
  llvm::Constant *BuildTypeInfo(QualType Ty, bool Force = false);
};
}

llvm::GlobalVariable *
ItaniumRTTIBuilder::GetAddrOfTypeName(QualType Ty,
                               llvm::GlobalVariable::LinkageTypes Linkage) {
  SmallString<256> OutName;
  llvm::raw_svector_ostream Out(OutName);
  CGM.getCXXABI().getMangleContext().mangleCXXRTTIName(Ty, Out);
  Out.flush();
  StringRef Name = OutName.str();

  // We know that the mangled name of the type starts at index 4 of the
  // mangled name of the typename, so we can just index into it in order to
  // get the mangled name of the type.
  llvm::Constant *Init = llvm::ConstantDataArray::getString(VMContext,
                                                            Name.substr(4));

  llvm::GlobalVariable *GV = 
    CGM.CreateOrReplaceCXXRuntimeVariable(Name, Init->getType(), Linkage);

  GV->setInitializer(Init);

  return GV;
}

llvm::Constant *
ItaniumRTTIBuilder::GetAddrOfExternalRTTIDescriptor(QualType Ty) {
  // Mangle the RTTI name.
  SmallString<256> OutName;
  llvm::raw_svector_ostream Out(OutName);
  CGM.getCXXABI().getMangleContext().mangleCXXRTTI(Ty, Out);
  Out.flush();
  StringRef Name = OutName.str();

  // Look for an existing global.
  llvm::GlobalVariable *GV = CGM.getModule().getNamedGlobal(Name);
  
  if (!GV) {
    // Create a new global variable.
    GV = new llvm::GlobalVariable(CGM.getModule(), CGM.Int8PtrTy,
                                  /*Constant=*/true,
                                  llvm::GlobalValue::ExternalLinkage, nullptr,
                                  Name);
  }
  
  return llvm::ConstantExpr::getBitCast(GV, CGM.Int8PtrTy);
}

/// TypeInfoIsInStandardLibrary - Given a builtin type, returns whether the type
/// info for that type is defined in the standard library.
static bool TypeInfoIsInStandardLibrary(const BuiltinType *Ty) {
  // Itanium C++ ABI 2.9.2:
  //   Basic type information (e.g. for "int", "bool", etc.) will be kept in
  //   the run-time support library. Specifically, the run-time support
  //   library should contain type_info objects for the types X, X* and 
  //   X const*, for every X in: void, std::nullptr_t, bool, wchar_t, char,
  //   unsigned char, signed char, short, unsigned short, int, unsigned int,
  //   long, unsigned long, long long, unsigned long long, float, double,
  //   long double, char16_t, char32_t, and the IEEE 754r decimal and 
  //   half-precision floating point types.
  switch (Ty->getKind()) {
    case BuiltinType::Void:
    case BuiltinType::NullPtr:
    case BuiltinType::Bool:
    case BuiltinType::WChar_S:
    case BuiltinType::WChar_U:
    case BuiltinType::Char_U:
    case BuiltinType::Char_S:
    case BuiltinType::UChar:
    case BuiltinType::SChar:
    case BuiltinType::Short:
    case BuiltinType::UShort:
    case BuiltinType::Int:
    case BuiltinType::UInt:
    case BuiltinType::Long:
    case BuiltinType::ULong:
    case BuiltinType::LongLong:
    case BuiltinType::ULongLong:
    case BuiltinType::Half:
    case BuiltinType::Float:
    case BuiltinType::Double:
    case BuiltinType::LongDouble:
    case BuiltinType::Char16:
    case BuiltinType::Char32:
    case BuiltinType::Int128:
    case BuiltinType::UInt128:
    case BuiltinType::OCLImage1d:
    case BuiltinType::OCLImage1dArray:
    case BuiltinType::OCLImage1dBuffer:
    case BuiltinType::OCLImage2d:
    case BuiltinType::OCLImage2dArray:
    case BuiltinType::OCLImage3d:
    case BuiltinType::OCLSampler:
    case BuiltinType::OCLEvent:
      return true;
      
    case BuiltinType::Dependent:
#define BUILTIN_TYPE(Id, SingletonId)
#define PLACEHOLDER_TYPE(Id, SingletonId) \
    case BuiltinType::Id:
#include "clang/AST/BuiltinTypes.def"
      llvm_unreachable("asking for RRTI for a placeholder type!");
      
    case BuiltinType::ObjCId:
    case BuiltinType::ObjCClass:
    case BuiltinType::ObjCSel:
      llvm_unreachable("FIXME: Objective-C types are unsupported!");
  }

  llvm_unreachable("Invalid BuiltinType Kind!");
}

static bool TypeInfoIsInStandardLibrary(const PointerType *PointerTy) {
  QualType PointeeTy = PointerTy->getPointeeType();
  const BuiltinType *BuiltinTy = dyn_cast<BuiltinType>(PointeeTy);
  if (!BuiltinTy)
    return false;
    
  // Check the qualifiers.
  Qualifiers Quals = PointeeTy.getQualifiers();
  Quals.removeConst();
    
  if (!Quals.empty())
    return false;
    
  return TypeInfoIsInStandardLibrary(BuiltinTy);
}

/// IsStandardLibraryRTTIDescriptor - Returns whether the type
/// information for the given type exists in the standard library.
static bool IsStandardLibraryRTTIDescriptor(QualType Ty) {
  // Type info for builtin types is defined in the standard library.
  if (const BuiltinType *BuiltinTy = dyn_cast<BuiltinType>(Ty))
    return TypeInfoIsInStandardLibrary(BuiltinTy);
  
  // Type info for some pointer types to builtin types is defined in the
  // standard library.
  if (const PointerType *PointerTy = dyn_cast<PointerType>(Ty))
    return TypeInfoIsInStandardLibrary(PointerTy);

  return false;
}

/// ShouldUseExternalRTTIDescriptor - Returns whether the type information for
/// the given type exists somewhere else, and that we should not emit the type
/// information in this translation unit.  Assumes that it is not a
/// standard-library type.
static bool ShouldUseExternalRTTIDescriptor(CodeGenModule &CGM,
                                            QualType Ty) {
  ASTContext &Context = CGM.getContext();

  // If RTTI is disabled, assume it might be disabled in the
  // translation unit that defines any potential key function, too.
  if (!Context.getLangOpts().RTTI) return false;

  if (const RecordType *RecordTy = dyn_cast<RecordType>(Ty)) {
    const CXXRecordDecl *RD = cast<CXXRecordDecl>(RecordTy->getDecl());
    if (!RD->hasDefinition())
      return false;

    if (!RD->isDynamicClass())
      return false;

    // FIXME: this may need to be reconsidered if the key function
    // changes.
    return CGM.getVTables().isVTableExternal(RD);
  }
  
  return false;
}

/// IsIncompleteClassType - Returns whether the given record type is incomplete.
static bool IsIncompleteClassType(const RecordType *RecordTy) {
  return !RecordTy->getDecl()->isCompleteDefinition();
}  

/// ContainsIncompleteClassType - Returns whether the given type contains an
/// incomplete class type. This is true if
///
///   * The given type is an incomplete class type.
///   * The given type is a pointer type whose pointee type contains an 
///     incomplete class type.
///   * The given type is a member pointer type whose class is an incomplete
///     class type.
///   * The given type is a member pointer type whoise pointee type contains an
///     incomplete class type.
/// is an indirect or direct pointer to an incomplete class type.
static bool ContainsIncompleteClassType(QualType Ty) {
  if (const RecordType *RecordTy = dyn_cast<RecordType>(Ty)) {
    if (IsIncompleteClassType(RecordTy))
      return true;
  }
  
  if (const PointerType *PointerTy = dyn_cast<PointerType>(Ty))
    return ContainsIncompleteClassType(PointerTy->getPointeeType());
  
  if (const MemberPointerType *MemberPointerTy = 
      dyn_cast<MemberPointerType>(Ty)) {
    // Check if the class type is incomplete.
    const RecordType *ClassType = cast<RecordType>(MemberPointerTy->getClass());
    if (IsIncompleteClassType(ClassType))
      return true;
    
    return ContainsIncompleteClassType(MemberPointerTy->getPointeeType());
  }
  
  return false;
}

/// getTypeInfoLinkage - Return the linkage that the type info and type info
/// name constants should have for the given type.
llvm::GlobalVariable::LinkageTypes 
CodeGenModule::getTypeInfoLinkage(QualType Ty) {
  // Itanium C++ ABI 2.9.5p7:
  //   In addition, it and all of the intermediate abi::__pointer_type_info 
  //   structs in the chain down to the abi::__class_type_info for the
  //   incomplete class type must be prevented from resolving to the 
  //   corresponding type_info structs for the complete class type, possibly
  //   by making them local static objects. Finally, a dummy class RTTI is
  //   generated for the incomplete type that will not resolve to the final 
  //   complete class RTTI (because the latter need not exist), possibly by 
  //   making it a local static object.
  if (ContainsIncompleteClassType(Ty))
    return llvm::GlobalValue::InternalLinkage;
  
  switch (Ty->getLinkage()) {
  case NoLinkage:
  case InternalLinkage:
  case UniqueExternalLinkage:
    return llvm::GlobalValue::InternalLinkage;

  case VisibleNoLinkage:
  case ExternalLinkage:
    if (!getLangOpts().RTTI) {
      // RTTI is not enabled, which means that this type info struct is going
      // to be used for exception handling. Give it linkonce_odr linkage.
      return llvm::GlobalValue::LinkOnceODRLinkage;
    }

    if (const RecordType *Record = dyn_cast<RecordType>(Ty)) {
      const CXXRecordDecl *RD = cast<CXXRecordDecl>(Record->getDecl());
      if (RD->hasAttr<WeakAttr>())
        return llvm::GlobalValue::WeakODRLinkage;
      if (RD->isDynamicClass())
        return getVTableLinkage(RD);
    }

    return llvm::GlobalValue::LinkOnceODRLinkage;
  }

  llvm_unreachable("Invalid linkage!");
}

// CanUseSingleInheritance - Return whether the given record decl has a "single, 
// public, non-virtual base at offset zero (i.e. the derived class is dynamic 
// iff the base is)", according to Itanium C++ ABI, 2.95p6b.
static bool CanUseSingleInheritance(const CXXRecordDecl *RD) {
  // Check the number of bases.
  if (RD->getNumBases() != 1)
    return false;
  
  // Get the base.
  CXXRecordDecl::base_class_const_iterator Base = RD->bases_begin();
  
  // Check that the base is not virtual.
  if (Base->isVirtual())
    return false;
  
  // Check that the base is public.
  if (Base->getAccessSpecifier() != AS_public)
    return false;
  
  // Check that the class is dynamic iff the base is.
  const CXXRecordDecl *BaseDecl = 
    cast<CXXRecordDecl>(Base->getType()->getAs<RecordType>()->getDecl());
  if (!BaseDecl->isEmpty() && 
      BaseDecl->isDynamicClass() != RD->isDynamicClass())
    return false;
  
  return true;
}

void ItaniumRTTIBuilder::BuildVTablePointer(const Type *Ty) {
  // abi::__class_type_info.
  static const char * const ClassTypeInfo =
    "_ZTVN10__cxxabiv117__class_type_infoE";
  // abi::__si_class_type_info.
  static const char * const SIClassTypeInfo =
    "_ZTVN10__cxxabiv120__si_class_type_infoE";
  // abi::__vmi_class_type_info.
  static const char * const VMIClassTypeInfo =
    "_ZTVN10__cxxabiv121__vmi_class_type_infoE";

  const char *VTableName = nullptr;

  switch (Ty->getTypeClass()) {
#define TYPE(Class, Base)
#define ABSTRACT_TYPE(Class, Base)
#define NON_CANONICAL_UNLESS_DEPENDENT_TYPE(Class, Base) case Type::Class:
#define NON_CANONICAL_TYPE(Class, Base) case Type::Class:
#define DEPENDENT_TYPE(Class, Base) case Type::Class:
#include "clang/AST/TypeNodes.def"
    llvm_unreachable("Non-canonical and dependent types shouldn't get here");

  case Type::LValueReference:
  case Type::RValueReference:
    llvm_unreachable("References shouldn't get here");

  case Type::Auto:
    llvm_unreachable("Undeduced auto type shouldn't get here");

  case Type::Builtin:
  // GCC treats vector and complex types as fundamental types.
  case Type::Vector:
  case Type::ExtVector:
  case Type::Complex:
  case Type::Atomic:
  // FIXME: GCC treats block pointers as fundamental types?!
  case Type::BlockPointer:
    // abi::__fundamental_type_info.
    VTableName = "_ZTVN10__cxxabiv123__fundamental_type_infoE";
    break;

  case Type::ConstantArray:
  case Type::IncompleteArray:
  case Type::VariableArray:
    // abi::__array_type_info.
    VTableName = "_ZTVN10__cxxabiv117__array_type_infoE";
    break;

  case Type::FunctionNoProto:
  case Type::FunctionProto:
    // abi::__function_type_info.
    VTableName = "_ZTVN10__cxxabiv120__function_type_infoE";
    break;

  case Type::Enum:
    // abi::__enum_type_info.
    VTableName = "_ZTVN10__cxxabiv116__enum_type_infoE";
    break;

  case Type::Record: {
    const CXXRecordDecl *RD = 
      cast<CXXRecordDecl>(cast<RecordType>(Ty)->getDecl());
    
    if (!RD->hasDefinition() || !RD->getNumBases()) {
      VTableName = ClassTypeInfo;
    } else if (CanUseSingleInheritance(RD)) {
      VTableName = SIClassTypeInfo;
    } else {
      VTableName = VMIClassTypeInfo;
    }
    
    break;
  }

  case Type::ObjCObject:
    // Ignore protocol qualifiers.
    Ty = cast<ObjCObjectType>(Ty)->getBaseType().getTypePtr();

    // Handle id and Class.
    if (isa<BuiltinType>(Ty)) {
      VTableName = ClassTypeInfo;
      break;
    }

    assert(isa<ObjCInterfaceType>(Ty));
    // Fall through.

  case Type::ObjCInterface:
    if (cast<ObjCInterfaceType>(Ty)->getDecl()->getSuperClass()) {
      VTableName = SIClassTypeInfo;
    } else {
      VTableName = ClassTypeInfo;
    }
    break;

  case Type::ObjCObjectPointer:
  case Type::Pointer:
    // abi::__pointer_type_info.
    VTableName = "_ZTVN10__cxxabiv119__pointer_type_infoE";
    break;

  case Type::MemberPointer:
    // abi::__pointer_to_member_type_info.
    VTableName = "_ZTVN10__cxxabiv129__pointer_to_member_type_infoE";
    break;
  }

  llvm::Constant *VTable = 
    CGM.getModule().getOrInsertGlobal(VTableName, CGM.Int8PtrTy);
    
  llvm::Type *PtrDiffTy = 
    CGM.getTypes().ConvertType(CGM.getContext().getPointerDiffType());

  // The vtable address point is 2.
  llvm::Constant *Two = llvm::ConstantInt::get(PtrDiffTy, 2);
  VTable = llvm::ConstantExpr::getInBoundsGetElementPtr(VTable, Two);
  VTable = llvm::ConstantExpr::getBitCast(VTable, CGM.Int8PtrTy);

  Fields.push_back(VTable);
}

llvm::Constant *ItaniumRTTIBuilder::BuildTypeInfo(QualType Ty, bool Force) {
  // We want to operate on the canonical type.
  Ty = CGM.getContext().getCanonicalType(Ty);

  // Check if we've already emitted an RTTI descriptor for this type.
  SmallString<256> OutName;
  llvm::raw_svector_ostream Out(OutName);
  CGM.getCXXABI().getMangleContext().mangleCXXRTTI(Ty, Out);
  Out.flush();
  StringRef Name = OutName.str();

  llvm::GlobalVariable *OldGV = CGM.getModule().getNamedGlobal(Name);
  if (OldGV && !OldGV->isDeclaration()) {
    assert(!OldGV->hasAvailableExternallyLinkage() &&
           "available_externally typeinfos not yet implemented");

    return llvm::ConstantExpr::getBitCast(OldGV, CGM.Int8PtrTy);
  }

  // Check if there is already an external RTTI descriptor for this type.
  bool IsStdLib = IsStandardLibraryRTTIDescriptor(Ty);
  if (!Force && (IsStdLib || ShouldUseExternalRTTIDescriptor(CGM, Ty)))
    return GetAddrOfExternalRTTIDescriptor(Ty);

  // Emit the standard library with external linkage.
  llvm::GlobalVariable::LinkageTypes Linkage;
  if (IsStdLib)
    Linkage = llvm::GlobalValue::ExternalLinkage;
  else
    Linkage = CGM.getTypeInfoLinkage(Ty);

  // Add the vtable pointer.
  BuildVTablePointer(cast<Type>(Ty));
  
  // And the name.
  llvm::GlobalVariable *TypeName = GetAddrOfTypeName(Ty, Linkage);
  llvm::Constant *TypeNameField;

  // If we're supposed to demote the visibility, be sure to set a flag
  // to use a string comparison for type_info comparisons.
  CGCXXABI::RTTIUniquenessKind RTTIUniqueness =
      CGM.getCXXABI().classifyRTTIUniqueness(Ty, Linkage);
  if (RTTIUniqueness != CGCXXABI::RUK_Unique) {
    // The flag is the sign bit, which on ARM64 is defined to be clear
    // for global pointers.  This is very ARM64-specific.
    TypeNameField = llvm::ConstantExpr::getPtrToInt(TypeName, CGM.Int64Ty);
    llvm::Constant *flag =
        llvm::ConstantInt::get(CGM.Int64Ty, ((uint64_t)1) << 63);
    TypeNameField = llvm::ConstantExpr::getAdd(TypeNameField, flag);
    TypeNameField =
        llvm::ConstantExpr::getIntToPtr(TypeNameField, CGM.Int8PtrTy);
  } else {
    TypeNameField = llvm::ConstantExpr::getBitCast(TypeName, CGM.Int8PtrTy);
  }
  Fields.push_back(TypeNameField);

  switch (Ty->getTypeClass()) {
#define TYPE(Class, Base)
#define ABSTRACT_TYPE(Class, Base)
#define NON_CANONICAL_UNLESS_DEPENDENT_TYPE(Class, Base) case Type::Class:
#define NON_CANONICAL_TYPE(Class, Base) case Type::Class:
#define DEPENDENT_TYPE(Class, Base) case Type::Class:
#include "clang/AST/TypeNodes.def"
    llvm_unreachable("Non-canonical and dependent types shouldn't get here");

  // GCC treats vector types as fundamental types.
  case Type::Builtin:
  case Type::Vector:
  case Type::ExtVector:
  case Type::Complex:
  case Type::BlockPointer:
    // Itanium C++ ABI 2.9.5p4:
    // abi::__fundamental_type_info adds no data members to std::type_info.
    break;

  case Type::LValueReference:
  case Type::RValueReference:
    llvm_unreachable("References shouldn't get here");

  case Type::Auto:
    llvm_unreachable("Undeduced auto type shouldn't get here");

  case Type::ConstantArray:
  case Type::IncompleteArray:
  case Type::VariableArray:
    // Itanium C++ ABI 2.9.5p5:
    // abi::__array_type_info adds no data members to std::type_info.
    break;

  case Type::FunctionNoProto:
  case Type::FunctionProto:
    // Itanium C++ ABI 2.9.5p5:
    // abi::__function_type_info adds no data members to std::type_info.
    break;

  case Type::Enum:
    // Itanium C++ ABI 2.9.5p5:
    // abi::__enum_type_info adds no data members to std::type_info.
    break;

  case Type::Record: {
    const CXXRecordDecl *RD = 
      cast<CXXRecordDecl>(cast<RecordType>(Ty)->getDecl());
    if (!RD->hasDefinition() || !RD->getNumBases()) {
      // We don't need to emit any fields.
      break;
    }
    
    if (CanUseSingleInheritance(RD))
      BuildSIClassTypeInfo(RD);
    else 
      BuildVMIClassTypeInfo(RD);

    break;
  }

  case Type::ObjCObject:
  case Type::ObjCInterface:
    BuildObjCObjectTypeInfo(cast<ObjCObjectType>(Ty));
    break;

  case Type::ObjCObjectPointer:
    BuildPointerTypeInfo(cast<ObjCObjectPointerType>(Ty)->getPointeeType());
    break; 
      
  case Type::Pointer:
    BuildPointerTypeInfo(cast<PointerType>(Ty)->getPointeeType());
    break;

  case Type::MemberPointer:
    BuildPointerToMemberTypeInfo(cast<MemberPointerType>(Ty));
    break;

  case Type::Atomic:
    // No fields, at least for the moment.
    break;
  }

  llvm::Constant *Init = llvm::ConstantStruct::getAnon(Fields);

  llvm::GlobalVariable *GV = 
    new llvm::GlobalVariable(CGM.getModule(), Init->getType(), 
                             /*Constant=*/true, Linkage, Init, Name);
  
  // If there's already an old global variable, replace it with the new one.
  if (OldGV) {
    GV->takeName(OldGV);
    llvm::Constant *NewPtr = 
      llvm::ConstantExpr::getBitCast(GV, OldGV->getType());
    OldGV->replaceAllUsesWith(NewPtr);
    OldGV->eraseFromParent();
  }

  // The Itanium ABI specifies that type_info objects must be globally
  // unique, with one exception: if the type is an incomplete class
  // type or a (possibly indirect) pointer to one.  That exception
  // affects the general case of comparing type_info objects produced
  // by the typeid operator, which is why the comparison operators on
  // std::type_info generally use the type_info name pointers instead
  // of the object addresses.  However, the language's built-in uses
  // of RTTI generally require class types to be complete, even when
  // manipulating pointers to those class types.  This allows the
  // implementation of dynamic_cast to rely on address equality tests,
  // which is much faster.

  // All of this is to say that it's important that both the type_info
  // object and the type_info name be uniqued when weakly emitted.

  // Give the type_info object and name the formal visibility of the
  // type itself.
  llvm::GlobalValue::VisibilityTypes llvmVisibility;
  if (llvm::GlobalValue::isLocalLinkage(Linkage))
    // If the linkage is local, only default visibility makes sense.
    llvmVisibility = llvm::GlobalValue::DefaultVisibility;
  else if (RTTIUniqueness == CGCXXABI::RUK_NonUniqueHidden)
    llvmVisibility = llvm::GlobalValue::HiddenVisibility;
  else
    llvmVisibility = CodeGenModule::GetLLVMVisibility(Ty->getVisibility());
  TypeName->setVisibility(llvmVisibility);
  GV->setVisibility(llvmVisibility);

  return llvm::ConstantExpr::getBitCast(GV, CGM.Int8PtrTy);
}

/// ComputeQualifierFlags - Compute the pointer type info flags from the
/// given qualifier.
static unsigned ComputeQualifierFlags(Qualifiers Quals) {
  unsigned Flags = 0;

  if (Quals.hasConst())
    Flags |= ItaniumRTTIBuilder::PTI_Const;
  if (Quals.hasVolatile())
    Flags |= ItaniumRTTIBuilder::PTI_Volatile;
  if (Quals.hasRestrict())
    Flags |= ItaniumRTTIBuilder::PTI_Restrict;

  return Flags;
}

/// BuildObjCObjectTypeInfo - Build the appropriate kind of type_info
/// for the given Objective-C object type.
void ItaniumRTTIBuilder::BuildObjCObjectTypeInfo(const ObjCObjectType *OT) {
  // Drop qualifiers.
  const Type *T = OT->getBaseType().getTypePtr();
  assert(isa<BuiltinType>(T) || isa<ObjCInterfaceType>(T));

  // The builtin types are abi::__class_type_infos and don't require
  // extra fields.
  if (isa<BuiltinType>(T)) return;

  ObjCInterfaceDecl *Class = cast<ObjCInterfaceType>(T)->getDecl();
  ObjCInterfaceDecl *Super = Class->getSuperClass();

  // Root classes are also __class_type_info.
  if (!Super) return;

  QualType SuperTy = CGM.getContext().getObjCInterfaceType(Super);

  // Everything else is single inheritance.
  llvm::Constant *BaseTypeInfo = ItaniumRTTIBuilder(CGM).BuildTypeInfo(SuperTy);
  Fields.push_back(BaseTypeInfo);
}

/// BuildSIClassTypeInfo - Build an abi::__si_class_type_info, used for single
/// inheritance, according to the Itanium C++ ABI, 2.95p6b.
void ItaniumRTTIBuilder::BuildSIClassTypeInfo(const CXXRecordDecl *RD) {
  // Itanium C++ ABI 2.9.5p6b:
  // It adds to abi::__class_type_info a single member pointing to the 
  // type_info structure for the base type,
  llvm::Constant *BaseTypeInfo = 
    ItaniumRTTIBuilder(CGM).BuildTypeInfo(RD->bases_begin()->getType());
  Fields.push_back(BaseTypeInfo);
}

namespace {
  /// SeenBases - Contains virtual and non-virtual bases seen when traversing
  /// a class hierarchy.
  struct SeenBases {
    llvm::SmallPtrSet<const CXXRecordDecl *, 16> NonVirtualBases;
    llvm::SmallPtrSet<const CXXRecordDecl *, 16> VirtualBases;
  };
}

/// ComputeVMIClassTypeInfoFlags - Compute the value of the flags member in
/// abi::__vmi_class_type_info.
///
static unsigned ComputeVMIClassTypeInfoFlags(const CXXBaseSpecifier *Base, 
                                             SeenBases &Bases) {
  
  unsigned Flags = 0;
  
  const CXXRecordDecl *BaseDecl = 
    cast<CXXRecordDecl>(Base->getType()->getAs<RecordType>()->getDecl());
  
  if (Base->isVirtual()) {
    // Mark the virtual base as seen.
    if (!Bases.VirtualBases.insert(BaseDecl)) {
      // If this virtual base has been seen before, then the class is diamond
      // shaped.
      Flags |= ItaniumRTTIBuilder::VMI_DiamondShaped;
    } else {
      if (Bases.NonVirtualBases.count(BaseDecl))
        Flags |= ItaniumRTTIBuilder::VMI_NonDiamondRepeat;
    }
  } else {
    // Mark the non-virtual base as seen.
    if (!Bases.NonVirtualBases.insert(BaseDecl)) {
      // If this non-virtual base has been seen before, then the class has non-
      // diamond shaped repeated inheritance.
      Flags |= ItaniumRTTIBuilder::VMI_NonDiamondRepeat;
    } else {
      if (Bases.VirtualBases.count(BaseDecl))
        Flags |= ItaniumRTTIBuilder::VMI_NonDiamondRepeat;
    }
  }

  // Walk all bases.
  for (const auto &I : BaseDecl->bases()) 
    Flags |= ComputeVMIClassTypeInfoFlags(&I, Bases);
  
  return Flags;
}

static unsigned ComputeVMIClassTypeInfoFlags(const CXXRecordDecl *RD) {
  unsigned Flags = 0;
  SeenBases Bases;
  
  // Walk all bases.
  for (const auto &I : RD->bases()) 
    Flags |= ComputeVMIClassTypeInfoFlags(&I, Bases);
  
  return Flags;
}

/// BuildVMIClassTypeInfo - Build an abi::__vmi_class_type_info, used for
/// classes with bases that do not satisfy the abi::__si_class_type_info 
/// constraints, according ti the Itanium C++ ABI, 2.9.5p5c.
void ItaniumRTTIBuilder::BuildVMIClassTypeInfo(const CXXRecordDecl *RD) {
  llvm::Type *UnsignedIntLTy = 
    CGM.getTypes().ConvertType(CGM.getContext().UnsignedIntTy);
  
  // Itanium C++ ABI 2.9.5p6c:
  //   __flags is a word with flags describing details about the class 
  //   structure, which may be referenced by using the __flags_masks 
  //   enumeration. These flags refer to both direct and indirect bases. 
  unsigned Flags = ComputeVMIClassTypeInfoFlags(RD);
  Fields.push_back(llvm::ConstantInt::get(UnsignedIntLTy, Flags));

  // Itanium C++ ABI 2.9.5p6c:
  //   __base_count is a word with the number of direct proper base class 
  //   descriptions that follow.
  Fields.push_back(llvm::ConstantInt::get(UnsignedIntLTy, RD->getNumBases()));
  
  if (!RD->getNumBases())
    return;
  
  llvm::Type *LongLTy = 
    CGM.getTypes().ConvertType(CGM.getContext().LongTy);

  // Now add the base class descriptions.
  
  // Itanium C++ ABI 2.9.5p6c:
  //   __base_info[] is an array of base class descriptions -- one for every 
  //   direct proper base. Each description is of the type:
  //
  //   struct abi::__base_class_type_info {
  //   public:
  //     const __class_type_info *__base_type;
  //     long __offset_flags;
  //
  //     enum __offset_flags_masks {
  //       __virtual_mask = 0x1,
  //       __public_mask = 0x2,
  //       __offset_shift = 8
  //     };
  //   };
  for (const auto &Base : RD->bases()) {
    // The __base_type member points to the RTTI for the base type.
    Fields.push_back(ItaniumRTTIBuilder(CGM).BuildTypeInfo(Base.getType()));

    const CXXRecordDecl *BaseDecl = 
      cast<CXXRecordDecl>(Base.getType()->getAs<RecordType>()->getDecl());

    int64_t OffsetFlags = 0;
    
    // All but the lower 8 bits of __offset_flags are a signed offset. 
    // For a non-virtual base, this is the offset in the object of the base
    // subobject. For a virtual base, this is the offset in the virtual table of
    // the virtual base offset for the virtual base referenced (negative).
    CharUnits Offset;
    if (Base.isVirtual())
      Offset = 
        CGM.getItaniumVTableContext().getVirtualBaseOffsetOffset(RD, BaseDecl);
    else {
      const ASTRecordLayout &Layout = CGM.getContext().getASTRecordLayout(RD);
      Offset = Layout.getBaseClassOffset(BaseDecl);
    };
    
    OffsetFlags = uint64_t(Offset.getQuantity()) << 8;
    
    // The low-order byte of __offset_flags contains flags, as given by the 
    // masks from the enumeration __offset_flags_masks.
    if (Base.isVirtual())
      OffsetFlags |= BCTI_Virtual;
    if (Base.getAccessSpecifier() == AS_public)
      OffsetFlags |= BCTI_Public;

    Fields.push_back(llvm::ConstantInt::get(LongLTy, OffsetFlags));
  }
}

/// BuildPointerTypeInfo - Build an abi::__pointer_type_info struct,
/// used for pointer types.
void ItaniumRTTIBuilder::BuildPointerTypeInfo(QualType PointeeTy) {
  Qualifiers Quals;
  QualType UnqualifiedPointeeTy = 
    CGM.getContext().getUnqualifiedArrayType(PointeeTy, Quals);
  
  // Itanium C++ ABI 2.9.5p7:
  //   __flags is a flag word describing the cv-qualification and other 
  //   attributes of the type pointed to
  unsigned Flags = ComputeQualifierFlags(Quals);

  // Itanium C++ ABI 2.9.5p7:
  //   When the abi::__pbase_type_info is for a direct or indirect pointer to an
  //   incomplete class type, the incomplete target type flag is set. 
  if (ContainsIncompleteClassType(UnqualifiedPointeeTy))
    Flags |= PTI_Incomplete;

  llvm::Type *UnsignedIntLTy = 
    CGM.getTypes().ConvertType(CGM.getContext().UnsignedIntTy);
  Fields.push_back(llvm::ConstantInt::get(UnsignedIntLTy, Flags));
  
  // Itanium C++ ABI 2.9.5p7:
  //  __pointee is a pointer to the std::type_info derivation for the 
  //  unqualified type being pointed to.
  llvm::Constant *PointeeTypeInfo = 
    ItaniumRTTIBuilder(CGM).BuildTypeInfo(UnqualifiedPointeeTy);
  Fields.push_back(PointeeTypeInfo);
}

/// BuildPointerToMemberTypeInfo - Build an abi::__pointer_to_member_type_info 
/// struct, used for member pointer types.
void
ItaniumRTTIBuilder::BuildPointerToMemberTypeInfo(const MemberPointerType *Ty) {
  QualType PointeeTy = Ty->getPointeeType();
  
  Qualifiers Quals;
  QualType UnqualifiedPointeeTy = 
    CGM.getContext().getUnqualifiedArrayType(PointeeTy, Quals);
  
  // Itanium C++ ABI 2.9.5p7:
  //   __flags is a flag word describing the cv-qualification and other 
  //   attributes of the type pointed to.
  unsigned Flags = ComputeQualifierFlags(Quals);

  const RecordType *ClassType = cast<RecordType>(Ty->getClass());

  // Itanium C++ ABI 2.9.5p7:
  //   When the abi::__pbase_type_info is for a direct or indirect pointer to an
  //   incomplete class type, the incomplete target type flag is set. 
  if (ContainsIncompleteClassType(UnqualifiedPointeeTy))
    Flags |= PTI_Incomplete;

  if (IsIncompleteClassType(ClassType))
    Flags |= PTI_ContainingClassIncomplete;
  
  llvm::Type *UnsignedIntLTy = 
    CGM.getTypes().ConvertType(CGM.getContext().UnsignedIntTy);
  Fields.push_back(llvm::ConstantInt::get(UnsignedIntLTy, Flags));
  
  // Itanium C++ ABI 2.9.5p7:
  //   __pointee is a pointer to the std::type_info derivation for the 
  //   unqualified type being pointed to.
  llvm::Constant *PointeeTypeInfo = 
    ItaniumRTTIBuilder(CGM).BuildTypeInfo(UnqualifiedPointeeTy);
  Fields.push_back(PointeeTypeInfo);

  // Itanium C++ ABI 2.9.5p9:
  //   __context is a pointer to an abi::__class_type_info corresponding to the
  //   class type containing the member pointed to 
  //   (e.g., the "A" in "int A::*").
  Fields.push_back(
      ItaniumRTTIBuilder(CGM).BuildTypeInfo(QualType(ClassType, 0)));
}

llvm::Constant *CodeGenModule::GetAddrOfRTTIDescriptor(QualType Ty,
                                                       bool ForEH) {
  // Return a bogus pointer if RTTI is disabled, unless it's for EH.
  // FIXME: should we even be calling this method if RTTI is disabled
  // and it's not for EH?
  if (!ForEH && !getLangOpts().RTTI)
    return llvm::Constant::getNullValue(Int8PtrTy);
  
  if (ForEH && Ty->isObjCObjectPointerType() &&
      LangOpts.ObjCRuntime.isGNUFamily())
    return ObjCRuntime->GetEHType(Ty);

  if (getTarget().getCXXABI().isMicrosoft())
    return getMSTypeDescriptor(Ty);
  return ItaniumRTTIBuilder(*this).BuildTypeInfo(Ty);
}

void CodeGenModule::EmitFundamentalRTTIDescriptor(QualType Type) {
  QualType PointerType = Context.getPointerType(Type);
  QualType PointerTypeConst = Context.getPointerType(Type.withConst());
  ItaniumRTTIBuilder(*this).BuildTypeInfo(Type, true);
  ItaniumRTTIBuilder(*this).BuildTypeInfo(PointerType, true);
  ItaniumRTTIBuilder(*this).BuildTypeInfo(PointerTypeConst, true);
}

void CodeGenModule::EmitFundamentalRTTIDescriptors() {
  QualType FundamentalTypes[] = { Context.VoidTy, Context.NullPtrTy,
                                  Context.BoolTy, Context.WCharTy,
                                  Context.CharTy, Context.UnsignedCharTy,
                                  Context.SignedCharTy, Context.ShortTy, 
                                  Context.UnsignedShortTy, Context.IntTy,
                                  Context.UnsignedIntTy, Context.LongTy, 
                                  Context.UnsignedLongTy, Context.LongLongTy, 
                                  Context.UnsignedLongLongTy,
                                  Context.HalfTy, Context.FloatTy,
                                  Context.DoubleTy, Context.LongDoubleTy,
                                  Context.Char16Ty, Context.Char32Ty };
  for (unsigned i = 0; i < llvm::array_lengthof(FundamentalTypes); ++i)
    EmitFundamentalRTTIDescriptor(FundamentalTypes[i]);
}
