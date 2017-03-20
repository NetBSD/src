//===-- TypeStreamMerger.cpp ------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/TypeStreamMerger.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/DebugInfo/CodeView/CVTypeVisitor.h"
#include "llvm/DebugInfo/CodeView/TypeDeserializer.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/DebugInfo/CodeView/TypeRecord.h"
#include "llvm/DebugInfo/CodeView/TypeTableBuilder.h"
#include "llvm/DebugInfo/CodeView/TypeVisitorCallbackPipeline.h"
#include "llvm/DebugInfo/CodeView/TypeVisitorCallbacks.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ScopedPrinter.h"

using namespace llvm;
using namespace llvm::codeview;

namespace {

/// Implementation of CodeView type stream merging.
///
/// A CodeView type stream is a series of records that reference each other
/// through type indices. A type index is either "simple", meaning it is less
/// than 0x1000 and refers to a builtin type, or it is complex, meaning it
/// refers to a prior type record in the current stream. The type index of a
/// record is equal to the number of records before it in the stream plus
/// 0x1000.
///
/// Type records are only allowed to use type indices smaller than their own, so
/// a type stream is effectively a topologically sorted DAG. Cycles occuring in
/// the type graph of the source program are resolved with forward declarations
/// of composite types. This class implements the following type stream merging
/// algorithm, which relies on this DAG structure:
///
/// - Begin with a new empty stream, and a new empty hash table that maps from
///   type record contents to new type index.
/// - For each new type stream, maintain a map from source type index to
///   destination type index.
/// - For each record, copy it and rewrite its type indices to be valid in the
///   destination type stream.
/// - If the new type record is not already present in the destination stream
///   hash table, append it to the destination type stream, assign it the next
///   type index, and update the two hash tables.
/// - If the type record already exists in the destination stream, discard it
///   and update the type index map to forward the source type index to the
///   existing destination type index.
class TypeStreamMerger : public TypeVisitorCallbacks {
public:
  TypeStreamMerger(TypeTableBuilder &DestStream)
      : DestStream(DestStream), FieldListBuilder(DestStream) {
    assert(!hadError());
  }

/// TypeVisitorCallbacks overrides.
#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  Error visitKnownRecord(CVType &CVR, Name##Record &Record) override;
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  Error visitKnownMember(CVMemberRecord &CVR, Name##Record &Record) override;
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"

  Error visitUnknownType(CVType &Record) override;

  Error visitTypeBegin(CVType &Record) override;
  Error visitTypeEnd(CVType &Record) override;
  Error visitMemberEnd(CVMemberRecord &Record) override;

  bool mergeStream(const CVTypeArray &Types);

private:
  template <typename RecordType>
  Error visitKnownRecordImpl(RecordType &Record) {
    FoundBadTypeIndex |= !Record.remapTypeIndices(IndexMap);
    IndexMap.push_back(DestStream.writeKnownType(Record));
    return Error::success();
  }

  Error visitKnownRecordImpl(FieldListRecord &Record) {
    CVTypeVisitor Visitor(*this);

    if (auto EC = Visitor.visitFieldListMemberStream(Record.Data))
      return EC;
    return Error::success();
  }

  template <typename RecordType>
  Error visitKnownMemberRecordImpl(RecordType &Record) {
    FoundBadTypeIndex |= !Record.remapTypeIndices(IndexMap);
    FieldListBuilder.writeMemberType(Record);
    return Error::success();
  }

  bool hadError() { return FoundBadTypeIndex; }

  bool FoundBadTypeIndex = false;

  BumpPtrAllocator Allocator;

  TypeTableBuilder &DestStream;
  FieldListRecordBuilder FieldListBuilder;

  bool IsInFieldList{false};
  size_t BeginIndexMapSize = 0;

  /// Map from source type index to destination type index. Indexed by source
  /// type index minus 0x1000.
  SmallVector<TypeIndex, 0> IndexMap;
};

} // end anonymous namespace

Error TypeStreamMerger::visitTypeBegin(CVRecord<TypeLeafKind> &Rec) {
  if (Rec.Type == TypeLeafKind::LF_FIELDLIST) {
    assert(!IsInFieldList);
    IsInFieldList = true;
    FieldListBuilder.begin();
  } else
    BeginIndexMapSize = IndexMap.size();
  return Error::success();
}

Error TypeStreamMerger::visitTypeEnd(CVRecord<TypeLeafKind> &Rec) {
  if (Rec.Type == TypeLeafKind::LF_FIELDLIST) {
    TypeIndex Index = FieldListBuilder.end();
    IndexMap.push_back(Index);
    IsInFieldList = false;
  }
  return Error::success();
}

Error TypeStreamMerger::visitMemberEnd(CVMemberRecord &Rec) {
  assert(IndexMap.size() == BeginIndexMapSize + 1);
  return Error::success();
}

#define TYPE_RECORD(EnumName, EnumVal, Name)                                   \
  Error TypeStreamMerger::visitKnownRecord(CVType &CVR,                        \
                                           Name##Record &Record) {             \
    return visitKnownRecordImpl(Record);                                       \
  }
#define TYPE_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#define MEMBER_RECORD(EnumName, EnumVal, Name)                                 \
  Error TypeStreamMerger::visitKnownMember(CVMemberRecord &CVR,                \
                                           Name##Record &Record) {             \
    return visitKnownMemberRecordImpl(Record);                                 \
  }
#define MEMBER_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/TypeRecords.def"

Error TypeStreamMerger::visitUnknownType(CVType &Rec) {
  // We failed to translate a type. Translate this index as "not translated".
  IndexMap.push_back(
      TypeIndex(SimpleTypeKind::NotTranslated, SimpleTypeMode::Direct));
  return llvm::make_error<CodeViewError>(cv_error_code::corrupt_record);
}

bool TypeStreamMerger::mergeStream(const CVTypeArray &Types) {
  assert(IndexMap.empty());
  TypeVisitorCallbackPipeline Pipeline;

  TypeDeserializer Deserializer;
  Pipeline.addCallbackToPipeline(Deserializer);
  Pipeline.addCallbackToPipeline(*this);

  CVTypeVisitor Visitor(Pipeline);

  if (auto EC = Visitor.visitTypeStream(Types)) {
    consumeError(std::move(EC));
    return false;
  }
  IndexMap.clear();
  return !hadError();
}

bool llvm::codeview::mergeTypeStreams(TypeTableBuilder &DestStream,
                                      const CVTypeArray &Types) {
  return TypeStreamMerger(DestStream).mergeStream(Types);
}
