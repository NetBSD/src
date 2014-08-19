//===-- GDBRegistrar.cpp - Registers objects with GDB ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "JITRegistrar.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Mutex.h"
#include "llvm/Support/MutexGuard.h"
#include "llvm/Support/ManagedStatic.h"

using namespace llvm;

// This must be kept in sync with gdb/gdb/jit.h .
extern "C" {

  typedef enum {
    JIT_NOACTION = 0,
    JIT_REGISTER_FN,
    JIT_UNREGISTER_FN
  } jit_actions_t;

  struct jit_code_entry {
    struct jit_code_entry *next_entry;
    struct jit_code_entry *prev_entry;
    const char *symfile_addr;
    uint64_t symfile_size;
  };

  struct jit_descriptor {
    uint32_t version;
    // This should be jit_actions_t, but we want to be specific about the
    // bit-width.
    uint32_t action_flag;
    struct jit_code_entry *relevant_entry;
    struct jit_code_entry *first_entry;
  };

  // We put information about the JITed function in this global, which the
  // debugger reads.  Make sure to specify the version statically, because the
  // debugger checks the version before we can set it during runtime.
  struct jit_descriptor __jit_debug_descriptor = { 1, 0, nullptr, nullptr };

  // Debuggers puts a breakpoint in this function.
  LLVM_ATTRIBUTE_NOINLINE void __jit_debug_register_code() {
    // The noinline and the asm prevent calls to this function from being
    // optimized out.
#if !defined(_MSC_VER)
    asm volatile("":::"memory");
#endif
  }

}

namespace {

// Buffer for an in-memory object file in executable memory
typedef llvm::DenseMap< const char*,
                        std::pair<std::size_t, jit_code_entry*> >
  RegisteredObjectBufferMap;

/// Global access point for the JIT debugging interface designed for use with a
/// singleton toolbox. Handles thread-safe registration and deregistration of
/// object files that are in executable memory managed by the client of this
/// class.
class GDBJITRegistrar : public JITRegistrar {
  /// A map of in-memory object files that have been registered with the
  /// JIT interface.
  RegisteredObjectBufferMap ObjectBufferMap;

public:
  /// Instantiates the JIT service.
  GDBJITRegistrar() : ObjectBufferMap() {}

  /// Unregisters each object that was previously registered and releases all
  /// internal resources.
  virtual ~GDBJITRegistrar();

  /// Creates an entry in the JIT registry for the buffer @p Object,
  /// which must contain an object file in executable memory with any
  /// debug information for the debugger.
  void registerObject(const ObjectBuffer &Object) override;

  /// Removes the internal registration of @p Object, and
  /// frees associated resources.
  /// Returns true if @p Object was found in ObjectBufferMap.
  bool deregisterObject(const ObjectBuffer &Object) override;

private:
  /// Deregister the debug info for the given object file from the debugger
  /// and delete any temporary copies.  This private method does not remove
  /// the function from Map so that it can be called while iterating over Map.
  void deregisterObjectInternal(RegisteredObjectBufferMap::iterator I);
};

/// Lock used to serialize all jit registration events, since they
/// modify global variables.
llvm::sys::Mutex JITDebugLock;

/// Do the registration.
void NotifyDebugger(jit_code_entry* JITCodeEntry) {
  __jit_debug_descriptor.action_flag = JIT_REGISTER_FN;

  // Insert this entry at the head of the list.
  JITCodeEntry->prev_entry = nullptr;
  jit_code_entry* NextEntry = __jit_debug_descriptor.first_entry;
  JITCodeEntry->next_entry = NextEntry;
  if (NextEntry) {
    NextEntry->prev_entry = JITCodeEntry;
  }
  __jit_debug_descriptor.first_entry = JITCodeEntry;
  __jit_debug_descriptor.relevant_entry = JITCodeEntry;
  __jit_debug_register_code();
}

GDBJITRegistrar::~GDBJITRegistrar() {
  // Free all registered object files.
  llvm::MutexGuard locked(JITDebugLock);
  for (RegisteredObjectBufferMap::iterator I = ObjectBufferMap.begin(), E = ObjectBufferMap.end();
       I != E; ++I) {
    // Call the private method that doesn't update the map so our iterator
    // doesn't break.
    deregisterObjectInternal(I);
  }
  ObjectBufferMap.clear();
}

void GDBJITRegistrar::registerObject(const ObjectBuffer &Object) {

  const char *Buffer = Object.getBufferStart();
  size_t      Size = Object.getBufferSize();

  assert(Buffer && "Attempt to register a null object with a debugger.");
  llvm::MutexGuard locked(JITDebugLock);
  assert(ObjectBufferMap.find(Buffer) == ObjectBufferMap.end() &&
         "Second attempt to perform debug registration.");
  jit_code_entry* JITCodeEntry = new jit_code_entry();

  if (!JITCodeEntry) {
    llvm::report_fatal_error(
      "Allocation failed when registering a JIT entry!\n");
  } else {
    JITCodeEntry->symfile_addr = Buffer;
    JITCodeEntry->symfile_size = Size;

    ObjectBufferMap[Buffer] = std::make_pair(Size, JITCodeEntry);
    NotifyDebugger(JITCodeEntry);
  }
}

bool GDBJITRegistrar::deregisterObject(const ObjectBuffer& Object) {
  const char *Buffer = Object.getBufferStart();
  llvm::MutexGuard locked(JITDebugLock);
  RegisteredObjectBufferMap::iterator I = ObjectBufferMap.find(Buffer);

  if (I != ObjectBufferMap.end()) {
    deregisterObjectInternal(I);
    ObjectBufferMap.erase(I);
    return true;
  }
  return false;
}

void GDBJITRegistrar::deregisterObjectInternal(
    RegisteredObjectBufferMap::iterator I) {

  jit_code_entry*& JITCodeEntry = I->second.second;

  // Do the unregistration.
  {
    __jit_debug_descriptor.action_flag = JIT_UNREGISTER_FN;

    // Remove the jit_code_entry from the linked list.
    jit_code_entry* PrevEntry = JITCodeEntry->prev_entry;
    jit_code_entry* NextEntry = JITCodeEntry->next_entry;

    if (NextEntry) {
      NextEntry->prev_entry = PrevEntry;
    }
    if (PrevEntry) {
      PrevEntry->next_entry = NextEntry;
    }
    else {
      assert(__jit_debug_descriptor.first_entry == JITCodeEntry);
      __jit_debug_descriptor.first_entry = NextEntry;
    }

    // Tell the debugger which entry we removed, and unregister the code.
    __jit_debug_descriptor.relevant_entry = JITCodeEntry;
    __jit_debug_register_code();
  }

  delete JITCodeEntry;
  JITCodeEntry = nullptr;
}

llvm::ManagedStatic<GDBJITRegistrar> TheRegistrar;

} // end namespace

namespace llvm {

JITRegistrar& JITRegistrar::getGDBRegistrar() {
  return *TheRegistrar;
}

} // namespace llvm
