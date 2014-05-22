//===-- ObjectImageCommon.h - Format independent executuable object image -===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares a file format independent ObjectImage class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_RUNTIMEDYLD_OBJECTIMAGECOMMON_H
#define LLVM_RUNTIMEDYLD_OBJECTIMAGECOMMON_H

#include "llvm/ExecutionEngine/ObjectBuffer.h"
#include "llvm/ExecutionEngine/ObjectImage.h"
#include "llvm/Object/ObjectFile.h"

namespace llvm {

namespace object {
  class ObjectFile;
}

class ObjectImageCommon : public ObjectImage {
  ObjectImageCommon(); // = delete
  ObjectImageCommon(const ObjectImageCommon &other); // = delete
  virtual void anchor();

protected:
  object::ObjectFile *ObjFile;

  // This form of the constructor allows subclasses to use
  // format-specific subclasses of ObjectFile directly
  ObjectImageCommon(ObjectBuffer *Input, object::ObjectFile *Obj)
  : ObjectImage(Input), // saves Input as Buffer and takes ownership
    ObjFile(Obj)
  {
  }

public:
  ObjectImageCommon(ObjectBuffer* Input)
  : ObjectImage(Input) // saves Input as Buffer and takes ownership
  {
    ObjFile =
        object::ObjectFile::createObjectFile(Buffer->getMemBuffer()).get();
  }
  ObjectImageCommon(object::ObjectFile* Input)
  : ObjectImage(NULL), ObjFile(Input)  {}
  virtual ~ObjectImageCommon() { delete ObjFile; }

  virtual object::symbol_iterator begin_symbols() const
              { return ObjFile->symbol_begin(); }
  virtual object::symbol_iterator end_symbols() const
              { return ObjFile->symbol_end(); }

  virtual object::section_iterator begin_sections() const
              { return ObjFile->section_begin(); }
  virtual object::section_iterator end_sections() const
              { return ObjFile->section_end(); }

  virtual /* Triple::ArchType */ unsigned getArch() const
              { return ObjFile->getArch(); }

  virtual StringRef getData() const { return ObjFile->getData(); }

  virtual object::ObjectFile* getObjectFile() const { return ObjFile; }

  // Subclasses can override these methods to update the image with loaded
  // addresses for sections and common symbols
  virtual void updateSectionAddress(const object::SectionRef &Sec,
                                    uint64_t Addr) {}
  virtual void updateSymbolAddress(const object::SymbolRef &Sym, uint64_t Addr)
              {}

  // Subclasses can override these methods to provide JIT debugging support
  virtual void registerWithDebugger() {}
  virtual void deregisterWithDebugger() {}
};

} // end namespace llvm

#endif // LLVM_RUNTIMEDYLD_OBJECT_IMAGE_H
