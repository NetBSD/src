//===----- RemoteTargetExternal.h - LLVM out-of-process JIT execution -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Definition of the RemoteTargetExternal class which executes JITed code in a
// separate process from where it was built.
//
//===----------------------------------------------------------------------===//

#ifndef LLI_REMOTETARGETEXTERNAL_H
#define LLI_REMOTETARGETEXTERNAL_H

#include "RemoteTarget.h"
#include "RemoteTargetMessage.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Config/config.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/Memory.h"
#include <stdlib.h>
#include <string>

namespace llvm {

class RemoteTargetExternal : public RemoteTarget {
public:
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
  virtual bool loadData(uint64_t Address, const void *Data, size_t Size);

  /// Load code into the target address space and prepare it for execution.
  ///
  /// @param      Address   Destination address in the target process.
  /// @param      Data      Source address in the host process.
  /// @param      Size      Number of bytes to copy.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool loadCode(uint64_t Address, const void *Data, size_t Size);

  /// Execute code in the target process. The called function is required
  /// to be of signature int "(*)(void)".
  ///
  /// @param      Address   Address of the loaded function in the target
  ///                       process.
  /// @param[out] RetVal    The integer return value of the called function.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool executeCode(uint64_t Address, int &RetVal);

  /// Minimum alignment for memory permissions. Used to seperate code and
  /// data regions to make sure data doesn't get marked as code or vice
  /// versa.
  ///
  /// @returns Page alignment return value. Default of 4k.
  virtual unsigned getPageAlignment() { return 4096; }

  /// Start the remote process.
  ///
  /// @returns True on success. On failure, ErrorMsg is updated with
  ///          descriptive text of the encountered error.
  virtual bool create();

  /// Terminate the remote process.
  virtual void stop();

  RemoteTargetExternal(std::string &Name) : RemoteTarget(), ChildName(Name) {}
  virtual ~RemoteTargetExternal();

private:
  std::string ChildName;

  bool SendAllocateSpace(uint32_t Alignment, uint32_t Size);
  bool SendLoadSection(uint64_t Addr,
                       const void *Data,
                       uint32_t Size,
                       bool IsCode);
  bool SendExecute(uint64_t Addr);
  bool SendTerminate();

  // High-level wrappers for receiving data
  bool Receive(LLIMessageType Msg);
  bool Receive(LLIMessageType Msg, int32_t &Data);
  bool Receive(LLIMessageType Msg, uint64_t &Data);

  // Lower level target-independent read/write to deal with errors
  bool ReceiveHeader(LLIMessageType Msg);
  bool ReceivePayload();
  bool SendHeader(LLIMessageType Msg);
  bool SendPayload();

  // Functions to append/retrieve data from the payload
  SmallVector<const void *, 2> SendData;
  SmallVector<void *, 1> ReceiveData; // Future proof
  SmallVector<int, 2> Sizes;
  void AppendWrite(const void *Data, uint32_t Size);
  void AppendRead(void *Data, uint32_t Size);

  // This will get filled in as a point to an OS-specific structure.
  void *ConnectionData;

  bool WriteBytes(const void *Data, size_t Size);
  bool ReadBytes(void *Data, size_t Size);
  void Wait();
};

} // end namespace llvm

#endif // LLI_REMOTETARGETEXTERNAL_H
