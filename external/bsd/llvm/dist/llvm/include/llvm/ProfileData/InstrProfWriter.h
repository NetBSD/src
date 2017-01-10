//=-- InstrProfWriter.h - Instrumented profiling writer -----------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for writing profiling data for instrumentation
// based PGO and coverage.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PROFILEDATA_INSTRPROFWRITER_H
#define LLVM_PROFILEDATA_INSTRPROFWRITER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

/// Writer for instrumentation based profile data.
class ProfOStream;
class InstrProfRecordWriterTrait;

class InstrProfWriter {
public:
  typedef SmallDenseMap<uint64_t, InstrProfRecord, 1> ProfilingData;
  enum ProfKind { PF_Unknown = 0, PF_FE, PF_IRLevel };

private:
  bool Sparse;
  StringMap<ProfilingData> FunctionData;
  ProfKind ProfileKind;
  // Use raw pointer here for the incomplete type object.
  InstrProfRecordWriterTrait *InfoObj;

public:
  InstrProfWriter(bool Sparse = false);
  ~InstrProfWriter();

  /// Add function counts for the given function. If there are already counts
  /// for this function and the hash and number of counts match, each counter is
  /// summed. Optionally scale counts by \p Weight.
  Error addRecord(InstrProfRecord &&I, uint64_t Weight = 1);
  /// Merge existing function counts from the given writer.
  Error mergeRecordsFromWriter(InstrProfWriter &&IPW);
  /// Write the profile to \c OS
  void write(raw_fd_ostream &OS);
  /// Write the profile in text format to \c OS
  void writeText(raw_fd_ostream &OS);
  /// Write \c Record in text format to \c OS
  static void writeRecordInText(const InstrProfRecord &Record,
                                InstrProfSymtab &Symtab, raw_fd_ostream &OS);
  /// Write the profile, returning the raw data. For testing.
  std::unique_ptr<MemoryBuffer> writeBuffer();

  /// Set the ProfileKind. Report error if mixing FE and IR level profiles.
  Error setIsIRLevelProfile(bool IsIRLevel) {
    if (ProfileKind == PF_Unknown) {
      ProfileKind = IsIRLevel ? PF_IRLevel: PF_FE;
      return Error::success();
    }
    return (IsIRLevel == (ProfileKind == PF_IRLevel))
               ? Error::success()
               : make_error<InstrProfError>(
                     instrprof_error::unsupported_version);
  }

  // Internal interface for testing purpose only.
  void setValueProfDataEndianness(support::endianness Endianness);
  void setOutputSparse(bool Sparse);

private:
  bool shouldEncodeData(const ProfilingData &PD);
  void writeImpl(ProfOStream &OS);
};

} // end namespace llvm

#endif
