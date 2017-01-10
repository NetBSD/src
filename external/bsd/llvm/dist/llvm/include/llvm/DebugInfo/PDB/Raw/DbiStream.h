//===- DbiStream.h - PDB Dbi Stream (Stream 3) Access -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_PDB_RAW_PDBDBISTREAM_H
#define LLVM_DEBUGINFO_PDB_RAW_PDBDBISTREAM_H

#include "llvm/DebugInfo/CodeView/ModuleSubstream.h"
#include "llvm/DebugInfo/MSF/MappedBlockStream.h"
#include "llvm/DebugInfo/MSF/StreamArray.h"
#include "llvm/DebugInfo/MSF/StreamRef.h"
#include "llvm/DebugInfo/PDB/PDBTypes.h"
#include "llvm/DebugInfo/PDB/Raw/ModInfo.h"
#include "llvm/DebugInfo/PDB/Raw/NameHashTable.h"
#include "llvm/DebugInfo/PDB/Raw/RawConstants.h"
#include "llvm/DebugInfo/PDB/Raw/RawTypes.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"

namespace llvm {
namespace object {
struct FpoData;
struct coff_section;
}

namespace pdb {
class DbiStreamBuilder;
class PDBFile;
class ISectionContribVisitor;

class DbiStream {
  friend class DbiStreamBuilder;

public:
  DbiStream(PDBFile &File, std::unique_ptr<msf::MappedBlockStream> Stream);
  ~DbiStream();
  Error reload();

  PdbRaw_DbiVer getDbiVersion() const;
  uint32_t getAge() const;
  uint16_t getPublicSymbolStreamIndex() const;
  uint16_t getGlobalSymbolStreamIndex() const;

  uint16_t getFlags() const;
  bool isIncrementallyLinked() const;
  bool hasCTypes() const;
  bool isStripped() const;

  uint16_t getBuildNumber() const;
  uint16_t getBuildMajorVersion() const;
  uint16_t getBuildMinorVersion() const;

  uint16_t getPdbDllRbld() const;
  uint32_t getPdbDllVersion() const;

  uint32_t getSymRecordStreamIndex() const;

  PDB_Machine getMachineType() const;

  /// If the given stream type is present, returns its stream index. If it is
  /// not present, returns InvalidStreamIndex.
  uint32_t getDebugStreamIndex(DbgHeaderType Type) const;

  ArrayRef<ModuleInfoEx> modules() const;

  Expected<StringRef> getFileNameForIndex(uint32_t Index) const;

  msf::FixedStreamArray<object::coff_section> getSectionHeaders();

  msf::FixedStreamArray<object::FpoData> getFpoRecords();

  msf::FixedStreamArray<SecMapEntry> getSectionMap() const;
  void visitSectionContributions(ISectionContribVisitor &Visitor) const;

private:
  Error initializeModInfoArray();
  Error initializeSectionContributionData();
  Error initializeSectionHeadersData();
  Error initializeSectionMapData();
  Error initializeFileInfo();
  Error initializeFpoRecords();

  PDBFile &Pdb;
  std::unique_ptr<msf::MappedBlockStream> Stream;

  std::vector<ModuleInfoEx> ModuleInfos;
  NameHashTable ECNames;

  msf::ReadableStreamRef ModInfoSubstream;
  msf::ReadableStreamRef SecContrSubstream;
  msf::ReadableStreamRef SecMapSubstream;
  msf::ReadableStreamRef FileInfoSubstream;
  msf::ReadableStreamRef TypeServerMapSubstream;
  msf::ReadableStreamRef ECSubstream;

  msf::ReadableStreamRef NamesBuffer;

  msf::FixedStreamArray<support::ulittle16_t> DbgStreams;

  PdbRaw_DbiSecContribVer SectionContribVersion;
  msf::FixedStreamArray<SectionContrib> SectionContribs;
  msf::FixedStreamArray<SectionContrib2> SectionContribs2;
  msf::FixedStreamArray<SecMapEntry> SectionMap;
  msf::FixedStreamArray<support::little32_t> FileNameOffsets;

  std::unique_ptr<msf::MappedBlockStream> SectionHeaderStream;
  msf::FixedStreamArray<object::coff_section> SectionHeaders;

  std::unique_ptr<msf::MappedBlockStream> FpoStream;
  msf::FixedStreamArray<object::FpoData> FpoRecords;

  const DbiStreamHeader *Header;
};
}
}

#endif
