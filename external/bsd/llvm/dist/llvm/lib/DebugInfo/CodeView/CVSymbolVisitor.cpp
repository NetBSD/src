//===- CVSymbolVisitor.cpp --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/CVSymbolVisitor.h"

#include "llvm/DebugInfo/CodeView/CodeViewError.h"
#include "llvm/DebugInfo/CodeView/SymbolVisitorCallbacks.h"
#include "llvm/DebugInfo/MSF/ByteStream.h"

using namespace llvm;
using namespace llvm::codeview;

template <typename T>
static Error takeObject(ArrayRef<uint8_t> &Data, const T *&Res) {
  if (Data.size() < sizeof(*Res))
    return llvm::make_error<CodeViewError>(cv_error_code::insufficient_buffer);
  Res = reinterpret_cast<const T *>(Data.data());
  Data = Data.drop_front(sizeof(*Res));
  return Error::success();
}

CVSymbolVisitor::CVSymbolVisitor(SymbolVisitorCallbacks &Callbacks)
    : Callbacks(Callbacks) {}

template <typename T>
static Error visitKnownRecord(CVSymbol &Record,
                              SymbolVisitorCallbacks &Callbacks) {
  SymbolRecordKind RK = static_cast<SymbolRecordKind>(Record.Type);
  T KnownRecord(RK);
  if (auto EC = Callbacks.visitKnownRecord(Record, KnownRecord))
    return EC;
  return Error::success();
}

Error CVSymbolVisitor::visitSymbolRecord(CVSymbol &Record) {
  if (auto EC = Callbacks.visitSymbolBegin(Record))
    return EC;

  switch (Record.Type) {
  default:
    if (auto EC = Callbacks.visitUnknownSymbol(Record))
      return EC;
    break;
#define SYMBOL_RECORD(EnumName, EnumVal, Name)                                 \
  case EnumName: {                                                             \
    if (auto EC = visitKnownRecord<Name>(Record, Callbacks))                   \
      return EC;                                                               \
    break;                                                                     \
  }
#define SYMBOL_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)                \
  SYMBOL_RECORD(EnumVal, EnumVal, AliasName)
#include "llvm/DebugInfo/CodeView/CVSymbolTypes.def"
  }

  if (auto EC = Callbacks.visitSymbolEnd(Record))
    return EC;

  return Error::success();
}

Error CVSymbolVisitor::visitSymbolStream(const CVSymbolArray &Symbols) {
  for (auto I : Symbols) {
    if (auto EC = visitSymbolRecord(I))
      return EC;
  }
  return Error::success();
}
