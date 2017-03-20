//==-- llvm/CodeGen/TargetLoweringObjectFileImpl.h - Object Info -*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements classes used to handle lowerings specific to common
// object file formats.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_TARGETLOWERINGOBJECTFILEIMPL_H
#define LLVM_CODEGEN_TARGETLOWERINGOBJECTFILEIMPL_H

#include "llvm/MC/MCExpr.h"
#include "llvm/MC/SectionKind.h"
#include "llvm/Target/TargetLoweringObjectFile.h"

namespace llvm {
  class MachineModuleInfo;
  class Mangler;
  class MCAsmInfo;
  class MCSection;
  class MCSectionMachO;
  class MCSymbol;
  class MCContext;
  class GlobalValue;
  class TargetMachine;


class TargetLoweringObjectFileELF : public TargetLoweringObjectFile {
  bool UseInitArray;
  mutable unsigned NextUniqueID = 1;  // ID 0 is reserved for execute-only sections

protected:
  MCSymbolRefExpr::VariantKind PLTRelativeVariantKind =
      MCSymbolRefExpr::VK_None;

public:
  TargetLoweringObjectFileELF() : UseInitArray(false) {}

  ~TargetLoweringObjectFileELF() override {}

  void emitPersonalityValue(MCStreamer &Streamer, const DataLayout &TM,
                            const MCSymbol *Sym) const override;

  /// Given a constant with the SectionKind, return a section that it should be
  /// placed in.
  MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                   const Constant *C,
                                   unsigned &Align) const override;

  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;

  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;

  MCSection *getSectionForJumpTable(const Function &F,
                                    const TargetMachine &TM) const override;

  bool shouldPutJumpTableInFunctionSection(bool UsesLabelDifference,
                                           const Function &F) const override;

  /// Return an MCExpr to use for a reference to the specified type info global
  /// variable from exception handling information.
  const MCExpr *getTTypeGlobalReference(const GlobalValue *GV,
                                        unsigned Encoding,
                                        const TargetMachine &TM,
                                        MachineModuleInfo *MMI,
                                        MCStreamer &Streamer) const override;

  // The symbol that gets passed to .cfi_personality.
  MCSymbol *getCFIPersonalitySymbol(const GlobalValue *GV,
                                    const TargetMachine &TM,
                                    MachineModuleInfo *MMI) const override;

  void InitializeELF(bool UseInitArray_);
  MCSection *getStaticCtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;
  MCSection *getStaticDtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;

  const MCExpr *lowerRelativeReference(const GlobalValue *LHS,
                                       const GlobalValue *RHS,
                                       const TargetMachine &TM) const override;
};



class TargetLoweringObjectFileMachO : public TargetLoweringObjectFile {
public:
  ~TargetLoweringObjectFileMachO() override {}
  TargetLoweringObjectFileMachO();

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;

  /// Emit the module flags that specify the garbage collection information.
  void emitModuleFlags(MCStreamer &Streamer,
                       ArrayRef<Module::ModuleFlagEntry> ModuleFlags,
                       const TargetMachine &TM) const override;

  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;

  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;

  MCSection *getSectionForConstant(const DataLayout &DL, SectionKind Kind,
                                   const Constant *C,
                                   unsigned &Align) const override;

  /// The mach-o version of this method defaults to returning a stub reference.
  const MCExpr *getTTypeGlobalReference(const GlobalValue *GV,
                                        unsigned Encoding,
                                        const TargetMachine &TM,
                                        MachineModuleInfo *MMI,
                                        MCStreamer &Streamer) const override;

  // The symbol that gets passed to .cfi_personality.
  MCSymbol *getCFIPersonalitySymbol(const GlobalValue *GV,
                                    const TargetMachine &TM,
                                    MachineModuleInfo *MMI) const override;

  /// Get MachO PC relative GOT entry relocation
  const MCExpr *getIndirectSymViaGOTPCRel(const MCSymbol *Sym,
                                          const MCValue &MV, int64_t Offset,
                                          MachineModuleInfo *MMI,
                                          MCStreamer &Streamer) const override;

  void getNameWithPrefix(SmallVectorImpl<char> &OutName, const GlobalValue *GV,
                         const TargetMachine &TM) const override;
};



class TargetLoweringObjectFileCOFF : public TargetLoweringObjectFile {
  mutable unsigned NextUniqueID = 0;

public:
  ~TargetLoweringObjectFileCOFF() override {}

  void Initialize(MCContext &Ctx, const TargetMachine &TM) override;
  MCSection *getExplicitSectionGlobal(const GlobalObject *GO, SectionKind Kind,
                                      const TargetMachine &TM) const override;

  MCSection *SelectSectionForGlobal(const GlobalObject *GO, SectionKind Kind,
                                    const TargetMachine &TM) const override;

  void getNameWithPrefix(SmallVectorImpl<char> &OutName, const GlobalValue *GV,
                         const TargetMachine &TM) const override;

  MCSection *getSectionForJumpTable(const Function &F,
                                    const TargetMachine &TM) const override;

  /// Emit Obj-C garbage collection and linker options. Only linker option
  /// emission is implemented for COFF.
  void emitModuleFlags(MCStreamer &Streamer,
                       ArrayRef<Module::ModuleFlagEntry> ModuleFlags,
                       const TargetMachine &TM) const override;

  MCSection *getStaticCtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;
  MCSection *getStaticDtorSection(unsigned Priority,
                                  const MCSymbol *KeySym) const override;

  void emitLinkerFlagsForGlobal(raw_ostream &OS,
                                const GlobalValue *GV) const override;
};

} // end namespace llvm

#endif
