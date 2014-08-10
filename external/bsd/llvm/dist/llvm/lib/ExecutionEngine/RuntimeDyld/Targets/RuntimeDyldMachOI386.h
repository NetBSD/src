//===---- RuntimeDyldMachOI386.h ---- MachO/I386 specific code. ---*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_RUNTIMEDYLDMACHOI386_H
#define LLVM_RUNTIMEDYLDMACHOI386_H

#include "../RuntimeDyldMachO.h"

#define DEBUG_TYPE "dyld"

namespace llvm {

class RuntimeDyldMachOI386
    : public RuntimeDyldMachOCRTPBase<RuntimeDyldMachOI386> {
public:
  RuntimeDyldMachOI386(RTDyldMemoryManager *MM)
      : RuntimeDyldMachOCRTPBase(MM) {}

  unsigned getMaxStubSize() override { return 0; }

  unsigned getStubAlignment() override { return 1; }

  relocation_iterator
  processRelocationRef(unsigned SectionID, relocation_iterator RelI,
                       ObjectImage &ObjImg, ObjSectionToIDMap &ObjSectionToID,
                       const SymbolTableMap &Symbols, StubMap &Stubs) override {
    const MachOObjectFile &Obj =
        static_cast<const MachOObjectFile &>(*ObjImg.getObjectFile());
    MachO::any_relocation_info RelInfo =
        Obj.getRelocation(RelI->getRawDataRefImpl());
    uint32_t RelType = Obj.getAnyRelocationType(RelInfo);

    if (Obj.isRelocationScattered(RelInfo)) {
      if (RelType == MachO::GENERIC_RELOC_SECTDIFF ||
          RelType == MachO::GENERIC_RELOC_LOCAL_SECTDIFF)
        return processSECTDIFFRelocation(SectionID, RelI, ObjImg,
                                         ObjSectionToID);
      else if (Arch == Triple::x86 && RelType == MachO::GENERIC_RELOC_VANILLA)
        return processI386ScatteredVANILLA(SectionID, RelI, ObjImg,
                                           ObjSectionToID);
      llvm_unreachable("Unhandled scattered relocation.");
    }

    RelocationEntry RE(getRelocationEntry(SectionID, ObjImg, RelI));
    RE.Addend = memcpyAddend(RE);
    RelocationValueRef Value(
        getRelocationValueRef(ObjImg, RelI, RE, ObjSectionToID, Symbols));

    // Addends for external, PC-rel relocations on i386 point back to the zero
    // offset. Calculate the final offset from the relocation target instead.
    // This allows us to use the same logic for both external and internal
    // relocations in resolveI386RelocationRef.
    // bool IsExtern = Obj.getPlainRelocationExternal(RelInfo);
    // if (IsExtern && RE.IsPCRel) {
    //   uint64_t RelocAddr = 0;
    //   RelI->getAddress(RelocAddr);
    //   Value.Addend += RelocAddr + 4;
    // }
    if (RE.IsPCRel)
      makeValueAddendPCRel(Value, ObjImg, RelI, 1 << RE.Size);

    RE.Addend = Value.Addend;

    if (Value.SymbolName)
      addRelocationForSymbol(RE, Value.SymbolName);
    else
      addRelocationForSection(RE, Value.SectionID);

    return ++RelI;
  }

  void resolveRelocation(const RelocationEntry &RE, uint64_t Value) {
    DEBUG(dumpRelocationToResolve(RE, Value));

    const SectionEntry &Section = Sections[RE.SectionID];
    uint8_t *LocalAddress = Section.Address + RE.Offset;

    if (RE.IsPCRel) {
      uint64_t FinalAddress = Section.LoadAddress + RE.Offset;
      Value -= FinalAddress + 4; // see MachOX86_64::resolveRelocation.
    }

    switch (RE.RelType) {
    default:
      llvm_unreachable("Invalid relocation type!");
    case MachO::GENERIC_RELOC_VANILLA:
      writeBytesUnaligned(LocalAddress, Value + RE.Addend, 1 << RE.Size);
      break;
    case MachO::GENERIC_RELOC_SECTDIFF:
    case MachO::GENERIC_RELOC_LOCAL_SECTDIFF: {
      uint64_t SectionABase = Sections[RE.Sections.SectionA].LoadAddress;
      uint64_t SectionBBase = Sections[RE.Sections.SectionB].LoadAddress;
      assert((Value == SectionABase || Value == SectionBBase) &&
             "Unexpected SECTDIFF relocation value.");
      Value = SectionABase - SectionBBase + RE.Addend;
      writeBytesUnaligned(LocalAddress, Value, 1 << RE.Size);
      break;
    }
    case MachO::GENERIC_RELOC_PB_LA_PTR:
      Error("Relocation type not implemented yet!");
    }
  }

  void finalizeSection(ObjectImage &ObjImg, unsigned SectionID,
                       const SectionRef &Section) {
    StringRef Name;
    Section.getName(Name);

    if (Name == "__jump_table")
      populateJumpTable(cast<MachOObjectFile>(*ObjImg.getObjectFile()), Section,
                        SectionID);
    else if (Name == "__pointers")
      populatePointersSection(cast<MachOObjectFile>(*ObjImg.getObjectFile()),
                              Section, SectionID);
  }

private:
  relocation_iterator
  processSECTDIFFRelocation(unsigned SectionID, relocation_iterator RelI,
                            ObjectImage &Obj,
                            ObjSectionToIDMap &ObjSectionToID) {
    const MachOObjectFile *MachO =
        static_cast<const MachOObjectFile *>(Obj.getObjectFile());
    MachO::any_relocation_info RE =
        MachO->getRelocation(RelI->getRawDataRefImpl());

    SectionEntry &Section = Sections[SectionID];
    uint32_t RelocType = MachO->getAnyRelocationType(RE);
    bool IsPCRel = MachO->getAnyRelocationPCRel(RE);
    unsigned Size = MachO->getAnyRelocationLength(RE);
    uint64_t Offset;
    RelI->getOffset(Offset);
    uint8_t *LocalAddress = Section.Address + Offset;
    unsigned NumBytes = 1 << Size;
    int64_t Addend = 0;
    memcpy(&Addend, LocalAddress, NumBytes);

    ++RelI;
    MachO::any_relocation_info RE2 =
        MachO->getRelocation(RelI->getRawDataRefImpl());

    uint32_t AddrA = MachO->getScatteredRelocationValue(RE);
    section_iterator SAI = getSectionByAddress(*MachO, AddrA);
    assert(SAI != MachO->section_end() && "Can't find section for address A");
    uint64_t SectionABase;
    SAI->getAddress(SectionABase);
    uint64_t SectionAOffset = AddrA - SectionABase;
    SectionRef SectionA = *SAI;
    bool IsCode;
    SectionA.isText(IsCode);
    uint32_t SectionAID =
        findOrEmitSection(Obj, SectionA, IsCode, ObjSectionToID);

    uint32_t AddrB = MachO->getScatteredRelocationValue(RE2);
    section_iterator SBI = getSectionByAddress(*MachO, AddrB);
    assert(SBI != MachO->section_end() && "Can't find section for address B");
    uint64_t SectionBBase;
    SBI->getAddress(SectionBBase);
    uint64_t SectionBOffset = AddrB - SectionBBase;
    SectionRef SectionB = *SBI;
    uint32_t SectionBID =
        findOrEmitSection(Obj, SectionB, IsCode, ObjSectionToID);

    if (Addend != AddrA - AddrB)
      Error("Unexpected SECTDIFF relocation addend.");

    DEBUG(dbgs() << "Found SECTDIFF: AddrA: " << AddrA << ", AddrB: " << AddrB
                 << ", Addend: " << Addend << ", SectionA ID: " << SectionAID
                 << ", SectionAOffset: " << SectionAOffset
                 << ", SectionB ID: " << SectionBID
                 << ", SectionBOffset: " << SectionBOffset << "\n");
    RelocationEntry R(SectionID, Offset, RelocType, 0, SectionAID,
                      SectionAOffset, SectionBID, SectionBOffset, IsPCRel,
                      Size);

    addRelocationForSection(R, SectionAID);
    addRelocationForSection(R, SectionBID);

    return ++RelI;
  }

  relocation_iterator processI386ScatteredVANILLA(
      unsigned SectionID, relocation_iterator RelI, ObjectImage &Obj,
      RuntimeDyldMachO::ObjSectionToIDMap &ObjSectionToID) {
    const MachOObjectFile *MachO =
        static_cast<const MachOObjectFile *>(Obj.getObjectFile());
    MachO::any_relocation_info RE =
        MachO->getRelocation(RelI->getRawDataRefImpl());

    SectionEntry &Section = Sections[SectionID];
    uint32_t RelocType = MachO->getAnyRelocationType(RE);
    bool IsPCRel = MachO->getAnyRelocationPCRel(RE);
    unsigned Size = MachO->getAnyRelocationLength(RE);
    uint64_t Offset;
    RelI->getOffset(Offset);
    uint8_t *LocalAddress = Section.Address + Offset;
    unsigned NumBytes = 1 << Size;
    int64_t Addend = 0;
    memcpy(&Addend, LocalAddress, NumBytes);

    unsigned SymbolBaseAddr = MachO->getScatteredRelocationValue(RE);
    section_iterator TargetSI = getSectionByAddress(*MachO, SymbolBaseAddr);
    assert(TargetSI != MachO->section_end() && "Can't find section for symbol");
    uint64_t SectionBaseAddr;
    TargetSI->getAddress(SectionBaseAddr);
    SectionRef TargetSection = *TargetSI;
    bool IsCode;
    TargetSection.isText(IsCode);
    uint32_t TargetSectionID =
        findOrEmitSection(Obj, TargetSection, IsCode, ObjSectionToID);

    Addend -= SectionBaseAddr;
    RelocationEntry R(SectionID, Offset, RelocType, Addend, IsPCRel, Size);

    addRelocationForSection(R, TargetSectionID);

    return ++RelI;
  }

  // Populate stubs in __jump_table section.
  void populateJumpTable(MachOObjectFile &Obj, const SectionRef &JTSection,
                         unsigned JTSectionID) {
    assert(!Obj.is64Bit() &&
           "__jump_table section not supported in 64-bit MachO.");

    MachO::dysymtab_command DySymTabCmd = Obj.getDysymtabLoadCommand();
    MachO::section Sec32 = Obj.getSection(JTSection.getRawDataRefImpl());
    uint32_t JTSectionSize = Sec32.size;
    unsigned FirstIndirectSymbol = Sec32.reserved1;
    unsigned JTEntrySize = Sec32.reserved2;
    unsigned NumJTEntries = JTSectionSize / JTEntrySize;
    uint8_t *JTSectionAddr = getSectionAddress(JTSectionID);
    unsigned JTEntryOffset = 0;

    assert((JTSectionSize % JTEntrySize) == 0 &&
           "Jump-table section does not contain a whole number of stubs?");

    for (unsigned i = 0; i < NumJTEntries; ++i) {
      unsigned SymbolIndex =
          Obj.getIndirectSymbolTableEntry(DySymTabCmd, FirstIndirectSymbol + i);
      symbol_iterator SI = Obj.getSymbolByIndex(SymbolIndex);
      StringRef IndirectSymbolName;
      SI->getName(IndirectSymbolName);
      uint8_t *JTEntryAddr = JTSectionAddr + JTEntryOffset;
      createStubFunction(JTEntryAddr);
      RelocationEntry RE(JTSectionID, JTEntryOffset + 1,
                         MachO::GENERIC_RELOC_VANILLA, 0, true, 2);
      addRelocationForSymbol(RE, IndirectSymbolName);
      JTEntryOffset += JTEntrySize;
    }
  }

  // Populate __pointers section.
  void populatePointersSection(MachOObjectFile &Obj,
                               const SectionRef &PTSection,
                               unsigned PTSectionID) {
    assert(!Obj.is64Bit() &&
           "__pointers section not supported in 64-bit MachO.");

    MachO::dysymtab_command DySymTabCmd = Obj.getDysymtabLoadCommand();
    MachO::section Sec32 = Obj.getSection(PTSection.getRawDataRefImpl());
    uint32_t PTSectionSize = Sec32.size;
    unsigned FirstIndirectSymbol = Sec32.reserved1;
    const unsigned PTEntrySize = 4;
    unsigned NumPTEntries = PTSectionSize / PTEntrySize;
    unsigned PTEntryOffset = 0;

    assert((PTSectionSize % PTEntrySize) == 0 &&
           "Pointers section does not contain a whole number of stubs?");

    DEBUG(dbgs() << "Populating __pointers, Section ID " << PTSectionID << ", "
                 << NumPTEntries << " entries, " << PTEntrySize
                 << " bytes each:\n");

    for (unsigned i = 0; i < NumPTEntries; ++i) {
      unsigned SymbolIndex =
          Obj.getIndirectSymbolTableEntry(DySymTabCmd, FirstIndirectSymbol + i);
      symbol_iterator SI = Obj.getSymbolByIndex(SymbolIndex);
      StringRef IndirectSymbolName;
      SI->getName(IndirectSymbolName);
      DEBUG(dbgs() << "  " << IndirectSymbolName << ": index " << SymbolIndex
                   << ", PT offset: " << PTEntryOffset << "\n");
      RelocationEntry RE(PTSectionID, PTEntryOffset,
                         MachO::GENERIC_RELOC_VANILLA, 0, false, 2);
      addRelocationForSymbol(RE, IndirectSymbolName);
      PTEntryOffset += PTEntrySize;
    }
  }

  static section_iterator getSectionByAddress(const MachOObjectFile &Obj,
                                              uint64_t Addr) {
    section_iterator SI = Obj.section_begin();
    section_iterator SE = Obj.section_end();

    for (; SI != SE; ++SI) {
      uint64_t SAddr, SSize;
      SI->getAddress(SAddr);
      SI->getSize(SSize);
      if ((Addr >= SAddr) && (Addr < SAddr + SSize))
        return SI;
    }

    return SE;
  }
};
}

#undef DEBUG_TYPE

#endif // LLVM_RUNTIMEDYLDMACHOI386_H
