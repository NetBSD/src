//===-- llvm/CodeGen/DwarfFile.h - Dwarf Debug Framework -------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef CODEGEN_ASMPRINTER_DWARFFILE_H__
#define CODEGEN_ASMPRINTER_DWARFFILE_H__

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/Allocator.h"
#include "AddressPool.h"
#include "DwarfStringPool.h"

#include <vector>
#include <string>
#include <memory>

namespace llvm {
class AsmPrinter;
class DwarfUnit;
class DIEAbbrev;
class MCSymbol;
class DIE;
class StringRef;
class DwarfDebug;
class MCSection;
class DwarfFile {
  // Target of Dwarf emission, used for sizing of abbreviations.
  AsmPrinter *Asm;

  // Used to uniquely define abbreviations.
  FoldingSet<DIEAbbrev> AbbreviationsSet;

  // A list of all the unique abbreviations in use.
  std::vector<DIEAbbrev *> Abbreviations;

  // A pointer to all units in the section.
  SmallVector<std::unique_ptr<DwarfUnit>, 1> CUs;

  DwarfStringPool StrPool;

public:
  DwarfFile(AsmPrinter *AP, StringRef Pref, BumpPtrAllocator &DA);

  ~DwarfFile();

  const SmallVectorImpl<std::unique_ptr<DwarfUnit>> &getUnits() { return CUs; }

  /// \brief Compute the size and offset of a DIE given an incoming Offset.
  unsigned computeSizeAndOffset(DIE &Die, unsigned Offset);

  /// \brief Compute the size and offset of all the DIEs.
  void computeSizeAndOffsets();

  /// \brief Define a unique number for the abbreviation.
  void assignAbbrevNumber(DIEAbbrev &Abbrev);

  /// \brief Add a unit to the list of CUs.
  void addUnit(std::unique_ptr<DwarfUnit> U);

  /// \brief Emit all of the units to the section listed with the given
  /// abbreviation section.
  void emitUnits(DwarfDebug *DD, const MCSymbol *ASectionSym);

  /// \brief Emit a set of abbreviations to the specific section.
  void emitAbbrevs(const MCSection *);

  /// \brief Emit all of the strings to the section given.
  void emitStrings(const MCSection *StrSection,
                   const MCSection *OffsetSection = nullptr,
                   const MCSymbol *StrSecSym = nullptr);

  /// \brief Returns the string pool.
  DwarfStringPool &getStringPool() { return StrPool; }
};
}
#endif
