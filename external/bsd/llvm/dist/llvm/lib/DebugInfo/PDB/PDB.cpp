//===- PDB.cpp - base header file for creating a PDB reader -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/PDB/PDB.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/DebugInfo/PDB/GenericError.h"
#include "llvm/DebugInfo/PDB/IPDBSession.h"
#include "llvm/DebugInfo/PDB/PDB.h"
#if LLVM_ENABLE_DIA_SDK
#include "llvm/DebugInfo/PDB/DIA/DIASession.h"
#endif
#include "llvm/DebugInfo/PDB/Raw/RawSession.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/ManagedStatic.h"

using namespace llvm;
using namespace llvm::pdb;

Error llvm::pdb::loadDataForPDB(PDB_ReaderType Type, StringRef Path,
                                std::unique_ptr<IPDBSession> &Session) {
  // Create the correct concrete instance type based on the value of Type.
  if (Type == PDB_ReaderType::Raw)
    return RawSession::createFromPdb(Path, Session);

#if LLVM_ENABLE_DIA_SDK
  return DIASession::createFromPdb(Path, Session);
#else
  return llvm::make_error<GenericError>("DIA is not installed on the system");
#endif
}

Error llvm::pdb::loadDataForEXE(PDB_ReaderType Type, StringRef Path,
                                std::unique_ptr<IPDBSession> &Session) {
  // Create the correct concrete instance type based on the value of Type.
  if (Type == PDB_ReaderType::Raw)
    return RawSession::createFromExe(Path, Session);

#if LLVM_ENABLE_DIA_SDK
  return DIASession::createFromExe(Path, Session);
#else
  return llvm::make_error<GenericError>("DIA is not installed on the system");
#endif
}
