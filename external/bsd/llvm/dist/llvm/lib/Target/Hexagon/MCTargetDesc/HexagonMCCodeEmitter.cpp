//===-- HexagonMCCodeEmitter.cpp - Hexagon Target Descriptions ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Hexagon.h"
#include "MCTargetDesc/HexagonBaseInfo.h"
#include "MCTargetDesc/HexagonFixupKinds.h"
#include "MCTargetDesc/HexagonMCCodeEmitter.h"
#include "MCTargetDesc/HexagonMCInstrInfo.h"
#include "MCTargetDesc/HexagonMCTargetDesc.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG_TYPE "mccodeemitter"

using namespace llvm;
using namespace Hexagon;

STATISTIC(MCNumEmitted, "Number of MC instructions emitted");

HexagonMCCodeEmitter::HexagonMCCodeEmitter(MCInstrInfo const &aMII,
                                           MCContext &aMCT)
    : MCT(aMCT), MCII(aMII), Addend(new unsigned(0)),
      Extended(new bool(false)), CurrentBundle(new MCInst const *) {}

uint32_t HexagonMCCodeEmitter::parseBits(size_t Instruction, size_t Last,
                                         MCInst const &MCB,
                                         MCInst const &MCI) const {
  bool Duplex = HexagonMCInstrInfo::isDuplex(MCII, MCI);
  if (Instruction == 0) {
    if (HexagonMCInstrInfo::isInnerLoop(MCB)) {
      assert(!Duplex);
      assert(Instruction != Last);
      return HexagonII::INST_PARSE_LOOP_END;
    }
  }
  if (Instruction == 1) {
    if (HexagonMCInstrInfo::isOuterLoop(MCB)) {
      assert(!Duplex);
      assert(Instruction != Last);
      return HexagonII::INST_PARSE_LOOP_END;
    }
  }
  if (Duplex) {
    assert(Instruction == Last);
    return HexagonII::INST_PARSE_DUPLEX;
  }
  if(Instruction == Last)
    return HexagonII::INST_PARSE_PACKET_END;
  return HexagonII::INST_PARSE_NOT_END;
}

void HexagonMCCodeEmitter::encodeInstruction(MCInst const &MI, raw_ostream &OS,
                                             SmallVectorImpl<MCFixup> &Fixups,
                                             MCSubtargetInfo const &STI) const {
  MCInst &HMB = const_cast<MCInst &>(MI);

  assert(HexagonMCInstrInfo::isBundle(HMB));
  DEBUG(dbgs() << "Encoding bundle\n";);
  *Addend = 0;
  *Extended = false;
  *CurrentBundle = &MI;
  size_t Instruction = 0;
  size_t Last = HexagonMCInstrInfo::bundleSize(HMB) - 1;
  for (auto &I : HexagonMCInstrInfo::bundleInstructions(HMB)) {
    MCInst &HMI = const_cast<MCInst &>(*I.getInst());
    verifyInstructionPredicates(HMI,
                                computeAvailableFeatures(STI.getFeatureBits()));

    EncodeSingleInstruction(HMI, OS, Fixups, STI,
                            parseBits(Instruction, Last, HMB, HMI),
                            Instruction);
    *Extended = HexagonMCInstrInfo::isImmext(HMI);
    *Addend += HEXAGON_INSTR_SIZE;
    ++Instruction;
  }
  return;
}

static bool RegisterMatches(unsigned Consumer, unsigned Producer,
                            unsigned Producer2) {
  if (Consumer == Producer)
    return true;
  if (Consumer == Producer2)
    return true;
  // Calculate if we're a single vector consumer referencing a double producer
  if (Producer >= Hexagon::W0 && Producer <= Hexagon::W15)
    if (Consumer >= Hexagon::V0 && Consumer <= Hexagon::V31)
      return ((Consumer - Hexagon::V0) >> 1) == (Producer - Hexagon::W0);
  return false;
}

/// EncodeSingleInstruction - Emit a single
void HexagonMCCodeEmitter::EncodeSingleInstruction(
    const MCInst &MI, raw_ostream &OS, SmallVectorImpl<MCFixup> &Fixups,
    const MCSubtargetInfo &STI, uint32_t Parse, size_t Index) const {
  MCInst HMB = MI;
  assert(!HexagonMCInstrInfo::isBundle(HMB));
  uint64_t Binary;

  // Compound instructions are limited to using registers 0-7 and 16-23
  // and here we make a map 16-23 to 8-15 so they can be correctly encoded.
  static unsigned RegMap[8] = {Hexagon::R8,  Hexagon::R9,  Hexagon::R10,
                               Hexagon::R11, Hexagon::R12, Hexagon::R13,
                               Hexagon::R14, Hexagon::R15};

  // Pseudo instructions don't get encoded and shouldn't be here
  // in the first place!
  assert(!HexagonMCInstrInfo::getDesc(MCII, HMB).isPseudo() &&
         "pseudo-instruction found");
  DEBUG(dbgs() << "Encoding insn"
                  " `" << HexagonMCInstrInfo::getName(MCII, HMB) << "'"
                                                                    "\n");

  if (llvm::HexagonMCInstrInfo::getType(MCII, HMB) == HexagonII::TypeCOMPOUND) {
    for (unsigned i = 0; i < HMB.getNumOperands(); ++i)
      if (HMB.getOperand(i).isReg()) {
        unsigned Reg =
            MCT.getRegisterInfo()->getEncodingValue(HMB.getOperand(i).getReg());
        if ((Reg <= 23) && (Reg >= 16))
          HMB.getOperand(i).setReg(RegMap[Reg - 16]);
      }
  }

  if (HexagonMCInstrInfo::isNewValue(MCII, HMB)) {
    // Calculate the new value distance to the associated producer
    MCOperand &MCO =
        HMB.getOperand(HexagonMCInstrInfo::getNewValueOp(MCII, HMB));
    unsigned SOffset = 0;
    unsigned VOffset = 0;
    unsigned Register = MCO.getReg();
    unsigned Register1;
    unsigned Register2;
    auto Instructions = HexagonMCInstrInfo::bundleInstructions(**CurrentBundle);
    auto i = Instructions.begin() + Index - 1;
    for (;; --i) {
      assert(i != Instructions.begin() - 1 && "Couldn't find producer");
      MCInst const &Inst = *i->getInst();
      if (HexagonMCInstrInfo::isImmext(Inst))
        continue;
      ++SOffset;
      if (HexagonMCInstrInfo::isVector(MCII, Inst))
        // Vector instructions don't count scalars
        ++VOffset;
      Register1 =
          HexagonMCInstrInfo::hasNewValue(MCII, Inst)
              ? HexagonMCInstrInfo::getNewValueOperand(MCII, Inst).getReg()
              : static_cast<unsigned>(Hexagon::NoRegister);
      Register2 =
          HexagonMCInstrInfo::hasNewValue2(MCII, Inst)
              ? HexagonMCInstrInfo::getNewValueOperand2(MCII, Inst).getReg()
              : static_cast<unsigned>(Hexagon::NoRegister);
      if (!RegisterMatches(Register, Register1, Register2))
        // This isn't the register we're looking for
        continue;
      if (!HexagonMCInstrInfo::isPredicated(MCII, Inst))
        // Producer is unpredicated
        break;
      assert(HexagonMCInstrInfo::isPredicated(MCII, HMB) &&
             "Unpredicated consumer depending on predicated producer");
      if (HexagonMCInstrInfo::isPredicatedTrue(MCII, Inst) ==
          HexagonMCInstrInfo::isPredicatedTrue(MCII, HMB))
        // Producer predicate sense matched ours
        break;
    }
    // Hexagon PRM 10.11 Construct Nt from distance
    unsigned Offset =
        HexagonMCInstrInfo::isVector(MCII, HMB) ? VOffset : SOffset;
    Offset <<= 1;
    Offset |=
        HexagonMCInstrInfo::SubregisterBit(Register, Register1, Register2);
    MCO.setReg(Offset + Hexagon::R0);
  }

  Binary = getBinaryCodeForInstr(HMB, Fixups, STI);
  // Check for unimplemented instructions. Immediate extenders
  // are encoded as zero, so they need to be accounted for.
  if ((!Binary) &&
      ((HMB.getOpcode() != DuplexIClass0) && (HMB.getOpcode() != A4_ext) &&
       (HMB.getOpcode() != A4_ext_b) && (HMB.getOpcode() != A4_ext_c) &&
       (HMB.getOpcode() != A4_ext_g))) {
    DEBUG(dbgs() << "Unimplemented inst: "
                    " `" << HexagonMCInstrInfo::getName(MCII, HMB) << "'"
                                                                      "\n");
    llvm_unreachable("Unimplemented Instruction");
  }
  Binary |= Parse;

  // if we need to emit a duplexed instruction
  if (HMB.getOpcode() >= Hexagon::DuplexIClass0 &&
      HMB.getOpcode() <= Hexagon::DuplexIClassF) {
    assert(Parse == HexagonII::INST_PARSE_DUPLEX &&
           "Emitting duplex without duplex parse bits");
    unsigned dupIClass;
    switch (HMB.getOpcode()) {
    case Hexagon::DuplexIClass0:
      dupIClass = 0;
      break;
    case Hexagon::DuplexIClass1:
      dupIClass = 1;
      break;
    case Hexagon::DuplexIClass2:
      dupIClass = 2;
      break;
    case Hexagon::DuplexIClass3:
      dupIClass = 3;
      break;
    case Hexagon::DuplexIClass4:
      dupIClass = 4;
      break;
    case Hexagon::DuplexIClass5:
      dupIClass = 5;
      break;
    case Hexagon::DuplexIClass6:
      dupIClass = 6;
      break;
    case Hexagon::DuplexIClass7:
      dupIClass = 7;
      break;
    case Hexagon::DuplexIClass8:
      dupIClass = 8;
      break;
    case Hexagon::DuplexIClass9:
      dupIClass = 9;
      break;
    case Hexagon::DuplexIClassA:
      dupIClass = 10;
      break;
    case Hexagon::DuplexIClassB:
      dupIClass = 11;
      break;
    case Hexagon::DuplexIClassC:
      dupIClass = 12;
      break;
    case Hexagon::DuplexIClassD:
      dupIClass = 13;
      break;
    case Hexagon::DuplexIClassE:
      dupIClass = 14;
      break;
    case Hexagon::DuplexIClassF:
      dupIClass = 15;
      break;
    default:
      llvm_unreachable("Unimplemented DuplexIClass");
      break;
    }
    // 29 is the bit position.
    // 0b1110 =0xE bits are masked off and down shifted by 1 bit.
    // Last bit is moved to bit position 13
    Binary = ((dupIClass & 0xE) << (29 - 1)) | ((dupIClass & 0x1) << 13);

    const MCInst *subInst0 = HMB.getOperand(0).getInst();
    const MCInst *subInst1 = HMB.getOperand(1).getInst();

    // get subinstruction slot 0
    unsigned subInstSlot0Bits = getBinaryCodeForInstr(*subInst0, Fixups, STI);
    // get subinstruction slot 1
    unsigned subInstSlot1Bits = getBinaryCodeForInstr(*subInst1, Fixups, STI);

    Binary |= subInstSlot0Bits | (subInstSlot1Bits << 16);
  }
  support::endian::Writer<support::little>(OS).write<uint32_t>(Binary);
  ++MCNumEmitted;
}

namespace {
void raise_relocation_error(unsigned bits, unsigned kind) {
  std::string Text;
  {
    llvm::raw_string_ostream Stream(Text);
    Stream << "Unrecognized relocation combination bits: " << bits
           << " kind: " << kind;
  }
  report_fatal_error(Text);
}
}

/// getFixupNoBits - Some insns are not extended and thus have no
/// bits.  These cases require a more brute force method for determining
/// the correct relocation.
namespace {
Hexagon::Fixups getFixupNoBits(MCInstrInfo const &MCII, const MCInst &MI,
                                      const MCOperand &MO,
                                      const MCSymbolRefExpr::VariantKind kind) {
  const MCInstrDesc &MCID = HexagonMCInstrInfo::getDesc(MCII, MI);
  unsigned insnType = llvm::HexagonMCInstrInfo::getType(MCII, MI);

  if (insnType == HexagonII::TypePREFIX) {
    switch (kind) {
    case MCSymbolRefExpr::VK_GOTREL:
      return Hexagon::fixup_Hexagon_GOTREL_32_6_X;
    case MCSymbolRefExpr::VK_GOT:
      return Hexagon::fixup_Hexagon_GOT_32_6_X;
    case MCSymbolRefExpr::VK_TPREL:
      return Hexagon::fixup_Hexagon_TPREL_32_6_X;
    case MCSymbolRefExpr::VK_DTPREL:
      return Hexagon::fixup_Hexagon_DTPREL_32_6_X;
    case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
      return Hexagon::fixup_Hexagon_GD_GOT_32_6_X;
    case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
      return Hexagon::fixup_Hexagon_LD_GOT_32_6_X;
    case MCSymbolRefExpr::VK_Hexagon_IE:
      return Hexagon::fixup_Hexagon_IE_32_6_X;
    case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
      return Hexagon::fixup_Hexagon_IE_GOT_32_6_X;
    case MCSymbolRefExpr::VK_Hexagon_PCREL:
    case MCSymbolRefExpr::VK_None:
      if (MCID.isBranch())
        return Hexagon::fixup_Hexagon_B32_PCREL_X;
      else
        return Hexagon::fixup_Hexagon_32_6_X;
    default:
      raise_relocation_error(0, kind);
    }
  } else if (MCID.isBranch())
    return Hexagon::fixup_Hexagon_B13_PCREL;

  switch (MCID.getOpcode()) {
  case Hexagon::HI:
  case Hexagon::A2_tfrih:
    switch (kind) {
    case MCSymbolRefExpr::VK_GOT:
      return Hexagon::fixup_Hexagon_GOT_HI16;
    case MCSymbolRefExpr::VK_GOTREL:
      return Hexagon::fixup_Hexagon_GOTREL_HI16;
    case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
      return Hexagon::fixup_Hexagon_GD_GOT_HI16;
    case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
      return Hexagon::fixup_Hexagon_LD_GOT_HI16;
    case MCSymbolRefExpr::VK_Hexagon_IE:
      return Hexagon::fixup_Hexagon_IE_HI16;
    case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
      return Hexagon::fixup_Hexagon_IE_GOT_HI16;
    case MCSymbolRefExpr::VK_TPREL:
      return Hexagon::fixup_Hexagon_TPREL_HI16;
    case MCSymbolRefExpr::VK_DTPREL:
      return Hexagon::fixup_Hexagon_DTPREL_HI16;
    case MCSymbolRefExpr::VK_None:
      return Hexagon::fixup_Hexagon_HI16;
    default:
      raise_relocation_error(0, kind);
    }

  case Hexagon::LO:
  case Hexagon::A2_tfril:
    switch (kind) {
    case MCSymbolRefExpr::VK_GOT:
      return Hexagon::fixup_Hexagon_GOT_LO16;
    case MCSymbolRefExpr::VK_GOTREL:
      return Hexagon::fixup_Hexagon_GOTREL_LO16;
    case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
      return Hexagon::fixup_Hexagon_GD_GOT_LO16;
    case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
      return Hexagon::fixup_Hexagon_LD_GOT_LO16;
    case MCSymbolRefExpr::VK_Hexagon_IE:
      return Hexagon::fixup_Hexagon_IE_LO16;
    case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
      return Hexagon::fixup_Hexagon_IE_GOT_LO16;
    case MCSymbolRefExpr::VK_TPREL:
      return Hexagon::fixup_Hexagon_TPREL_LO16;
    case MCSymbolRefExpr::VK_DTPREL:
      return Hexagon::fixup_Hexagon_DTPREL_LO16;
    case MCSymbolRefExpr::VK_None:
      return Hexagon::fixup_Hexagon_LO16;
    default:
      raise_relocation_error(0, kind);
    }

  // The only relocs left should be GP relative:
  default:
    if (MCID.mayStore() || MCID.mayLoad()) {
      for (const MCPhysReg *ImpUses = MCID.getImplicitUses(); *ImpUses;
           ++ImpUses) {
        if (*ImpUses != Hexagon::GP)
          continue;
        switch (HexagonMCInstrInfo::getAccessSize(MCII, MI)) {
        case HexagonII::MemAccessSize::ByteAccess:
          return fixup_Hexagon_GPREL16_0;
        case HexagonII::MemAccessSize::HalfWordAccess:
          return fixup_Hexagon_GPREL16_1;
        case HexagonII::MemAccessSize::WordAccess:
          return fixup_Hexagon_GPREL16_2;
        case HexagonII::MemAccessSize::DoubleWordAccess:
          return fixup_Hexagon_GPREL16_3;
        default:
          raise_relocation_error(0, kind);
        }
      }
    }
    raise_relocation_error(0, kind);
  }
  llvm_unreachable("Relocation exit not taken");
}
}

namespace llvm {
extern const MCInstrDesc HexagonInsts[];
}

namespace {
  bool isPCRel (unsigned Kind) {
    switch(Kind){
    case fixup_Hexagon_B22_PCREL:
    case fixup_Hexagon_B15_PCREL:
    case fixup_Hexagon_B7_PCREL:
    case fixup_Hexagon_B13_PCREL:
    case fixup_Hexagon_B9_PCREL:
    case fixup_Hexagon_B32_PCREL_X:
    case fixup_Hexagon_B22_PCREL_X:
    case fixup_Hexagon_B15_PCREL_X:
    case fixup_Hexagon_B13_PCREL_X:
    case fixup_Hexagon_B9_PCREL_X:
    case fixup_Hexagon_B7_PCREL_X:
    case fixup_Hexagon_32_PCREL:
    case fixup_Hexagon_PLT_B22_PCREL:
    case fixup_Hexagon_GD_PLT_B22_PCREL:
    case fixup_Hexagon_LD_PLT_B22_PCREL:
    case fixup_Hexagon_6_PCREL_X:
      return true;
    default:
      return false;
    }
  }
}

unsigned HexagonMCCodeEmitter::getExprOpValue(const MCInst &MI,
                                              const MCOperand &MO,
                                              const MCExpr *ME,
                                              SmallVectorImpl<MCFixup> &Fixups,
                                              const MCSubtargetInfo &STI) const

{
  if (isa<HexagonMCExpr>(ME))
    ME = &HexagonMCInstrInfo::getExpr(*ME);
  int64_t Value;
  if (ME->evaluateAsAbsolute(Value))
    return Value;
  assert(ME->getKind() == MCExpr::SymbolRef || ME->getKind() == MCExpr::Binary);
  if (ME->getKind() == MCExpr::Binary) {
    MCBinaryExpr const *Binary = cast<MCBinaryExpr>(ME);
    getExprOpValue(MI, MO, Binary->getLHS(), Fixups, STI);
    getExprOpValue(MI, MO, Binary->getRHS(), Fixups, STI);
    return 0;
  }
  Hexagon::Fixups FixupKind =
      Hexagon::Fixups(Hexagon::fixup_Hexagon_TPREL_LO16);
  const MCSymbolRefExpr *MCSRE = static_cast<const MCSymbolRefExpr *>(ME);
  const MCInstrDesc &MCID = HexagonMCInstrInfo::getDesc(MCII, MI);
  unsigned bits = HexagonMCInstrInfo::getExtentBits(MCII, MI) -
                  HexagonMCInstrInfo::getExtentAlignment(MCII, MI);
  const MCSymbolRefExpr::VariantKind kind = MCSRE->getKind();

  DEBUG(dbgs() << "----------------------------------------\n");
  DEBUG(dbgs() << "Opcode Name: " << HexagonMCInstrInfo::getName(MCII, MI)
               << "\n");
  DEBUG(dbgs() << "Opcode: " << MCID.getOpcode() << "\n");
  DEBUG(dbgs() << "Relocation bits: " << bits << "\n");
  DEBUG(dbgs() << "Addend: " << *Addend << "\n");
  DEBUG(dbgs() << "----------------------------------------\n");

  switch (bits) {
  default:
    raise_relocation_error(bits, kind);
  case 32:
    switch (kind) {
    case MCSymbolRefExpr::VK_DTPREL:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_DTPREL_32_6_X
                            : Hexagon::fixup_Hexagon_DTPREL_32;
      break;
    case MCSymbolRefExpr::VK_GOT:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_GOT_32_6_X
                            : Hexagon::fixup_Hexagon_GOT_32;
      break;
    case MCSymbolRefExpr::VK_GOTREL:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_GOTREL_32_6_X
                            : Hexagon::fixup_Hexagon_GOTREL_32;
      break;
    case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_GD_GOT_32_6_X
                            : Hexagon::fixup_Hexagon_GD_GOT_32;
      break;
    case MCSymbolRefExpr::VK_Hexagon_IE:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_IE_32_6_X
                            : Hexagon::fixup_Hexagon_IE_32;
      break;
    case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_IE_GOT_32_6_X
                            : Hexagon::fixup_Hexagon_IE_GOT_32;
      break;
    case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_LD_GOT_32_6_X
                            : Hexagon::fixup_Hexagon_LD_GOT_32;
      break;
    case MCSymbolRefExpr::VK_Hexagon_PCREL:
      FixupKind = Hexagon::fixup_Hexagon_32_PCREL;
      break;
    case MCSymbolRefExpr::VK_None:
      FixupKind =
          *Extended ? Hexagon::fixup_Hexagon_32_6_X : Hexagon::fixup_Hexagon_32;
      break;
    case MCSymbolRefExpr::VK_TPREL:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_TPREL_32_6_X
                            : Hexagon::fixup_Hexagon_TPREL_32;
      break;
    default:
      raise_relocation_error(bits, kind);
    }
    break;

  case 22:
    switch (kind) {
    case MCSymbolRefExpr::VK_Hexagon_GD_PLT:
      FixupKind = Hexagon::fixup_Hexagon_GD_PLT_B22_PCREL;
      break;
    case MCSymbolRefExpr::VK_Hexagon_LD_PLT:
      FixupKind = Hexagon::fixup_Hexagon_LD_PLT_B22_PCREL;
      break;
    case MCSymbolRefExpr::VK_None:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_B22_PCREL_X
                            : Hexagon::fixup_Hexagon_B22_PCREL;
      break;
    case MCSymbolRefExpr::VK_PLT:
      FixupKind = Hexagon::fixup_Hexagon_PLT_B22_PCREL;
      break;
    default:
      raise_relocation_error(bits, kind);
    }
    break;

  case 16:
    if (*Extended) {
      switch (kind) {
      case MCSymbolRefExpr::VK_DTPREL:
        FixupKind = Hexagon::fixup_Hexagon_DTPREL_16_X;
        break;
      case MCSymbolRefExpr::VK_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GOT_16_X;
        break;
      case MCSymbolRefExpr::VK_GOTREL:
        FixupKind = Hexagon::fixup_Hexagon_GOTREL_16_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GD_GOT_16_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_IE:
        FixupKind = Hexagon::fixup_Hexagon_IE_16_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
        FixupKind = Hexagon::fixup_Hexagon_IE_GOT_16_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_LD_GOT_16_X;
        break;
      case MCSymbolRefExpr::VK_None:
        FixupKind = Hexagon::fixup_Hexagon_16_X;
        break;
      case MCSymbolRefExpr::VK_TPREL:
        FixupKind = Hexagon::fixup_Hexagon_TPREL_16_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    } else
      switch (kind) {
      case MCSymbolRefExpr::VK_None: {
        if (HexagonMCInstrInfo::s23_2_reloc(*MO.getExpr()))
          FixupKind = Hexagon::fixup_Hexagon_23_REG;
        else
          raise_relocation_error(bits, kind);
        break;
      }
      case MCSymbolRefExpr::VK_DTPREL:
        FixupKind = Hexagon::fixup_Hexagon_DTPREL_16;
        break;
      case MCSymbolRefExpr::VK_GOTREL:
        if (MCID.getOpcode() == Hexagon::HI)
          FixupKind = Hexagon::fixup_Hexagon_GOTREL_HI16;
        else
          FixupKind = Hexagon::fixup_Hexagon_GOTREL_LO16;
        break;
      case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GD_GOT_16;
        break;
      case MCSymbolRefExpr::VK_Hexagon_GPREL:
        FixupKind = Hexagon::fixup_Hexagon_GPREL16_0;
        break;
      case MCSymbolRefExpr::VK_Hexagon_HI16:
        FixupKind = Hexagon::fixup_Hexagon_HI16;
        break;
      case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
        FixupKind = Hexagon::fixup_Hexagon_IE_GOT_16;
        break;
      case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_LD_GOT_16;
        break;
      case MCSymbolRefExpr::VK_Hexagon_LO16:
        FixupKind = Hexagon::fixup_Hexagon_LO16;
        break;
      case MCSymbolRefExpr::VK_TPREL:
        FixupKind = Hexagon::fixup_Hexagon_TPREL_16;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    break;

  case 15:
    switch (kind) {
    case MCSymbolRefExpr::VK_None:
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_B15_PCREL_X
                            : Hexagon::fixup_Hexagon_B15_PCREL;
      break;
    default:
      raise_relocation_error(bits, kind);
    }
    break;

  case 13:
    switch (kind) {
    case MCSymbolRefExpr::VK_None:
      FixupKind = Hexagon::fixup_Hexagon_B13_PCREL;
      break;
    default:
      raise_relocation_error(bits, kind);
    }
    break;

  case 12:
    if (*Extended)
      switch (kind) {
      // There isn't a GOT_12_X, both 11_X and 16_X resolve to 6/26
      case MCSymbolRefExpr::VK_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GOT_16_X;
        break;
      case MCSymbolRefExpr::VK_GOTREL:
        FixupKind = Hexagon::fixup_Hexagon_GOTREL_16_X;
        break;
      case MCSymbolRefExpr::VK_None:
        FixupKind = Hexagon::fixup_Hexagon_12_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    else
      raise_relocation_error(bits, kind);
    break;

  case 11:
    if (*Extended)
      switch (kind) {
      case MCSymbolRefExpr::VK_DTPREL:
        FixupKind = Hexagon::fixup_Hexagon_DTPREL_11_X;
        break;
      case MCSymbolRefExpr::VK_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GOT_11_X;
        break;
      case MCSymbolRefExpr::VK_GOTREL:
        FixupKind = Hexagon::fixup_Hexagon_GOTREL_11_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_GD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GD_GOT_11_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_IE_GOT:
        FixupKind = Hexagon::fixup_Hexagon_IE_GOT_11_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_LD_GOT:
        FixupKind = Hexagon::fixup_Hexagon_LD_GOT_11_X;
        break;
      case MCSymbolRefExpr::VK_None:
        FixupKind = Hexagon::fixup_Hexagon_11_X;
        break;
      case MCSymbolRefExpr::VK_TPREL:
        FixupKind = Hexagon::fixup_Hexagon_TPREL_11_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    else {
      switch (kind) {
      case MCSymbolRefExpr::VK_TPREL:
        FixupKind = Hexagon::fixup_Hexagon_TPREL_11_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    }
    break;

  case 10:
    if (*Extended) {
      switch (kind) {
      case MCSymbolRefExpr::VK_None:
        FixupKind = Hexagon::fixup_Hexagon_10_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    } else
      raise_relocation_error(bits, kind);
    break;

  case 9:
    if (MCID.isBranch() ||
        (HexagonMCInstrInfo::getType(MCII, MI) == HexagonII::TypeCR))
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_B9_PCREL_X
                            : Hexagon::fixup_Hexagon_B9_PCREL;
    else if (*Extended)
      FixupKind = Hexagon::fixup_Hexagon_9_X;
    else
      raise_relocation_error(bits, kind);
    break;

  case 8:
    if (*Extended)
      FixupKind = Hexagon::fixup_Hexagon_8_X;
    else
      raise_relocation_error(bits, kind);
    break;

  case 7:
    if (MCID.isBranch() ||
        (HexagonMCInstrInfo::getType(MCII, MI) == HexagonII::TypeCR))
      FixupKind = *Extended ? Hexagon::fixup_Hexagon_B7_PCREL_X
                            : Hexagon::fixup_Hexagon_B7_PCREL;
    else if (*Extended)
      FixupKind = Hexagon::fixup_Hexagon_7_X;
    else
      raise_relocation_error(bits, kind);
    break;

  case 6:
    if (*Extended) {
      switch (kind) {
      case MCSymbolRefExpr::VK_DTPREL:
        FixupKind = Hexagon::fixup_Hexagon_DTPREL_16_X;
        break;
      // This is part of an extender, GOT_11 is a
      // Word32_U6 unsigned/truncated reloc.
      case MCSymbolRefExpr::VK_GOT:
        FixupKind = Hexagon::fixup_Hexagon_GOT_11_X;
        break;
      case MCSymbolRefExpr::VK_GOTREL:
        FixupKind = Hexagon::fixup_Hexagon_GOTREL_11_X;
        break;
      case MCSymbolRefExpr::VK_Hexagon_PCREL:
        FixupKind = Hexagon::fixup_Hexagon_6_PCREL_X;
        break;
      case MCSymbolRefExpr::VK_TPREL:
        FixupKind = Hexagon::fixup_Hexagon_TPREL_16_X;
        break;
      case MCSymbolRefExpr::VK_None:
        FixupKind = Hexagon::fixup_Hexagon_6_X;
        break;
      default:
        raise_relocation_error(bits, kind);
      }
    } else
      raise_relocation_error(bits, kind);
    break;

  case 0:
    FixupKind = getFixupNoBits(MCII, MI, MO, kind);
    break;
  }

  MCExpr const *FixupExpression =
      (*Addend > 0 && isPCRel(FixupKind))
          ? MCBinaryExpr::createAdd(MO.getExpr(),
                                    MCConstantExpr::create(*Addend, MCT), MCT)
          : MO.getExpr();

  MCFixup fixup = MCFixup::create(*Addend, FixupExpression,
                                  MCFixupKind(FixupKind), MI.getLoc());
  Fixups.push_back(fixup);
  // All of the information is in the fixup.
  return 0;
}

unsigned
HexagonMCCodeEmitter::getMachineOpValue(MCInst const &MI, MCOperand const &MO,
                                        SmallVectorImpl<MCFixup> &Fixups,
                                        MCSubtargetInfo const &STI) const {
  assert(!MO.isImm());
  if (MO.isReg()) {
    unsigned Reg = MO.getReg();
    if (HexagonMCInstrInfo::isSubInstruction(MI))
      return HexagonMCInstrInfo::getDuplexRegisterNumbering(Reg);
    switch(MI.getOpcode()){
    case Hexagon::A2_tfrrcr:
    case Hexagon::A2_tfrcrr:
      if(Reg == Hexagon::M0)
        Reg = Hexagon::C6;
      if(Reg == Hexagon::M1)
        Reg = Hexagon::C7;
    }
    return MCT.getRegisterInfo()->getEncodingValue(Reg);
  }

  return getExprOpValue(MI, MO, MO.getExpr(), Fixups, STI);
}

MCCodeEmitter *llvm::createHexagonMCCodeEmitter(MCInstrInfo const &MII,
                                                MCRegisterInfo const &MRI,
                                                MCContext &MCT) {
  return new HexagonMCCodeEmitter(MII, MCT);
}

#define ENABLE_INSTR_PREDICATE_VERIFIER
#include "HexagonGenMCCodeEmitter.inc"
