//===-- llvm/Target/ARMTargetObjectFile.cpp - ARM Object Info Impl --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ARMTargetObjectFile.h"
#include "ARMTargetMachine.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/IR/Mangler.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCSectionELF.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/ELF.h"
#include "llvm/Target/TargetLowering.h"
using namespace llvm;
using namespace dwarf;

//===----------------------------------------------------------------------===//
//                               ELF Target
//===----------------------------------------------------------------------===//

void ARMElfTargetObjectFile::Initialize(MCContext &Ctx,
                                        const TargetMachine &TM) {
  const ARMTargetMachine &ARM_TM = static_cast<const ARMTargetMachine &>(TM);
  bool isAAPCS_ABI = ARM_TM.TargetABI == ARMTargetMachine::ARMABI::ARM_ABI_AAPCS;
  genExecuteOnly = ARM_TM.getSubtargetImpl()->genExecuteOnly();

  TargetLoweringObjectFileELF::Initialize(Ctx, TM);
  InitializeELF(isAAPCS_ABI);

  if (isAAPCS_ABI) {
    LSDASection = nullptr;
  }

  AttributesSection =
      getContext().getELFSection(".ARM.attributes", ELF::SHT_ARM_ATTRIBUTES, 0);

  // Make code section unreadable when in execute-only mode
  if (genExecuteOnly) {
    unsigned  Type = ELF::SHT_PROGBITS;
    unsigned Flags = ELF::SHF_EXECINSTR | ELF::SHF_ALLOC | ELF::SHF_ARM_PURECODE;
    // Since we cannot modify flags for an existing section, we create a new
    // section with the right flags, and use 0 as the unique ID for
    // execute-only text
    TextSection = Ctx.getELFSection(".text", Type, Flags, 0, "", 0U);
  }
}

const MCExpr *ARMElfTargetObjectFile::getTTypeGlobalReference(
    const GlobalValue *GV, unsigned Encoding, const TargetMachine &TM,
    MachineModuleInfo *MMI, MCStreamer &Streamer) const {
  if (TM.getMCAsmInfo()->getExceptionHandlingType() != ExceptionHandling::ARM)
    return TargetLoweringObjectFileELF::getTTypeGlobalReference(
        GV, Encoding, TM, MMI, Streamer);

  assert(Encoding == DW_EH_PE_absptr && "Can handle absptr encoding only");

  return MCSymbolRefExpr::create(TM.getSymbol(GV),
                                 MCSymbolRefExpr::VK_ARM_TARGET2, getContext());
}

const MCExpr *ARMElfTargetObjectFile::
getDebugThreadLocalSymbol(const MCSymbol *Sym) const {
  return MCSymbolRefExpr::create(Sym, MCSymbolRefExpr::VK_ARM_TLSLDO,
                                 getContext());
}

MCSection *
ARMElfTargetObjectFile::getExplicitSectionGlobal(const GlobalObject *GO,
                                                 SectionKind SK, const TargetMachine &TM) const {
  // Set execute-only access for the explicit section
  if (genExecuteOnly && SK.isText())
    SK = SectionKind::getExecuteOnly();

  return TargetLoweringObjectFileELF::getExplicitSectionGlobal(GO, SK, TM);
}

MCSection *
ARMElfTargetObjectFile::SelectSectionForGlobal(const GlobalObject *GO,
                                               SectionKind SK, const TargetMachine &TM) const {
  // Place the global in the execute-only text section
  if (genExecuteOnly && SK.isText())
    SK = SectionKind::getExecuteOnly();

  return TargetLoweringObjectFileELF::SelectSectionForGlobal(GO, SK, TM);
}
