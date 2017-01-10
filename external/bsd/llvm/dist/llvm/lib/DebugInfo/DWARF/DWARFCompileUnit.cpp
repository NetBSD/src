//===-- DWARFCompileUnit.cpp ----------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/DWARF/DWARFCompileUnit.h"
#include "llvm/DebugInfo/DWARF/DWARFDebugAbbrev.h"
#include "llvm/DebugInfo/DWARF/DWARFDie.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void DWARFCompileUnit::dump(raw_ostream &OS) {
  OS << format("0x%08x", getOffset()) << ": Compile Unit:"
     << " length = " << format("0x%08x", getLength())
     << " version = " << format("0x%04x", getVersion())
     << " abbr_offset = " << format("0x%04x", getAbbreviations()->getOffset())
     << " addr_size = " << format("0x%02x", getAddressByteSize())
     << " (next unit at " << format("0x%08x", getNextUnitOffset())
     << ")\n";

  if (DWARFDie CUDie = getUnitDIE(false))
    CUDie.dump(OS, -1U);
  else
    OS << "<compile unit can't be parsed!>\n\n";
}

// VTable anchor.
DWARFCompileUnit::~DWARFCompileUnit() = default;
