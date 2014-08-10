//===- SymbolicFile.cpp - Interface that only provides symbols --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines a file format independent SymbolicFile class.
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/IRObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Object/SymbolicFile.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;
using namespace object;

SymbolicFile::SymbolicFile(unsigned int Type,
                           std::unique_ptr<MemoryBuffer> Source)
    : Binary(Type, std::move(Source)) {}

SymbolicFile::~SymbolicFile() {}

ErrorOr<std::unique_ptr<SymbolicFile>>
SymbolicFile::createSymbolicFile(std::unique_ptr<MemoryBuffer> &Object,
                                 sys::fs::file_magic Type,
                                 LLVMContext *Context) {
  if (Type == sys::fs::file_magic::unknown)
    Type = sys::fs::identify_magic(Object->getBuffer());

  switch (Type) {
  case sys::fs::file_magic::bitcode:
    if (Context)
      return IRObjectFile::createIRObjectFile(std::move(Object), *Context);
  // Fallthrough
  case sys::fs::file_magic::unknown:
  case sys::fs::file_magic::archive:
  case sys::fs::file_magic::macho_universal_binary:
  case sys::fs::file_magic::windows_resource:
    return object_error::invalid_file_type;
  case sys::fs::file_magic::elf_relocatable:
  case sys::fs::file_magic::elf_executable:
  case sys::fs::file_magic::elf_shared_object:
  case sys::fs::file_magic::elf_core:
  case sys::fs::file_magic::macho_object:
  case sys::fs::file_magic::macho_executable:
  case sys::fs::file_magic::macho_fixed_virtual_memory_shared_lib:
  case sys::fs::file_magic::macho_core:
  case sys::fs::file_magic::macho_preload_executable:
  case sys::fs::file_magic::macho_dynamically_linked_shared_lib:
  case sys::fs::file_magic::macho_dynamic_linker:
  case sys::fs::file_magic::macho_bundle:
  case sys::fs::file_magic::macho_dynamically_linked_shared_lib_stub:
  case sys::fs::file_magic::macho_dsym_companion:
  case sys::fs::file_magic::coff_object:
  case sys::fs::file_magic::coff_import_library:
  case sys::fs::file_magic::pecoff_executable:
    return ObjectFile::createObjectFile(Object, Type);
  }
  llvm_unreachable("Unexpected Binary File Type");
}
