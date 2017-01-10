//===- SymbolRecordMapping.h ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_SYMBOLRECORDMAPPING_H
#define LLVM_DEBUGINFO_CODEVIEW_SYMBOLRECORDMAPPING_H

#include "llvm/DebugInfo/CodeView/CodeViewRecordIO.h"
#include "llvm/DebugInfo/CodeView/SymbolVisitorCallbacks.h"

namespace llvm {
namespace msf {
class StreamReader;
class StreamWriter;
}

namespace codeview {
class SymbolRecordMapping : public SymbolVisitorCallbacks {
public:
  explicit SymbolRecordMapping(msf::StreamReader &Reader) : IO(Reader) {}
  explicit SymbolRecordMapping(msf::StreamWriter &Writer) : IO(Writer) {}

  Error visitSymbolBegin(CVSymbol &Record) override;
  Error visitSymbolEnd(CVSymbol &Record) override;

#define SYMBOL_RECORD(EnumName, EnumVal, Name)                                 \
  Error visitKnownRecord(CVSymbol &CVR, Name &Record) override;
#define SYMBOL_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "CVSymbolTypes.def"

private:
  Optional<SymbolKind> Kind;

  CodeViewRecordIO IO;
};
}
}

#endif
