//===- PDBFileBuilder.h - PDB File Creation ---------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_PDB_RAW_PDBFILEBUILDER_H
#define LLVM_DEBUGINFO_PDB_RAW_PDBFILEBUILDER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/Optional.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"
#include "llvm/DebugInfo/PDB/Raw/RawConstants.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"

#include <memory>
#include <vector>

namespace llvm {
namespace msf {
class MSFBuilder;
}
namespace pdb {
class DbiStreamBuilder;
class InfoStreamBuilder;
class TpiStreamBuilder;

class PDBFileBuilder {
public:
  explicit PDBFileBuilder(BumpPtrAllocator &Allocator);
  PDBFileBuilder(const PDBFileBuilder &) = delete;
  PDBFileBuilder &operator=(const PDBFileBuilder &) = delete;

  Error initialize(uint32_t BlockSize);

  msf::MSFBuilder &getMsfBuilder();
  InfoStreamBuilder &getInfoBuilder();
  DbiStreamBuilder &getDbiBuilder();
  TpiStreamBuilder &getTpiBuilder();
  TpiStreamBuilder &getIpiBuilder();

  Error commit(StringRef Filename);

private:
  Expected<msf::MSFLayout> finalizeMsfLayout() const;

  BumpPtrAllocator &Allocator;

  std::unique_ptr<msf::MSFBuilder> Msf;
  std::unique_ptr<InfoStreamBuilder> Info;
  std::unique_ptr<DbiStreamBuilder> Dbi;
  std::unique_ptr<TpiStreamBuilder> Tpi;
  std::unique_ptr<TpiStreamBuilder> Ipi;
};
}
}

#endif
