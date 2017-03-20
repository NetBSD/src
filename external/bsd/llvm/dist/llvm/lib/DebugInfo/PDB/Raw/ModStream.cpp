//===- ModStream.cpp - PDB Module Info Stream Access ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/iterator_range.h"
#include "llvm/DebugInfo/CodeView/SymbolRecord.h"
#include "llvm/DebugInfo/MSF/StreamReader.h"
#include "llvm/DebugInfo/MSF/StreamRef.h"
#include "llvm/DebugInfo/PDB/Raw/ModInfo.h"
#include "llvm/DebugInfo/PDB/Raw/ModStream.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"
#include "llvm/DebugInfo/PDB/Raw/RawError.h"
#include "llvm/DebugInfo/PDB/Raw/RawTypes.h"
#include "llvm/Support/Error.h"
#include <algorithm>
#include <cstdint>

using namespace llvm;
using namespace llvm::msf;
using namespace llvm::pdb;

ModStream::ModStream(const ModInfo &Module,
                     std::unique_ptr<MappedBlockStream> Stream)
    : Mod(Module), Stream(std::move(Stream)) {}

ModStream::~ModStream() = default;

Error ModStream::reload() {
  StreamReader Reader(*Stream);

  uint32_t SymbolSize = Mod.getSymbolDebugInfoByteSize();
  uint32_t C11Size = Mod.getLineInfoByteSize();
  uint32_t C13Size = Mod.getC13LineInfoByteSize();

  if (C11Size > 0 && C13Size > 0)
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "Module has both C11 and C13 line info");

  ReadableStreamRef S;

  if (auto EC = Reader.readInteger(Signature))
    return EC;
  if (auto EC = Reader.readArray(SymbolsSubstream, SymbolSize - 4))
    return EC;

  if (auto EC = Reader.readStreamRef(LinesSubstream, C11Size))
    return EC;
  if (auto EC = Reader.readStreamRef(C13LinesSubstream, C13Size))
    return EC;

  StreamReader LineReader(C13LinesSubstream);
  if (auto EC = LineReader.readArray(LineInfo, LineReader.bytesRemaining()))
    return EC;

  uint32_t GlobalRefsSize;
  if (auto EC = Reader.readInteger(GlobalRefsSize))
    return EC;
  if (auto EC = Reader.readStreamRef(GlobalRefsSubstream, GlobalRefsSize))
    return EC;
  if (Reader.bytesRemaining() > 0)
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "Unexpected bytes in module stream.");

  return Error::success();
}

iterator_range<codeview::CVSymbolArray::Iterator>
ModStream::symbols(bool *HadError) const {
  // It's OK if the stream is empty.
  if (SymbolsSubstream.getUnderlyingStream().getLength() == 0)
    return make_range(SymbolsSubstream.end(), SymbolsSubstream.end());
  return make_range(SymbolsSubstream.begin(HadError), SymbolsSubstream.end());
}

iterator_range<codeview::ModuleSubstreamArray::Iterator>
ModStream::lines(bool *HadError) const {
  return make_range(LineInfo.begin(HadError), LineInfo.end());
}

Error ModStream::commit() { return Error::success(); }
