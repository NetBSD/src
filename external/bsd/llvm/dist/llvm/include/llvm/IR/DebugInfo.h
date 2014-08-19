//===- DebugInfo.h - Debug Information Helpers ------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a bunch of datatypes that are useful for creating and
// walking debug info in LLVM IR form. They essentially provide wrappers around
// the information in the global variables that's needed when constructing the
// DWARF information.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_DEBUGINFO_H
#define LLVM_IR_DEBUGINFO_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/iterator_range.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Dwarf.h"

namespace llvm {
class BasicBlock;
class Constant;
class Function;
class GlobalVariable;
class Module;
class Type;
class Value;
class DbgDeclareInst;
class DbgValueInst;
class Instruction;
class MDNode;
class MDString;
class NamedMDNode;
class LLVMContext;
class raw_ostream;

class DIFile;
class DISubprogram;
class DILexicalBlock;
class DILexicalBlockFile;
class DIVariable;
class DIType;
class DIScope;
class DIObjCProperty;

/// Maps from type identifier to the actual MDNode.
typedef DenseMap<const MDString *, MDNode *> DITypeIdentifierMap;

/// DIDescriptor - A thin wraper around MDNode to access encoded debug info.
/// This should not be stored in a container, because the underlying MDNode
/// may change in certain situations.
class DIDescriptor {
  // Befriends DIRef so DIRef can befriend the protected member
  // function: getFieldAs<DIRef>.
  template <typename T> friend class DIRef;

public:
  enum {
    FlagPrivate           = 1 << 0,
    FlagProtected         = 1 << 1,
    FlagFwdDecl           = 1 << 2,
    FlagAppleBlock        = 1 << 3,
    FlagBlockByrefStruct  = 1 << 4,
    FlagVirtual           = 1 << 5,
    FlagArtificial        = 1 << 6,
    FlagExplicit          = 1 << 7,
    FlagPrototyped        = 1 << 8,
    FlagObjcClassComplete = 1 << 9,
    FlagObjectPointer     = 1 << 10,
    FlagVector            = 1 << 11,
    FlagStaticMember      = 1 << 12,
    FlagIndirectVariable  = 1 << 13,
    FlagLValueReference   = 1 << 14,
    FlagRValueReference   = 1 << 15
  };

protected:
  const MDNode *DbgNode;

  StringRef getStringField(unsigned Elt) const;
  unsigned getUnsignedField(unsigned Elt) const {
    return (unsigned)getUInt64Field(Elt);
  }
  uint64_t getUInt64Field(unsigned Elt) const;
  int64_t getInt64Field(unsigned Elt) const;
  DIDescriptor getDescriptorField(unsigned Elt) const;

  template <typename DescTy> DescTy getFieldAs(unsigned Elt) const {
    return DescTy(getDescriptorField(Elt));
  }

  GlobalVariable *getGlobalVariableField(unsigned Elt) const;
  Constant *getConstantField(unsigned Elt) const;
  Function *getFunctionField(unsigned Elt) const;
  void replaceFunctionField(unsigned Elt, Function *F);

public:
  explicit DIDescriptor(const MDNode *N = nullptr) : DbgNode(N) {}

  bool Verify() const;

  operator MDNode *() const { return const_cast<MDNode *>(DbgNode); }
  MDNode *operator->() const { return const_cast<MDNode *>(DbgNode); }

  // An explicit operator bool so that we can do testing of DI values
  // easily.
  // FIXME: This operator bool isn't actually protecting anything at the
  // moment due to the conversion operator above making DIDescriptor nodes
  // implicitly convertable to bool.
  LLVM_EXPLICIT operator bool() const { return DbgNode != nullptr; }

  bool operator==(DIDescriptor Other) const { return DbgNode == Other.DbgNode; }
  bool operator!=(DIDescriptor Other) const { return !operator==(Other); }

  uint16_t getTag() const {
    return getUnsignedField(0) & ~LLVMDebugVersionMask;
  }

  bool isDerivedType() const;
  bool isCompositeType() const;
  bool isSubroutineType() const;
  bool isBasicType() const;
  bool isVariable() const;
  bool isSubprogram() const;
  bool isGlobalVariable() const;
  bool isScope() const;
  bool isFile() const;
  bool isCompileUnit() const;
  bool isNameSpace() const;
  bool isLexicalBlockFile() const;
  bool isLexicalBlock() const;
  bool isSubrange() const;
  bool isEnumerator() const;
  bool isType() const;
  bool isTemplateTypeParameter() const;
  bool isTemplateValueParameter() const;
  bool isObjCProperty() const;
  bool isImportedEntity() const;

  /// print - print descriptor.
  void print(raw_ostream &OS) const;

  /// dump - print descriptor to dbgs() with a newline.
  void dump() const;
};

/// DISubrange - This is used to represent ranges, for array bounds.
class DISubrange : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DISubrange(const MDNode *N = nullptr) : DIDescriptor(N) {}

  int64_t getLo() const { return getInt64Field(1); }
  int64_t getCount() const { return getInt64Field(2); }
  bool Verify() const;
};

/// DITypedArray - This descriptor holds an array of nodes with type T.
template <typename T> class DITypedArray : public DIDescriptor {
public:
  explicit DITypedArray(const MDNode *N = nullptr) : DIDescriptor(N) {}
  unsigned getNumElements() const {
    return DbgNode ? DbgNode->getNumOperands() : 0;
  }
  T getElement(unsigned Idx) const {
    return getFieldAs<T>(Idx);
  }
};

typedef DITypedArray<DIDescriptor> DIArray;

/// DIEnumerator - A wrapper for an enumerator (e.g. X and Y in 'enum {X,Y}').
/// FIXME: it seems strange that this doesn't have either a reference to the
/// type/precision or a file/line pair for location info.
class DIEnumerator : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIEnumerator(const MDNode *N = nullptr) : DIDescriptor(N) {}

  StringRef getName() const { return getStringField(1); }
  int64_t getEnumValue() const { return getInt64Field(2); }
  bool Verify() const;
};

template <typename T> class DIRef;
typedef DIRef<DIScope> DIScopeRef;
typedef DIRef<DIType> DITypeRef;
typedef DITypedArray<DITypeRef> DITypeArray;

/// DIScope - A base class for various scopes.
///
/// Although, implementation-wise, DIScope is the parent class of most
/// other DIxxx classes, including DIType and its descendants, most of
/// DIScope's descendants are not a substitutable subtype of
/// DIScope. The DIDescriptor::isScope() method only is true for
/// DIScopes that are scopes in the strict lexical scope sense
/// (DICompileUnit, DISubprogram, etc.), but not for, e.g., a DIType.
class DIScope : public DIDescriptor {
protected:
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIScope(const MDNode *N = nullptr) : DIDescriptor(N) {}

  /// Gets the parent scope for this scope node or returns a
  /// default constructed scope.
  DIScopeRef getContext() const;
  /// If the scope node has a name, return that, else return an empty string.
  StringRef getName() const;
  StringRef getFilename() const;
  StringRef getDirectory() const;

  /// Generate a reference to this DIScope. Uses the type identifier instead
  /// of the actual MDNode if possible, to help type uniquing.
  DIScopeRef getRef() const;
};

/// Represents reference to a DIDescriptor, abstracts over direct and
/// identifier-based metadata references.
template <typename T> class DIRef {
  template <typename DescTy>
  friend DescTy DIDescriptor::getFieldAs(unsigned Elt) const;
  friend DIScopeRef DIScope::getContext() const;
  friend DIScopeRef DIScope::getRef() const;
  friend class DIType;

  /// Val can be either a MDNode or a MDString, in the latter,
  /// MDString specifies the type identifier.
  const Value *Val;
  explicit DIRef(const Value *V);

public:
  T resolve(const DITypeIdentifierMap &Map) const;
  StringRef getName() const;
  operator Value *() const { return const_cast<Value *>(Val); }
};

template <typename T>
T DIRef<T>::resolve(const DITypeIdentifierMap &Map) const {
  if (!Val)
    return T();

  if (const MDNode *MD = dyn_cast<MDNode>(Val))
    return T(MD);

  const MDString *MS = cast<MDString>(Val);
  // Find the corresponding MDNode.
  DITypeIdentifierMap::const_iterator Iter = Map.find(MS);
  assert(Iter != Map.end() && "Identifier not in the type map?");
  assert(DIDescriptor(Iter->second).isType() &&
         "MDNode in DITypeIdentifierMap should be a DIType.");
  return T(Iter->second);
}

template <typename T> StringRef DIRef<T>::getName() const {
  if (!Val)
    return StringRef();

  if (const MDNode *MD = dyn_cast<MDNode>(Val))
    return T(MD).getName();

  const MDString *MS = cast<MDString>(Val);
  return MS->getString();
}

/// Specialize getFieldAs to handle fields that are references to DIScopes.
template <> DIScopeRef DIDescriptor::getFieldAs<DIScopeRef>(unsigned Elt) const;
/// Specialize DIRef constructor for DIScopeRef.
template <> DIRef<DIScope>::DIRef(const Value *V);

/// Specialize getFieldAs to handle fields that are references to DITypes.
template <> DITypeRef DIDescriptor::getFieldAs<DITypeRef>(unsigned Elt) const;
/// Specialize DIRef constructor for DITypeRef.
template <> DIRef<DIType>::DIRef(const Value *V);

/// DIType - This is a wrapper for a type.
/// FIXME: Types should be factored much better so that CV qualifiers and
/// others do not require a huge and empty descriptor full of zeros.
class DIType : public DIScope {
protected:
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIType(const MDNode *N = nullptr) : DIScope(N) {}
  operator DITypeRef () const {
    assert(isType() &&
           "constructing DITypeRef from an MDNode that is not a type");
    return DITypeRef(&*getRef());
  }

  /// Verify - Verify that a type descriptor is well formed.
  bool Verify() const;

  DIScopeRef getContext() const { return getFieldAs<DIScopeRef>(2); }
  StringRef getName() const { return getStringField(3); }
  unsigned getLineNumber() const { return getUnsignedField(4); }
  uint64_t getSizeInBits() const { return getUInt64Field(5); }
  uint64_t getAlignInBits() const { return getUInt64Field(6); }
  // FIXME: Offset is only used for DW_TAG_member nodes.  Making every type
  // carry this is just plain insane.
  uint64_t getOffsetInBits() const { return getUInt64Field(7); }
  unsigned getFlags() const { return getUnsignedField(8); }
  bool isPrivate() const { return (getFlags() & FlagPrivate) != 0; }
  bool isProtected() const { return (getFlags() & FlagProtected) != 0; }
  bool isForwardDecl() const { return (getFlags() & FlagFwdDecl) != 0; }
  // isAppleBlock - Return true if this is the Apple Blocks extension.
  bool isAppleBlockExtension() const {
    return (getFlags() & FlagAppleBlock) != 0;
  }
  bool isBlockByrefStruct() const {
    return (getFlags() & FlagBlockByrefStruct) != 0;
  }
  bool isVirtual() const { return (getFlags() & FlagVirtual) != 0; }
  bool isArtificial() const { return (getFlags() & FlagArtificial) != 0; }
  bool isObjectPointer() const { return (getFlags() & FlagObjectPointer) != 0; }
  bool isObjcClassComplete() const {
    return (getFlags() & FlagObjcClassComplete) != 0;
  }
  bool isVector() const { return (getFlags() & FlagVector) != 0; }
  bool isStaticMember() const { return (getFlags() & FlagStaticMember) != 0; }
  bool isLValueReference() const {
    return (getFlags() & FlagLValueReference) != 0;
  }
  bool isRValueReference() const {
    return (getFlags() & FlagRValueReference) != 0;
  }
  bool isValid() const { return DbgNode && isType(); }

  /// replaceAllUsesWith - Replace all uses of debug info referenced by
  /// this descriptor.
  void replaceAllUsesWith(LLVMContext &VMContext, DIDescriptor D);
  void replaceAllUsesWith(MDNode *D);
};

/// DIBasicType - A basic type, like 'int' or 'float'.
class DIBasicType : public DIType {
public:
  explicit DIBasicType(const MDNode *N = nullptr) : DIType(N) {}

  unsigned getEncoding() const { return getUnsignedField(9); }

  /// Verify - Verify that a basic type descriptor is well formed.
  bool Verify() const;
};

/// DIDerivedType - A simple derived type, like a const qualified type,
/// a typedef, a pointer or reference, et cetera.  Or, a data member of
/// a class/struct/union.
class DIDerivedType : public DIType {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIDerivedType(const MDNode *N = nullptr) : DIType(N) {}

  DITypeRef getTypeDerivedFrom() const { return getFieldAs<DITypeRef>(9); }

  /// getObjCProperty - Return property node, if this ivar is
  /// associated with one.
  MDNode *getObjCProperty() const;

  DITypeRef getClassType() const {
    assert(getTag() == dwarf::DW_TAG_ptr_to_member_type);
    return getFieldAs<DITypeRef>(10);
  }

  Constant *getConstant() const {
    assert((getTag() == dwarf::DW_TAG_member) && isStaticMember());
    return getConstantField(10);
  }

  /// Verify - Verify that a derived type descriptor is well formed.
  bool Verify() const;
};

/// DICompositeType - This descriptor holds a type that can refer to multiple
/// other types, like a function or struct.
/// DICompositeType is derived from DIDerivedType because some
/// composite types (such as enums) can be derived from basic types
// FIXME: Make this derive from DIType directly & just store the
// base type in a single DIType field.
class DICompositeType : public DIDerivedType {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;
  void setArraysHelper(MDNode *Elements, MDNode *TParams);

public:
  explicit DICompositeType(const MDNode *N = nullptr) : DIDerivedType(N) {}

  DIArray getElements() const {
    assert(!isSubroutineType() && "no elements for DISubroutineType");
    return getFieldAs<DIArray>(10);
  }
  template <typename T>
  void setArrays(DITypedArray<T> Elements, DIArray TParams = DIArray()) {
    assert((!TParams || DbgNode->getNumOperands() == 15) &&
           "If you're setting the template parameters this should include a slot "
           "for that!");
    setArraysHelper(Elements, TParams);
  }
  unsigned getRunTimeLang() const { return getUnsignedField(11); }
  DITypeRef getContainingType() const { return getFieldAs<DITypeRef>(12); }
  void setContainingType(DICompositeType ContainingType);
  DIArray getTemplateParams() const { return getFieldAs<DIArray>(13); }
  MDString *getIdentifier() const;

  /// Verify - Verify that a composite type descriptor is well formed.
  bool Verify() const;
};

class DISubroutineType : public DICompositeType {
public:
  explicit DISubroutineType(const MDNode *N = nullptr) : DICompositeType(N) {}
  DITypedArray<DITypeRef> getTypeArray() const {
    return getFieldAs<DITypedArray<DITypeRef>>(10);
  }
};

/// DIFile - This is a wrapper for a file.
class DIFile : public DIScope {
  friend class DIDescriptor;

public:
  explicit DIFile(const MDNode *N = nullptr) : DIScope(N) {}
  MDNode *getFileNode() const;
  bool Verify() const;
};

/// DICompileUnit - A wrapper for a compile unit.
class DICompileUnit : public DIScope {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DICompileUnit(const MDNode *N = nullptr) : DIScope(N) {}

  dwarf::SourceLanguage getLanguage() const {
    return static_cast<dwarf::SourceLanguage>(getUnsignedField(2));
  }
  StringRef getProducer() const { return getStringField(3); }

  bool isOptimized() const { return getUnsignedField(4) != 0; }
  StringRef getFlags() const { return getStringField(5); }
  unsigned getRunTimeVersion() const { return getUnsignedField(6); }

  DIArray getEnumTypes() const;
  DIArray getRetainedTypes() const;
  DIArray getSubprograms() const;
  DIArray getGlobalVariables() const;
  DIArray getImportedEntities() const;

  StringRef getSplitDebugFilename() const { return getStringField(12); }
  unsigned getEmissionKind() const { return getUnsignedField(13); }

  /// Verify - Verify that a compile unit is well formed.
  bool Verify() const;
};

/// DISubprogram - This is a wrapper for a subprogram (e.g. a function).
class DISubprogram : public DIScope {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DISubprogram(const MDNode *N = nullptr) : DIScope(N) {}

  DIScopeRef getContext() const { return getFieldAs<DIScopeRef>(2); }
  StringRef getName() const { return getStringField(3); }
  StringRef getDisplayName() const { return getStringField(4); }
  StringRef getLinkageName() const { return getStringField(5); }
  unsigned getLineNumber() const { return getUnsignedField(6); }
  DISubroutineType getType() const { return getFieldAs<DISubroutineType>(7); }

  /// isLocalToUnit - Return true if this subprogram is local to the current
  /// compile unit, like 'static' in C.
  unsigned isLocalToUnit() const { return getUnsignedField(8); }
  unsigned isDefinition() const { return getUnsignedField(9); }

  unsigned getVirtuality() const { return getUnsignedField(10); }
  unsigned getVirtualIndex() const { return getUnsignedField(11); }

  DITypeRef getContainingType() const { return getFieldAs<DITypeRef>(12); }

  unsigned getFlags() const { return getUnsignedField(13); }

  unsigned isArtificial() const {
    return (getUnsignedField(13) & FlagArtificial) != 0;
  }
  /// isPrivate - Return true if this subprogram has "private"
  /// access specifier.
  bool isPrivate() const { return (getUnsignedField(13) & FlagPrivate) != 0; }
  /// isProtected - Return true if this subprogram has "protected"
  /// access specifier.
  bool isProtected() const {
    return (getUnsignedField(13) & FlagProtected) != 0;
  }
  /// isExplicit - Return true if this subprogram is marked as explicit.
  bool isExplicit() const { return (getUnsignedField(13) & FlagExplicit) != 0; }
  /// isPrototyped - Return true if this subprogram is prototyped.
  bool isPrototyped() const {
    return (getUnsignedField(13) & FlagPrototyped) != 0;
  }

  /// Return true if this subprogram is a C++11 reference-qualified
  /// non-static member function (void foo() &).
  unsigned isLValueReference() const {
    return (getUnsignedField(13) & FlagLValueReference) != 0;
  }

  /// Return true if this subprogram is a C++11
  /// rvalue-reference-qualified non-static member function
  /// (void foo() &&).
  unsigned isRValueReference() const {
    return (getUnsignedField(13) & FlagRValueReference) != 0;
  }

  unsigned isOptimized() const;

  /// Verify - Verify that a subprogram descriptor is well formed.
  bool Verify() const;

  /// describes - Return true if this subprogram provides debugging
  /// information for the function F.
  bool describes(const Function *F);

  Function *getFunction() const { return getFunctionField(15); }
  void replaceFunction(Function *F) { replaceFunctionField(15, F); }
  DIArray getTemplateParams() const { return getFieldAs<DIArray>(16); }
  DISubprogram getFunctionDeclaration() const {
    return getFieldAs<DISubprogram>(17);
  }
  MDNode *getVariablesNodes() const;
  DIArray getVariables() const;

  /// getScopeLineNumber - Get the beginning of the scope of the
  /// function, not necessarily where the name of the program
  /// starts.
  unsigned getScopeLineNumber() const { return getUnsignedField(19); }
};

/// DILexicalBlock - This is a wrapper for a lexical block.
class DILexicalBlock : public DIScope {
public:
  explicit DILexicalBlock(const MDNode *N = nullptr) : DIScope(N) {}
  DIScope getContext() const { return getFieldAs<DIScope>(2); }
  unsigned getLineNumber() const { return getUnsignedField(3); }
  unsigned getColumnNumber() const { return getUnsignedField(4); }
  unsigned getDiscriminator() const { return getUnsignedField(5); }
  bool Verify() const;
};

/// DILexicalBlockFile - This is a wrapper for a lexical block with
/// a filename change.
class DILexicalBlockFile : public DIScope {
public:
  explicit DILexicalBlockFile(const MDNode *N = nullptr) : DIScope(N) {}
  DIScope getContext() const {
    if (getScope().isSubprogram())
      return getScope();
    return getScope().getContext();
  }
  unsigned getLineNumber() const { return getScope().getLineNumber(); }
  unsigned getColumnNumber() const { return getScope().getColumnNumber(); }
  DILexicalBlock getScope() const { return getFieldAs<DILexicalBlock>(2); }
  bool Verify() const;
};

/// DINameSpace - A wrapper for a C++ style name space.
class DINameSpace : public DIScope {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DINameSpace(const MDNode *N = nullptr) : DIScope(N) {}
  DIScope getContext() const { return getFieldAs<DIScope>(2); }
  StringRef getName() const { return getStringField(3); }
  unsigned getLineNumber() const { return getUnsignedField(4); }
  bool Verify() const;
};

/// DITemplateTypeParameter - This is a wrapper for template type parameter.
class DITemplateTypeParameter : public DIDescriptor {
public:
  explicit DITemplateTypeParameter(const MDNode *N = nullptr)
    : DIDescriptor(N) {}

  DIScopeRef getContext() const { return getFieldAs<DIScopeRef>(1); }
  StringRef getName() const { return getStringField(2); }
  DITypeRef getType() const { return getFieldAs<DITypeRef>(3); }
  StringRef getFilename() const { return getFieldAs<DIFile>(4).getFilename(); }
  StringRef getDirectory() const {
    return getFieldAs<DIFile>(4).getDirectory();
  }
  unsigned getLineNumber() const { return getUnsignedField(5); }
  unsigned getColumnNumber() const { return getUnsignedField(6); }
  bool Verify() const;
};

/// DITemplateValueParameter - This is a wrapper for template value parameter.
class DITemplateValueParameter : public DIDescriptor {
public:
  explicit DITemplateValueParameter(const MDNode *N = nullptr)
    : DIDescriptor(N) {}

  DIScopeRef getContext() const { return getFieldAs<DIScopeRef>(1); }
  StringRef getName() const { return getStringField(2); }
  DITypeRef getType() const { return getFieldAs<DITypeRef>(3); }
  Value *getValue() const;
  StringRef getFilename() const { return getFieldAs<DIFile>(5).getFilename(); }
  StringRef getDirectory() const {
    return getFieldAs<DIFile>(5).getDirectory();
  }
  unsigned getLineNumber() const { return getUnsignedField(6); }
  unsigned getColumnNumber() const { return getUnsignedField(7); }
  bool Verify() const;
};

/// DIGlobalVariable - This is a wrapper for a global variable.
class DIGlobalVariable : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIGlobalVariable(const MDNode *N = nullptr) : DIDescriptor(N) {}

  DIScope getContext() const { return getFieldAs<DIScope>(2); }
  StringRef getName() const { return getStringField(3); }
  StringRef getDisplayName() const { return getStringField(4); }
  StringRef getLinkageName() const { return getStringField(5); }
  StringRef getFilename() const { return getFieldAs<DIFile>(6).getFilename(); }
  StringRef getDirectory() const {
    return getFieldAs<DIFile>(6).getDirectory();
  }

  unsigned getLineNumber() const { return getUnsignedField(7); }
  DITypeRef getType() const { return getFieldAs<DITypeRef>(8); }
  unsigned isLocalToUnit() const { return getUnsignedField(9); }
  unsigned isDefinition() const { return getUnsignedField(10); }

  GlobalVariable *getGlobal() const { return getGlobalVariableField(11); }
  Constant *getConstant() const { return getConstantField(11); }
  DIDerivedType getStaticDataMemberDeclaration() const {
    return getFieldAs<DIDerivedType>(12);
  }

  /// Verify - Verify that a global variable descriptor is well formed.
  bool Verify() const;
};

/// DIVariable - This is a wrapper for a variable (e.g. parameter, local,
/// global etc).
class DIVariable : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIVariable(const MDNode *N = nullptr) : DIDescriptor(N) {}

  DIScope getContext() const { return getFieldAs<DIScope>(1); }
  StringRef getName() const { return getStringField(2); }
  DIFile getFile() const { return getFieldAs<DIFile>(3); }
  unsigned getLineNumber() const { return (getUnsignedField(4) << 8) >> 8; }
  unsigned getArgNumber() const {
    unsigned L = getUnsignedField(4);
    return L >> 24;
  }
  DITypeRef getType() const { return getFieldAs<DITypeRef>(5); }

  /// isArtificial - Return true if this variable is marked as "artificial".
  bool isArtificial() const {
    return (getUnsignedField(6) & FlagArtificial) != 0;
  }

  bool isObjectPointer() const {
    return (getUnsignedField(6) & FlagObjectPointer) != 0;
  }

  /// \brief Return true if this variable is represented as a pointer.
  bool isIndirect() const {
    return (getUnsignedField(6) & FlagIndirectVariable) != 0;
  }

  /// getInlinedAt - If this variable is inlined then return inline location.
  MDNode *getInlinedAt() const;

  /// Verify - Verify that a variable descriptor is well formed.
  bool Verify() const;

  /// HasComplexAddr - Return true if the variable has a complex address.
  bool hasComplexAddress() const { return getNumAddrElements() > 0; }

  /// \brief Return the size of this variable's complex address or
  /// zero if there is none.
  unsigned getNumAddrElements() const {
    if (DbgNode->getNumOperands() < 9)
      return 0;
    return getDescriptorField(8)->getNumOperands();
  }

  /// \brief return the Idx'th complex address element.
  uint64_t getAddrElement(unsigned Idx) const;

  /// isBlockByrefVariable - Return true if the variable was declared as
  /// a "__block" variable (Apple Blocks).
  bool isBlockByrefVariable(const DITypeIdentifierMap &Map) const {
    return (getType().resolve(Map)).isBlockByrefStruct();
  }

  /// isInlinedFnArgument - Return true if this variable provides debugging
  /// information for an inlined function arguments.
  bool isInlinedFnArgument(const Function *CurFn);

  /// isVariablePiece - Return whether this is a piece of an aggregate
  /// variable.
  bool isVariablePiece() const;
  /// getPieceOffset - Return the offset of this piece in bytes.
  uint64_t getPieceOffset() const;
  /// getPieceSize - Return the size of this piece in bytes.
  uint64_t getPieceSize() const;

  /// Return the size reported by the variable's type.
  unsigned getSizeInBits(const DITypeIdentifierMap &Map);

  void printExtendedName(raw_ostream &OS) const;
};

/// DILocation - This object holds location information. This object
/// is not associated with any DWARF tag.
class DILocation : public DIDescriptor {
public:
  explicit DILocation(const MDNode *N) : DIDescriptor(N) {}

  unsigned getLineNumber() const { return getUnsignedField(0); }
  unsigned getColumnNumber() const { return getUnsignedField(1); }
  DIScope getScope() const { return getFieldAs<DIScope>(2); }
  DILocation getOrigLocation() const { return getFieldAs<DILocation>(3); }
  StringRef getFilename() const { return getScope().getFilename(); }
  StringRef getDirectory() const { return getScope().getDirectory(); }
  bool Verify() const;
  bool atSameLineAs(const DILocation &Other) const {
    return (getLineNumber() == Other.getLineNumber() &&
            getFilename() == Other.getFilename());
  }
  /// getDiscriminator - DWARF discriminators are used to distinguish
  /// identical file locations for instructions that are on different
  /// basic blocks. If two instructions are inside the same lexical block
  /// and are in different basic blocks, we create a new lexical block
  /// with identical location as the original but with a different
  /// discriminator value (lib/Transforms/Util/AddDiscriminators.cpp
  /// for details).
  unsigned getDiscriminator() const {
    // Since discriminators are associated with lexical blocks, make
    // sure this location is a lexical block before retrieving its
    // value.
    return getScope().isLexicalBlock()
               ? getFieldAs<DILexicalBlock>(2).getDiscriminator()
               : 0;
  }
  unsigned computeNewDiscriminator(LLVMContext &Ctx);
  DILocation copyWithNewScope(LLVMContext &Ctx, DILexicalBlock NewScope);
};

class DIObjCProperty : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIObjCProperty(const MDNode *N) : DIDescriptor(N) {}

  StringRef getObjCPropertyName() const { return getStringField(1); }
  DIFile getFile() const { return getFieldAs<DIFile>(2); }
  unsigned getLineNumber() const { return getUnsignedField(3); }

  StringRef getObjCPropertyGetterName() const { return getStringField(4); }
  StringRef getObjCPropertySetterName() const { return getStringField(5); }
  bool isReadOnlyObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_readonly) != 0;
  }
  bool isReadWriteObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_readwrite) != 0;
  }
  bool isAssignObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_assign) != 0;
  }
  bool isRetainObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_retain) != 0;
  }
  bool isCopyObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_copy) != 0;
  }
  bool isNonAtomicObjCProperty() const {
    return (getUnsignedField(6) & dwarf::DW_APPLE_PROPERTY_nonatomic) != 0;
  }

  /// Objective-C doesn't have an ODR, so there is no benefit in storing
  /// the type as a DITypeRef here.
  DIType getType() const { return getFieldAs<DIType>(7); }

  /// Verify - Verify that a derived type descriptor is well formed.
  bool Verify() const;
};

/// \brief An imported module (C++ using directive or similar).
class DIImportedEntity : public DIDescriptor {
  friend class DIDescriptor;
  void printInternal(raw_ostream &OS) const;

public:
  explicit DIImportedEntity(const MDNode *N) : DIDescriptor(N) {}
  DIScope getContext() const { return getFieldAs<DIScope>(1); }
  DIScopeRef getEntity() const { return getFieldAs<DIScopeRef>(2); }
  unsigned getLineNumber() const { return getUnsignedField(3); }
  StringRef getName() const { return getStringField(4); }
  bool Verify() const;
};

/// getDISubprogram - Find subprogram that is enclosing this scope.
DISubprogram getDISubprogram(const MDNode *Scope);

/// getDICompositeType - Find underlying composite type.
DICompositeType getDICompositeType(DIType T);

/// getOrInsertFnSpecificMDNode - Return a NameMDNode that is suitable
/// to hold function specific information.
NamedMDNode *getOrInsertFnSpecificMDNode(Module &M, DISubprogram SP);

/// getFnSpecificMDNode - Return a NameMDNode, if available, that is
/// suitable to hold function specific information.
NamedMDNode *getFnSpecificMDNode(const Module &M, DISubprogram SP);

/// createInlinedVariable - Create a new inlined variable based on current
/// variable.
/// @param DV            Current Variable.
/// @param InlinedScope  Location at current variable is inlined.
DIVariable createInlinedVariable(MDNode *DV, MDNode *InlinedScope,
                                 LLVMContext &VMContext);

/// cleanseInlinedVariable - Remove inlined scope from the variable.
DIVariable cleanseInlinedVariable(MDNode *DV, LLVMContext &VMContext);

/// getEntireVariable - Remove OpPiece exprs from the variable.
DIVariable getEntireVariable(DIVariable DV);

/// Construct DITypeIdentifierMap by going through retained types of each CU.
DITypeIdentifierMap generateDITypeIdentifierMap(const NamedMDNode *CU_Nodes);

/// Strip debug info in the module if it exists.
/// To do this, we remove all calls to the debugger intrinsics and any named
/// metadata for debugging. We also remove debug locations for instructions.
/// Return true if module is modified.
bool StripDebugInfo(Module &M);

/// Return Debug Info Metadata Version by checking module flags.
unsigned getDebugMetadataVersionFromModule(const Module &M);

/// DebugInfoFinder tries to list all debug info MDNodes used in a module. To
/// list debug info MDNodes used by an instruction, DebugInfoFinder uses
/// processDeclare, processValue and processLocation to handle DbgDeclareInst,
/// DbgValueInst and DbgLoc attached to instructions. processModule will go
/// through all DICompileUnits in llvm.dbg.cu and list debug info MDNodes
/// used by the CUs.
class DebugInfoFinder {
public:
  DebugInfoFinder() : TypeMapInitialized(false) {}

  /// processModule - Process entire module and collect debug info
  /// anchors.
  void processModule(const Module &M);

  /// processDeclare - Process DbgDeclareInst.
  void processDeclare(const Module &M, const DbgDeclareInst *DDI);
  /// Process DbgValueInst.
  void processValue(const Module &M, const DbgValueInst *DVI);
  /// processLocation - Process DILocation.
  void processLocation(const Module &M, DILocation Loc);

  /// Clear all lists.
  void reset();

private:
  /// Initialize TypeIdentifierMap.
  void InitializeTypeMap(const Module &M);

  /// processType - Process DIType.
  void processType(DIType DT);

  /// processSubprogram - Process DISubprogram.
  void processSubprogram(DISubprogram SP);

  void processScope(DIScope Scope);

  /// addCompileUnit - Add compile unit into CUs.
  bool addCompileUnit(DICompileUnit CU);

  /// addGlobalVariable - Add global variable into GVs.
  bool addGlobalVariable(DIGlobalVariable DIG);

  // addSubprogram - Add subprogram into SPs.
  bool addSubprogram(DISubprogram SP);

  /// addType - Add type into Tys.
  bool addType(DIType DT);

  bool addScope(DIScope Scope);

public:
  typedef SmallVectorImpl<DICompileUnit>::const_iterator compile_unit_iterator;
  typedef SmallVectorImpl<DISubprogram>::const_iterator subprogram_iterator;
  typedef SmallVectorImpl<DIGlobalVariable>::const_iterator global_variable_iterator;
  typedef SmallVectorImpl<DIType>::const_iterator type_iterator;
  typedef SmallVectorImpl<DIScope>::const_iterator scope_iterator;

  iterator_range<compile_unit_iterator> compile_units() const {
    return iterator_range<compile_unit_iterator>(CUs.begin(), CUs.end());
  }

  iterator_range<subprogram_iterator> subprograms() const {
    return iterator_range<subprogram_iterator>(SPs.begin(), SPs.end());
  }

  iterator_range<global_variable_iterator> global_variables() const {
    return iterator_range<global_variable_iterator>(GVs.begin(), GVs.end());
  }

  iterator_range<type_iterator> types() const {
    return iterator_range<type_iterator>(TYs.begin(), TYs.end());
  }

  iterator_range<scope_iterator> scopes() const {
    return iterator_range<scope_iterator>(Scopes.begin(), Scopes.end());
  }

  unsigned compile_unit_count() const { return CUs.size(); }
  unsigned global_variable_count() const { return GVs.size(); }
  unsigned subprogram_count() const { return SPs.size(); }
  unsigned type_count() const { return TYs.size(); }
  unsigned scope_count() const { return Scopes.size(); }

private:
  SmallVector<DICompileUnit, 8> CUs;    // Compile Units
  SmallVector<DISubprogram, 8> SPs;    // Subprograms
  SmallVector<DIGlobalVariable, 8> GVs;    // Global Variables;
  SmallVector<DIType, 8> TYs;    // Types
  SmallVector<DIScope, 8> Scopes; // Scopes
  SmallPtrSet<MDNode *, 64> NodesSeen;
  DITypeIdentifierMap TypeIdentifierMap;
  /// Specify if TypeIdentifierMap is initialized.
  bool TypeMapInitialized;
};

DenseMap<const Function *, DISubprogram> makeSubprogramMap(const Module &M);

} // end namespace llvm

#endif
