//===------ OrcError.h - Reject symbol lookup requests ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//   Define an error category, error codes, and helper utilities for Orc.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_ORCERROR_H
#define LLVM_EXECUTIONENGINE_ORC_ORCERROR_H

#include "llvm/Support/Error.h"
#include <system_error>

namespace llvm {
namespace orc {

enum class OrcErrorCode : int {
  // RPC Errors
  RemoteAllocatorDoesNotExist = 1,
  RemoteAllocatorIdAlreadyInUse,
  RemoteMProtectAddrUnrecognized,
  RemoteIndirectStubsOwnerDoesNotExist,
  RemoteIndirectStubsOwnerIdAlreadyInUse,
  RPCResponseAbandoned,
  UnexpectedRPCCall,
  UnexpectedRPCResponse,
  UnknownRPCFunction
};

Error orcError(OrcErrorCode ErrCode);

} // End namespace orc.
} // End namespace llvm.

#endif // LLVM_EXECUTIONENGINE_ORC_ORCERROR_H
