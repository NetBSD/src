//===- TpiStream.cpp - PDB Type Info (TPI) Stream 2 Access ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/iterator_range.h"
#include "llvm/DebugInfo/CodeView/CVTypeVisitor.h"
#include "llvm/DebugInfo/CodeView/TypeDeserializer.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/DebugInfo/CodeView/TypeVisitorCallbackPipeline.h"
#include "llvm/DebugInfo/MSF/MappedBlockStream.h"
#include "llvm/DebugInfo/MSF/StreamReader.h"
#include "llvm/DebugInfo/PDB/Raw/PDBFile.h"
#include "llvm/DebugInfo/PDB/Raw/RawConstants.h"
#include "llvm/DebugInfo/PDB/Raw/RawError.h"
#include "llvm/DebugInfo/PDB/Raw/RawTypes.h"
#include "llvm/DebugInfo/PDB/Raw/TpiHashing.h"
#include "llvm/DebugInfo/PDB/Raw/TpiStream.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include <algorithm>
#include <cstdint>
#include <vector>

using namespace llvm;
using namespace llvm::codeview;
using namespace llvm::support;
using namespace llvm::msf;
using namespace llvm::pdb;

TpiStream::TpiStream(const PDBFile &File,
                     std::unique_ptr<MappedBlockStream> Stream)
    : Pdb(File), Stream(std::move(Stream)) {}

TpiStream::~TpiStream() = default;

// Verifies that a given type record matches with a given hash value.
// Currently we only verify SRC_LINE records.
Error TpiStream::verifyHashValues() {
  TpiHashVerifier Verifier(HashValues, Header->NumHashBuckets);
  TypeDeserializer Deserializer;

  TypeVisitorCallbackPipeline Pipeline;
  Pipeline.addCallbackToPipeline(Deserializer);
  Pipeline.addCallbackToPipeline(Verifier);

  CVTypeVisitor Visitor(Pipeline);
  return Visitor.visitTypeStream(TypeRecords);
}

Error TpiStream::reload() {
  StreamReader Reader(*Stream);

  if (Reader.bytesRemaining() < sizeof(TpiStreamHeader))
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "TPI Stream does not contain a header.");

  if (Reader.readObject(Header))
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "TPI Stream does not contain a header.");

  if (Header->Version != PdbTpiV80)
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "Unsupported TPI Version.");

  if (Header->HeaderSize != sizeof(TpiStreamHeader))
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "Corrupt TPI Header size.");

  if (Header->HashKeySize != sizeof(ulittle32_t))
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "TPI Stream expected 4 byte hash key size.");

  if (Header->NumHashBuckets < MinTpiHashBuckets ||
      Header->NumHashBuckets > MaxTpiHashBuckets)
    return make_error<RawError>(raw_error_code::corrupt_file,
                                "TPI Stream Invalid number of hash buckets.");

  // The actual type records themselves come from this stream
  if (auto EC = Reader.readArray(TypeRecords, Header->TypeRecordBytes))
    return EC;

  // Hash indices, hash values, etc come from the hash stream.
  if (Header->HashStreamIndex != kInvalidStreamIndex) {
    if (Header->HashStreamIndex >= Pdb.getNumStreams())
      return make_error<RawError>(raw_error_code::corrupt_file,
                                  "Invalid TPI hash stream index.");

    auto HS = MappedBlockStream::createIndexedStream(
        Pdb.getMsfLayout(), Pdb.getMsfBuffer(), Header->HashStreamIndex);
    StreamReader HSR(*HS);

    uint32_t NumHashValues =
        Header->HashValueBuffer.Length / sizeof(ulittle32_t);
    if (NumHashValues != NumTypeRecords())
      return make_error<RawError>(
          raw_error_code::corrupt_file,
          "TPI hash count does not match with the number of type records.");
    HSR.setOffset(Header->HashValueBuffer.Off);
    if (auto EC = HSR.readArray(HashValues, NumHashValues))
      return EC;
    std::vector<ulittle32_t> HashValueList;
    for (auto I : HashValues)
      HashValueList.push_back(I);

    HSR.setOffset(Header->IndexOffsetBuffer.Off);
    uint32_t NumTypeIndexOffsets =
        Header->IndexOffsetBuffer.Length / sizeof(TypeIndexOffset);
    if (auto EC = HSR.readArray(TypeIndexOffsets, NumTypeIndexOffsets))
      return EC;

    HSR.setOffset(Header->HashAdjBuffer.Off);
    uint32_t NumHashAdjustments =
        Header->HashAdjBuffer.Length / sizeof(TypeIndexOffset);
    if (auto EC = HSR.readArray(HashAdjustments, NumHashAdjustments))
      return EC;

    HashStream = std::move(HS);

    // TPI hash table is a parallel array for the type records.
    // Verify that the hash values match with type records.
    if (auto EC = verifyHashValues())
      return EC;
  }

  return Error::success();
}

PdbRaw_TpiVer TpiStream::getTpiVersion() const {
  uint32_t Value = Header->Version;
  return static_cast<PdbRaw_TpiVer>(Value);
}

uint32_t TpiStream::TypeIndexBegin() const { return Header->TypeIndexBegin; }

uint32_t TpiStream::TypeIndexEnd() const { return Header->TypeIndexEnd; }

uint32_t TpiStream::NumTypeRecords() const {
  return TypeIndexEnd() - TypeIndexBegin();
}

uint16_t TpiStream::getTypeHashStreamIndex() const {
  return Header->HashStreamIndex;
}

uint16_t TpiStream::getTypeHashStreamAuxIndex() const {
  return Header->HashAuxStreamIndex;
}

uint32_t TpiStream::NumHashBuckets() const { return Header->NumHashBuckets; }
uint32_t TpiStream::getHashKeySize() const { return Header->HashKeySize; }

FixedStreamArray<support::ulittle32_t>
TpiStream::getHashValues() const {
  return HashValues;
}

FixedStreamArray<TypeIndexOffset>
TpiStream::getTypeIndexOffsets() const {
  return TypeIndexOffsets;
}

FixedStreamArray<TypeIndexOffset>
TpiStream::getHashAdjustments() const {
  return HashAdjustments;
}

iterator_range<CVTypeArray::Iterator>
TpiStream::types(bool *HadError) const {
  return make_range(TypeRecords.begin(HadError), TypeRecords.end());
}

Error TpiStream::commit() { return Error::success(); }
