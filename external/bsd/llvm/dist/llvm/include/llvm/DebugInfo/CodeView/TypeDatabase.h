//===- TypeDatabase.h - A collection of CodeView type records ---*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_TYPEDATABASE_H
#define LLVM_DEBUGINFO_CODEVIEW_TYPEDATABASE_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/StringSaver.h"

namespace llvm {
namespace codeview {
class TypeDatabase {
public:
  TypeDatabase() : TypeNameStorage(Allocator) {}

  /// Gets the type index for the next type record.
  TypeIndex getNextTypeIndex() const;

  /// Records the name of a type, and reserves its type index.
  void recordType(StringRef Name, CVType Data);

  /// Saves the name in a StringSet and creates a stable StringRef.
  StringRef saveTypeName(StringRef TypeName);

  StringRef getTypeName(TypeIndex Index) const;

  bool containsTypeIndex(TypeIndex Index) const;

  uint32_t size() const;

private:
  BumpPtrAllocator Allocator;

  /// All user defined type records in .debug$T live in here. Type indices
  /// greater than 0x1000 are user defined. Subtract 0x1000 from the index to
  /// index into this vector.
  SmallVector<StringRef, 10> CVUDTNames;
  SmallVector<CVType, 10> TypeRecords;

  StringSaver TypeNameStorage;
};
}
}

#endif