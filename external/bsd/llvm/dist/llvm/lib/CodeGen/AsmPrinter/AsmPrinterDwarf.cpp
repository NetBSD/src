//===-- AsmPrinterDwarf.cpp - AsmPrinter Dwarf Support --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Dwarf emissions parts of AsmPrinter.
//
//===----------------------------------------------------------------------===//

#include "ByteStreamer.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtargetInfo.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

//===----------------------------------------------------------------------===//
// Dwarf Emission Helper Routines
//===----------------------------------------------------------------------===//

/// EmitSLEB128 - emit the specified signed leb128 value.
void AsmPrinter::EmitSLEB128(int64_t Value, const char *Desc) const {
  if (isVerbose() && Desc)
    OutStreamer.AddComment(Desc);

  OutStreamer.EmitSLEB128IntValue(Value);
}

/// EmitULEB128 - emit the specified signed leb128 value.
void AsmPrinter::EmitULEB128(uint64_t Value, const char *Desc,
                             unsigned PadTo) const {
  if (isVerbose() && Desc)
    OutStreamer.AddComment(Desc);

  OutStreamer.EmitULEB128IntValue(Value, PadTo);
}

/// EmitCFAByte - Emit a .byte 42 directive for a DW_CFA_xxx value.
void AsmPrinter::EmitCFAByte(unsigned Val) const {
  if (isVerbose()) {
    if (Val >= dwarf::DW_CFA_offset && Val < dwarf::DW_CFA_offset + 64)
      OutStreamer.AddComment("DW_CFA_offset + Reg (" +
                             Twine(Val - dwarf::DW_CFA_offset) + ")");
    else
      OutStreamer.AddComment(dwarf::CallFrameString(Val));
  }
  OutStreamer.EmitIntValue(Val, 1);
}

static const char *DecodeDWARFEncoding(unsigned Encoding) {
  switch (Encoding) {
  case dwarf::DW_EH_PE_absptr:
    return "absptr";
  case dwarf::DW_EH_PE_omit:
    return "omit";
  case dwarf::DW_EH_PE_pcrel:
    return "pcrel";
  case dwarf::DW_EH_PE_udata4:
    return "udata4";
  case dwarf::DW_EH_PE_udata8:
    return "udata8";
  case dwarf::DW_EH_PE_sdata4:
    return "sdata4";
  case dwarf::DW_EH_PE_sdata8:
    return "sdata8";
  case dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_udata4:
    return "pcrel udata4";
  case dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata4:
    return "pcrel sdata4";
  case dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_udata8:
    return "pcrel udata8";
  case dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata8:
    return "pcrel sdata8";
  case dwarf::DW_EH_PE_indirect | dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_udata4
      :
    return "indirect pcrel udata4";
  case dwarf::DW_EH_PE_indirect | dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata4
      :
    return "indirect pcrel sdata4";
  case dwarf::DW_EH_PE_indirect | dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_udata8
      :
    return "indirect pcrel udata8";
  case dwarf::DW_EH_PE_indirect | dwarf::DW_EH_PE_pcrel | dwarf::DW_EH_PE_sdata8
      :
    return "indirect pcrel sdata8";
  }

  return "<unknown encoding>";
}

/// EmitEncodingByte - Emit a .byte 42 directive that corresponds to an
/// encoding.  If verbose assembly output is enabled, we output comments
/// describing the encoding.  Desc is an optional string saying what the
/// encoding is specifying (e.g. "LSDA").
void AsmPrinter::EmitEncodingByte(unsigned Val, const char *Desc) const {
  if (isVerbose()) {
    if (Desc)
      OutStreamer.AddComment(Twine(Desc) + " Encoding = " +
                             Twine(DecodeDWARFEncoding(Val)));
    else
      OutStreamer.AddComment(Twine("Encoding = ") + DecodeDWARFEncoding(Val));
  }

  OutStreamer.EmitIntValue(Val, 1);
}

/// GetSizeOfEncodedValue - Return the size of the encoding in bytes.
unsigned AsmPrinter::GetSizeOfEncodedValue(unsigned Encoding) const {
  if (Encoding == dwarf::DW_EH_PE_omit)
    return 0;

  switch (Encoding & 0x07) {
  default:
    llvm_unreachable("Invalid encoded value.");
  case dwarf::DW_EH_PE_absptr:
    return TM.getSubtargetImpl()->getDataLayout()->getPointerSize();
  case dwarf::DW_EH_PE_udata2:
    return 2;
  case dwarf::DW_EH_PE_udata4:
    return 4;
  case dwarf::DW_EH_PE_udata8:
    return 8;
  }
}

void AsmPrinter::EmitTTypeReference(const GlobalValue *GV,
                                    unsigned Encoding) const {
  if (GV) {
    const TargetLoweringObjectFile &TLOF = getObjFileLowering();

    const MCExpr *Exp =
        TLOF.getTTypeGlobalReference(GV, Encoding, *Mang, TM, MMI, OutStreamer);
    OutStreamer.EmitValue(Exp, GetSizeOfEncodedValue(Encoding));
  } else
    OutStreamer.EmitIntValue(0, GetSizeOfEncodedValue(Encoding));
}

/// EmitSectionOffset - Emit the 4-byte offset of Label from the start of its
/// section.  This can be done with a special directive if the target supports
/// it (e.g. cygwin) or by emitting it as an offset from a label at the start
/// of the section.
///
/// SectionLabel is a temporary label emitted at the start of the section that
/// Label lives in.
void AsmPrinter::EmitSectionOffset(const MCSymbol *Label,
                                   const MCSymbol *SectionLabel) const {
  // On COFF targets, we have to emit the special .secrel32 directive.
  if (MAI->needsDwarfSectionOffsetDirective()) {
    OutStreamer.EmitCOFFSecRel32(Label);
    return;
  }

  // Get the section that we're referring to, based on SectionLabel.
  const MCSection &Section = SectionLabel->getSection();

  // If Label has already been emitted, verify that it is in the same section as
  // section label for sanity.
  assert((!Label->isInSection() || &Label->getSection() == &Section) &&
         "Section offset using wrong section base for label");

  // If the section in question will end up with an address of 0 anyway, we can
  // just emit an absolute reference to save a relocation.
  if (Section.isBaseAddressKnownZero()) {
    OutStreamer.EmitSymbolValue(Label, 4);
    return;
  }

  // Otherwise, emit it as a label difference from the start of the section.
  EmitLabelDifference(Label, SectionLabel, 4);
}

/// Emit a dwarf register operation.
static void emitDwarfRegOp(ByteStreamer &Streamer, int Reg) {
  assert(Reg >= 0);
  if (Reg < 32) {
    Streamer.EmitInt8(dwarf::DW_OP_reg0 + Reg,
                      dwarf::OperationEncodingString(dwarf::DW_OP_reg0 + Reg));
  } else {
    Streamer.EmitInt8(dwarf::DW_OP_regx, "DW_OP_regx");
    Streamer.EmitULEB128(Reg, Twine(Reg));
  }
}

/// Emit an (double-)indirect dwarf register operation.
static void emitDwarfRegOpIndirect(ByteStreamer &Streamer, int Reg, int Offset,
                                   bool Deref) {
  assert(Reg >= 0);
  if (Reg < 32) {
    Streamer.EmitInt8(dwarf::DW_OP_breg0 + Reg,
                      dwarf::OperationEncodingString(dwarf::DW_OP_breg0 + Reg));
  } else {
    Streamer.EmitInt8(dwarf::DW_OP_bregx, "DW_OP_bregx");
    Streamer.EmitULEB128(Reg, Twine(Reg));
  }
  Streamer.EmitSLEB128(Offset);
  if (Deref)
    Streamer.EmitInt8(dwarf::DW_OP_deref, "DW_OP_deref");
}

void AsmPrinter::EmitDwarfOpPiece(ByteStreamer &Streamer, unsigned SizeInBits,
                                  unsigned OffsetInBits) const {
  assert(SizeInBits > 0 && "piece has size zero");
  const unsigned SizeOfByte = 8;
  if (OffsetInBits > 0 || SizeInBits % SizeOfByte) {
    Streamer.EmitInt8(dwarf::DW_OP_bit_piece, "DW_OP_bit_piece");
    Streamer.EmitULEB128(SizeInBits, Twine(SizeInBits));
    Streamer.EmitULEB128(OffsetInBits, Twine(OffsetInBits));
  } else {
    Streamer.EmitInt8(dwarf::DW_OP_piece, "DW_OP_piece");
    unsigned ByteSize = SizeInBits / SizeOfByte;
    Streamer.EmitULEB128(ByteSize, Twine(ByteSize));
  }
}

/// Emit a shift-right dwarf expression.
static void emitDwarfOpShr(ByteStreamer &Streamer,
                           unsigned ShiftBy) {
  Streamer.EmitInt8(dwarf::DW_OP_constu, "DW_OP_constu");
  Streamer.EmitULEB128(ShiftBy);
  Streamer.EmitInt8(dwarf::DW_OP_shr, "DW_OP_shr");
}

// Some targets do not provide a DWARF register number for every
// register.  This function attempts to emit a DWARF register by
// emitting a piece of a super-register or by piecing together
// multiple subregisters that alias the register.
void AsmPrinter::EmitDwarfRegOpPiece(ByteStreamer &Streamer,
                                     const MachineLocation &MLoc,
                                     unsigned PieceSizeInBits,
                                     unsigned PieceOffsetInBits) const {
  assert(MLoc.isReg() && "MLoc must be a register");
  const TargetRegisterInfo *TRI = TM.getSubtargetImpl()->getRegisterInfo();
  int Reg = TRI->getDwarfRegNum(MLoc.getReg(), false);

  // If this is a valid register number, emit it.
  if (Reg >= 0) {
    emitDwarfRegOp(Streamer, Reg);
    EmitDwarfOpPiece(Streamer, PieceSizeInBits, PieceOffsetInBits);
    return;
  }

  // Walk up the super-register chain until we find a valid number.
  // For example, EAX on x86_64 is a 32-bit piece of RAX with offset 0.
  for (MCSuperRegIterator SR(MLoc.getReg(), TRI); SR.isValid(); ++SR) {
    Reg = TRI->getDwarfRegNum(*SR, false);
    if (Reg >= 0) {
      unsigned Idx = TRI->getSubRegIndex(*SR, MLoc.getReg());
      unsigned Size = TRI->getSubRegIdxSize(Idx);
      unsigned RegOffset = TRI->getSubRegIdxOffset(Idx);
      OutStreamer.AddComment("super-register");
      emitDwarfRegOp(Streamer, Reg);
      if (PieceOffsetInBits == RegOffset) {
        EmitDwarfOpPiece(Streamer, Size, RegOffset);
      } else {
        // If this is part of a variable in a sub-register at a
        // non-zero offset, we need to manually shift the value into
        // place, since the DW_OP_piece describes the part of the
        // variable, not the position of the subregister.
        if (RegOffset)
          emitDwarfOpShr(Streamer, RegOffset);
        EmitDwarfOpPiece(Streamer, Size, PieceOffsetInBits);
      }
      return;
    }
  }

  // Otherwise, attempt to find a covering set of sub-register numbers.
  // For example, Q0 on ARM is a composition of D0+D1.
  //
  // Keep track of the current position so we can emit the more
  // efficient DW_OP_piece.
  unsigned CurPos = PieceOffsetInBits;
  // The size of the register in bits, assuming 8 bits per byte.
  unsigned RegSize = TRI->getMinimalPhysRegClass(MLoc.getReg())->getSize() * 8;
  // Keep track of the bits in the register we already emitted, so we
  // can avoid emitting redundant aliasing subregs.
  SmallBitVector Coverage(RegSize, false);
  for (MCSubRegIterator SR(MLoc.getReg(), TRI); SR.isValid(); ++SR) {
    unsigned Idx = TRI->getSubRegIndex(MLoc.getReg(), *SR);
    unsigned Size = TRI->getSubRegIdxSize(Idx);
    unsigned Offset = TRI->getSubRegIdxOffset(Idx);
    Reg = TRI->getDwarfRegNum(*SR, false);

    // Intersection between the bits we already emitted and the bits
    // covered by this subregister.
    SmallBitVector Intersection(RegSize, false);
    Intersection.set(Offset, Offset + Size);
    Intersection ^= Coverage;

    // If this sub-register has a DWARF number and we haven't covered
    // its range, emit a DWARF piece for it.
    if (Reg >= 0 && Intersection.any()) {
      OutStreamer.AddComment("sub-register");
      emitDwarfRegOp(Streamer, Reg);
      EmitDwarfOpPiece(Streamer, Size, Offset == CurPos ? 0 : Offset);
      CurPos = Offset + Size;

      // Mark it as emitted.
      Coverage.set(Offset, Offset + Size);
    }
  }

  if (CurPos == PieceOffsetInBits) {
    // FIXME: We have no reasonable way of handling errors in here.
    Streamer.EmitInt8(dwarf::DW_OP_nop,
                      "nop (could not find a dwarf register number)");
  }
}

/// EmitDwarfRegOp - Emit dwarf register operation.
void AsmPrinter::EmitDwarfRegOp(ByteStreamer &Streamer,
                                const MachineLocation &MLoc,
                                bool Indirect) const {
  const TargetRegisterInfo *TRI = TM.getSubtargetImpl()->getRegisterInfo();
  int Reg = TRI->getDwarfRegNum(MLoc.getReg(), false);
  if (Reg < 0) {
    // We assume that pointers are always in an addressable register.
    if (Indirect || MLoc.isIndirect()) {
      // FIXME: We have no reasonable way of handling errors in here. The
      // caller might be in the middle of a dwarf expression. We should
      // probably assert that Reg >= 0 once debug info generation is more
      // mature.
      Streamer.EmitInt8(dwarf::DW_OP_nop,
                        "nop (invalid dwarf register number for indirect loc)");
      return;
    }

    // Attempt to find a valid super- or sub-register.
    return EmitDwarfRegOpPiece(Streamer, MLoc);
  }

  if (MLoc.isIndirect())
    emitDwarfRegOpIndirect(Streamer, Reg, MLoc.getOffset(), Indirect);
  else if (Indirect)
    emitDwarfRegOpIndirect(Streamer, Reg, 0, false);
  else
    emitDwarfRegOp(Streamer, Reg);
}

//===----------------------------------------------------------------------===//
// Dwarf Lowering Routines
//===----------------------------------------------------------------------===//

void AsmPrinter::emitCFIInstruction(const MCCFIInstruction &Inst) const {
  switch (Inst.getOperation()) {
  default:
    llvm_unreachable("Unexpected instruction");
  case MCCFIInstruction::OpDefCfaOffset:
    OutStreamer.EmitCFIDefCfaOffset(Inst.getOffset());
    break;
  case MCCFIInstruction::OpDefCfa:
    OutStreamer.EmitCFIDefCfa(Inst.getRegister(), Inst.getOffset());
    break;
  case MCCFIInstruction::OpDefCfaRegister:
    OutStreamer.EmitCFIDefCfaRegister(Inst.getRegister());
    break;
  case MCCFIInstruction::OpOffset:
    OutStreamer.EmitCFIOffset(Inst.getRegister(), Inst.getOffset());
    break;
  case MCCFIInstruction::OpRegister:
    OutStreamer.EmitCFIRegister(Inst.getRegister(), Inst.getRegister2());
    break;
  case MCCFIInstruction::OpWindowSave:
    OutStreamer.EmitCFIWindowSave();
    break;
  case MCCFIInstruction::OpSameValue:
    OutStreamer.EmitCFISameValue(Inst.getRegister());
    break;
  }
}
