//===- llvm/ExecutionEngine/Orc/RawByteChannel.h ----------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_RAWBYTECHANNEL_H
#define LLVM_EXECUTIONENGINE_ORC_RAWBYTECHANNEL_H

#include "OrcError.h"
#include "RPCSerialization.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/Error.h"
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace llvm {
namespace orc {
namespace rpc {

/// Interface for byte-streams to be used with RPC.
class RawByteChannel {
public:
  virtual ~RawByteChannel() {}

  /// Read Size bytes from the stream into *Dst.
  virtual Error readBytes(char *Dst, unsigned Size) = 0;

  /// Read size bytes from *Src and append them to the stream.
  virtual Error appendBytes(const char *Src, unsigned Size) = 0;

  /// Flush the stream if possible.
  virtual Error send() = 0;

  /// Notify the channel that we're starting a message send.
  /// Locks the channel for writing.
  template <typename FunctionIdT, typename SequenceIdT>
  Error startSendMessage(const FunctionIdT &FnId, const SequenceIdT &SeqNo) {
    writeLock.lock();
    return serializeSeq(*this, FnId, SeqNo);
  }

  /// Notify the channel that we're ending a message send.
  /// Unlocks the channel for writing.
  Error endSendMessage() {
    writeLock.unlock();
    return Error::success();
  }

  /// Notify the channel that we're starting a message receive.
  /// Locks the channel for reading.
  template <typename FunctionIdT, typename SequenceNumberT>
  Error startReceiveMessage(FunctionIdT &FnId, SequenceNumberT &SeqNo) {
    readLock.lock();
    return deserializeSeq(*this, FnId, SeqNo);
  }

  /// Notify the channel that we're ending a message receive.
  /// Unlocks the channel for reading.
  Error endReceiveMessage() {
    readLock.unlock();
    return Error::success();
  }

  /// Get the lock for stream reading.
  std::mutex &getReadLock() { return readLock; }

  /// Get the lock for stream writing.
  std::mutex &getWriteLock() { return writeLock; }

private:
  std::mutex readLock, writeLock;
};

template <typename ChannelT, typename T>
class SerializationTraits<
    ChannelT, T, T,
    typename std::enable_if<
        std::is_base_of<RawByteChannel, ChannelT>::value &&
        (std::is_same<T, uint8_t>::value || std::is_same<T, int8_t>::value ||
         std::is_same<T, uint16_t>::value || std::is_same<T, int16_t>::value ||
         std::is_same<T, uint32_t>::value || std::is_same<T, int32_t>::value ||
         std::is_same<T, uint64_t>::value || std::is_same<T, int64_t>::value ||
         std::is_same<T, char>::value)>::type> {
public:
  static Error serialize(ChannelT &C, T V) {
    support::endian::byte_swap<T, support::big>(V);
    return C.appendBytes(reinterpret_cast<const char *>(&V), sizeof(T));
  };

  static Error deserialize(ChannelT &C, T &V) {
    if (auto Err = C.readBytes(reinterpret_cast<char *>(&V), sizeof(T)))
      return Err;
    support::endian::byte_swap<T, support::big>(V);
    return Error::success();
  };
};

template <typename ChannelT>
class SerializationTraits<ChannelT, bool, bool,
                          typename std::enable_if<std::is_base_of<
                              RawByteChannel, ChannelT>::value>::type> {
public:
  static Error serialize(ChannelT &C, bool V) {
    return C.appendBytes(reinterpret_cast<const char *>(&V), 1);
  }

  static Error deserialize(ChannelT &C, bool &V) {
    return C.readBytes(reinterpret_cast<char *>(&V), 1);
  }
};

template <typename ChannelT>
class SerializationTraits<ChannelT, std::string, StringRef,
                          typename std::enable_if<std::is_base_of<
                              RawByteChannel, ChannelT>::value>::type> {
public:
  /// RPC channel serialization for std::strings.
  static Error serialize(RawByteChannel &C, StringRef S) {
    if (auto Err = serializeSeq(C, static_cast<uint64_t>(S.size())))
      return Err;
    return C.appendBytes((const char *)S.data(), S.size());
  }
};

template <typename ChannelT>
class SerializationTraits<ChannelT, std::string, const char *,
                          typename std::enable_if<std::is_base_of<
                              RawByteChannel, ChannelT>::value>::type> {
public:
  static Error serialize(RawByteChannel &C, const char *S) {
    return SerializationTraits<ChannelT, std::string, StringRef>::serialize(C,
                                                                            S);
  }
};

template <typename ChannelT>
class SerializationTraits<ChannelT, std::string, std::string,
                          typename std::enable_if<std::is_base_of<
                              RawByteChannel, ChannelT>::value>::type> {
public:
  /// RPC channel serialization for std::strings.
  static Error serialize(RawByteChannel &C, const std::string &S) {
    return SerializationTraits<ChannelT, std::string, StringRef>::serialize(C,
                                                                            S);
  }

  /// RPC channel deserialization for std::strings.
  static Error deserialize(RawByteChannel &C, std::string &S) {
    uint64_t Count = 0;
    if (auto Err = deserializeSeq(C, Count))
      return Err;
    S.resize(Count);
    return C.readBytes(&S[0], Count);
  }
};

} // end namespace rpc
} // end namespace orc
} // end namespace llvm

#endif // LLVM_EXECUTIONENGINE_ORC_RAWBYTECHANNEL_H
