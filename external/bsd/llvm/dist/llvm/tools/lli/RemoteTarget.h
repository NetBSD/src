//===- RemoteTarget.h - LLVM Remote process JIT execution ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Definition of the RemoteTarget class which executes JITed code in a
// separate address range from where it was built.
//
//===----------------------------------------------------------------------===//

#ifndef REMOTEPROCESS_H
#define REMOTEPROCESS_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Memory.h"
#include <stdlib.h>
#include <string>

namespace llvm {

class RemoteTarget {
  bool IsRunning;

  SmallVector<sys::MemoryBlock, 16> Allocations;

protected:
  std::string ErrorMsg;

public:
  StringRef getErrorMsg() const { return ErrorMsg; }

  /// Allocate space in the remote target address space.
  ///
  /// @param      Size      Amount of space, in bytes, to allocate.
  /// @param      Alignment Required minimum alignment for allocated space.
  /// @param[out] Address   Remote address of the allocated memory.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool allocateSpace(size_t Size,
                             unsigned Alignment,
                             uint64_t &Address);

  /// Load data into the target address space.
  ///
  /// @param      Address   Destination address in the target process.
  /// @param      Data      Source address in the host process.
  /// @param      Size      Number of bytes to copy.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool loadData(uint64_t Address,
                        const void *Data,
                        size_t Size);

  /// Load code into the target address space and prepare it for execution.
  ///
  /// @param      Address   Destination address in the target process.
  /// @param      Data      Source address in the host process.
  /// @param      Size      Number of bytes to copy.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool loadCode(uint64_t Address,
                        const void *Data,
                        size_t Size);

  /// Execute code in the target process. The called function is required
  /// to be of signature int "(*)(void)".
  ///
  /// @param      Address   Address of the loaded function in the target
  ///                       process.
  /// @param[out] RetVal    The integer return value of the called function.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool executeCode(uint64_t Address,
                           int &RetVal);

  /// Minimum alignment for memory permissions. Used to seperate code and
  /// data regions to make sure data doesn't get marked as code or vice
  /// versa.
  ///
  /// @returns Page alignment return value. Default of 4k.
  virtual unsigned getPageAlignment() { return 4096; }

  /// Start the remote process.
  virtual bool create();

  /// Terminate the remote process.
  virtual void stop();

  RemoteTarget() : IsRunning(false), ErrorMsg("") {}
  virtual ~RemoteTarget() { if (IsRunning) stop(); }

  // Create an instance of the system-specific remote target class.
  static RemoteTarget *createRemoteTarget();
  static RemoteTarget *createExternalRemoteTarget(std::string &ChildName);
  static bool hostSupportsExternalRemoteTarget(); 
private:
  // Main processing function for the remote target process. Command messages
  // are received on file descriptor CmdFD and responses come back on OutFD.
  static void doRemoteTargeting(int CmdFD, int OutFD);
};

} // end namespace llvm

#endif
