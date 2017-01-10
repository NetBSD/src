//===- TypeDeserializer.h ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_CODEVIEW_TYPEDESERIALIZER_H
#define LLVM_DEBUGINFO_CODEVIEW_TYPEDESERIALIZER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/DebugInfo/CodeView/CodeView.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/DebugInfo/CodeView/TypeRecordMapping.h"
#include "llvm/DebugInfo/CodeView/TypeVisitorCallbacks.h"
#include "llvm/DebugInfo/MSF/ByteStream.h"
#include "llvm/DebugInfo/MSF/StreamReader.h"
#include "llvm/Support/Error.h"
#include <cassert>
#include <cstdint>
#include <memory>

namespace llvm {
namespace codeview {

class TypeDeserializer : public TypeVisitorCallbacks {
  struct MappingInfo {
    explicit MappingInfo(ArrayRef<uint8_t> RecordData)
        : Stream(RecordData), Reader(Stream), Mapping(Reader) {}

    msf::ByteStream Stream;
    msf::StreamReader Reader;
    TypeRecordMapping Mapping;
  };

public:
  TypeDeserializer() = default;

  Error visitTypeBegin(CVType &Record) override {
    assert(!Mapping && "Already in a type mapping!");
    Mapping = llvm::make_unique<MappingInfo>(Record.content());
    return Mapping->Mapping.visitTypeBegin(Record);
  }

  Error visitTypeEnd(CVType &Record) override {
    assert(Mapping && "Not in a type mapping!");
    auto EC = Mapping->Mapping.visitTypeEnd(Record);
    Mapping.reset();
    return EC;
  }

#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  Error visitKnownRecord(CVType &CVR, Name##Record &Record) override {         \
    return visitKnownRecordImpl<Name##Record>(CVR, Record);                    \
  }
#define MEMBER_RECORD(EnumName, EnumVal, Name)
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "TypeRecords.def"

private:
  template <typename RecordType>
  Error visitKnownRecordImpl(CVType &CVR, RecordType &Record) {
    return Mapping->Mapping.visitKnownRecord(CVR, Record);
  }

  std::unique_ptr<MappingInfo> Mapping;
};

class FieldListDeserializer : public TypeVisitorCallbacks {
  struct MappingInfo {
    explicit MappingInfo(msf::StreamReader &R)
        : Reader(R), Mapping(Reader), StartOffset(0) {}

    msf::StreamReader &Reader;
    TypeRecordMapping Mapping;
    uint32_t StartOffset;
  };

public:
  explicit FieldListDeserializer(msf::StreamReader &Reader) : Mapping(Reader) {
    CVType FieldList;
    FieldList.Type = TypeLeafKind::LF_FIELDLIST;
    consumeError(Mapping.Mapping.visitTypeBegin(FieldList));
  }

  ~FieldListDeserializer() override {
    CVType FieldList;
    FieldList.Type = TypeLeafKind::LF_FIELDLIST;
    consumeError(Mapping.Mapping.visitTypeEnd(FieldList));
  }

  Error visitMemberBegin(CVMemberRecord &Record) override {
    Mapping.StartOffset = Mapping.Reader.getOffset();
    return Mapping.Mapping.visitMemberBegin(Record);
  }

  Error visitMemberEnd(CVMemberRecord &Record) override {
    if (auto EC = Mapping.Mapping.visitMemberEnd(Record))
      return EC;
    return Error::success();
  }

#define TYPE_RECORD(EnumName, EnumVal, Name)
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  Error visitKnownMember(CVMemberRecord &CVR, Name##Record &Record) override { \
    return visitKnownMemberImpl<Name##Record>(CVR, Record);                    \
  }
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "TypeRecords.def"

private:
  template <typename RecordType>
  Error visitKnownMemberImpl(CVMemberRecord &CVR, RecordType &Record) {
    if (auto EC = Mapping.Mapping.visitKnownMember(CVR, Record))
      return EC;

    uint32_t EndOffset = Mapping.Reader.getOffset();
    uint32_t RecordLength = EndOffset - Mapping.StartOffset;
    Mapping.Reader.setOffset(Mapping.StartOffset);
    if (auto EC = Mapping.Reader.readBytes(CVR.Data, RecordLength))
      return EC;
    assert(Mapping.Reader.getOffset() == EndOffset);
    return Error::success();
  }
  MappingInfo Mapping;
};

} // end namespace codeview
} // end namespace llvm

#endif // LLVM_DEBUGINFO_CODEVIEW_TYPEDESERIALIZER_H
