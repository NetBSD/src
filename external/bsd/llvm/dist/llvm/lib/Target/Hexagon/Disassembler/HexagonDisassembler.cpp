//===-- HexagonDisassembler.cpp - Disassembler for Hexagon ISA ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "hexagon-disassembler"

#include "Hexagon.h"
#include "MCTargetDesc/HexagonBaseInfo.h"
#include "MCTargetDesc/HexagonMCChecker.h"
#include "MCTargetDesc/HexagonMCTargetDesc.h"
#include "MCTargetDesc/HexagonMCInstrInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

using namespace llvm;
using namespace Hexagon;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// \brief Hexagon disassembler for all Hexagon platforms.
class HexagonDisassembler : public MCDisassembler {
public:
  std::unique_ptr<MCInstrInfo const> const MCII;
  std::unique_ptr<MCInst *> CurrentBundle;

  HexagonDisassembler(const MCSubtargetInfo &STI, MCContext &Ctx,
                      MCInstrInfo const *MCII)
      : MCDisassembler(STI, Ctx), MCII(MCII), CurrentBundle(new MCInst *) {}

  DecodeStatus getSingleInstruction(MCInst &Instr, MCInst &MCB,
                                    ArrayRef<uint8_t> Bytes, uint64_t Address,
                                    raw_ostream &VStream, raw_ostream &CStream,
                                    bool &Complete) const;
  DecodeStatus getInstruction(MCInst &Instr, uint64_t &Size,
                              ArrayRef<uint8_t> Bytes, uint64_t Address,
                              raw_ostream &VStream,
                              raw_ostream &CStream) const override;

  void adjustExtendedInstructions(MCInst &MCI, MCInst const &MCB) const;
  void addSubinstOperands(MCInst *MI, unsigned opcode, unsigned inst) const;
};

} // end anonymous namespace

// Forward declare these because the auto-generated code will reference them.
// Definitions are further down.

static DecodeStatus DecodeIntRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeIntRegsLow8RegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder);
static DecodeStatus DecodeVectorRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus DecodeDoubleRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus DecodeVecDblRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t Address,
                                                  const void *Decoder);
static DecodeStatus DecodePredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                uint64_t Address,
                                                const void *Decoder);
static DecodeStatus DecodeVecPredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder);
static DecodeStatus DecodeCtrRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeModRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder);
static DecodeStatus DecodeCtrRegs64RegisterClass(MCInst &Inst, unsigned RegNo,
                                                 uint64_t Address,
                                                 const void *Decoder);

static DecodeStatus decodeSpecial(MCInst &MI, uint32_t insn);
static DecodeStatus decodeImmext(MCInst &MI, uint32_t insn,
                                 void const *Decoder);

static unsigned GetSubinstOpcode(unsigned IClass, unsigned inst, unsigned &op,
                                 raw_ostream &os);

static unsigned getRegFromSubinstEncoding(unsigned encoded_reg);

static DecodeStatus unsignedImmDecoder(MCInst &MI, unsigned tmp,
                                       uint64_t Address, const void *Decoder);
static DecodeStatus s16_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                  const void *Decoder);
static DecodeStatus s12_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                  const void *Decoder);
static DecodeStatus s11_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);
static DecodeStatus s11_1ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);
static DecodeStatus s11_2ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);
static DecodeStatus s11_3ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);
static DecodeStatus s10_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                  const void *Decoder);
static DecodeStatus s8_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                 const void *Decoder);
static DecodeStatus s6_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_1ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_2ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_3ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s4_6ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus s3_6ImmDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                   const void *Decoder);
static DecodeStatus brtargetDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder);

#include "HexagonGenDisassemblerTables.inc"

static MCDisassembler *createHexagonDisassembler(const Target &T,
                                                 const MCSubtargetInfo &STI,
                                                 MCContext &Ctx) {
  return new HexagonDisassembler(STI, Ctx, T.createMCInstrInfo());
}

extern "C" void LLVMInitializeHexagonDisassembler() {
  TargetRegistry::RegisterMCDisassembler(getTheHexagonTarget(),
                                         createHexagonDisassembler);
}

DecodeStatus HexagonDisassembler::getInstruction(MCInst &MI, uint64_t &Size,
                                                 ArrayRef<uint8_t> Bytes,
                                                 uint64_t Address,
                                                 raw_ostream &os,
                                                 raw_ostream &cs) const {
  DecodeStatus Result = DecodeStatus::Success;
  bool Complete = false;
  Size = 0;

  *CurrentBundle = &MI;
  MI = HexagonMCInstrInfo::createBundle();
  while (Result == Success && !Complete) {
    if (Bytes.size() < HEXAGON_INSTR_SIZE)
      return MCDisassembler::Fail;
    MCInst *Inst = new (getContext()) MCInst;
    Result = getSingleInstruction(*Inst, MI, Bytes, Address, os, cs, Complete);
    MI.addOperand(MCOperand::createInst(Inst));
    Size += HEXAGON_INSTR_SIZE;
    Bytes = Bytes.slice(HEXAGON_INSTR_SIZE);
  }
  if(Result == MCDisassembler::Fail)
    return Result;
  HexagonMCChecker Checker (*MCII, STI, MI, MI, *getContext().getRegisterInfo());
  if(!Checker.check())
    return MCDisassembler::Fail;
  return MCDisassembler::Success;
}

static HexagonDisassembler const &disassembler(void const *Decoder) {
  return *static_cast<HexagonDisassembler const *>(Decoder);
}

static MCContext &contextFromDecoder(void const *Decoder) {
  return disassembler(Decoder).getContext();
}

DecodeStatus HexagonDisassembler::getSingleInstruction(
    MCInst &MI, MCInst &MCB, ArrayRef<uint8_t> Bytes, uint64_t Address,
    raw_ostream &os, raw_ostream &cs, bool &Complete) const {
  assert(Bytes.size() >= HEXAGON_INSTR_SIZE);

  uint32_t Instruction =
      (Bytes[3] << 24) | (Bytes[2] << 16) | (Bytes[1] << 8) | (Bytes[0] << 0);

  auto BundleSize = HexagonMCInstrInfo::bundleSize(MCB);
  if ((Instruction & HexagonII::INST_PARSE_MASK) ==
      HexagonII::INST_PARSE_LOOP_END) {
    if (BundleSize == 0)
      HexagonMCInstrInfo::setInnerLoop(MCB);
    else if (BundleSize == 1)
      HexagonMCInstrInfo::setOuterLoop(MCB);
    else
      return DecodeStatus::Fail;
  }

  DecodeStatus Result = DecodeStatus::Success;
  if ((Instruction & HexagonII::INST_PARSE_MASK) ==
      HexagonII::INST_PARSE_DUPLEX) {
    // Determine the instruction class of each instruction in the duplex.
    unsigned duplexIClass, IClassLow, IClassHigh;

    duplexIClass = ((Instruction >> 28) & 0xe) | ((Instruction >> 13) & 0x1);
    switch (duplexIClass) {
    default:
      return MCDisassembler::Fail;
    case 0:
      IClassLow = HexagonII::HSIG_L1;
      IClassHigh = HexagonII::HSIG_L1;
      break;
    case 1:
      IClassLow = HexagonII::HSIG_L2;
      IClassHigh = HexagonII::HSIG_L1;
      break;
    case 2:
      IClassLow = HexagonII::HSIG_L2;
      IClassHigh = HexagonII::HSIG_L2;
      break;
    case 3:
      IClassLow = HexagonII::HSIG_A;
      IClassHigh = HexagonII::HSIG_A;
      break;
    case 4:
      IClassLow = HexagonII::HSIG_L1;
      IClassHigh = HexagonII::HSIG_A;
      break;
    case 5:
      IClassLow = HexagonII::HSIG_L2;
      IClassHigh = HexagonII::HSIG_A;
      break;
    case 6:
      IClassLow = HexagonII::HSIG_S1;
      IClassHigh = HexagonII::HSIG_A;
      break;
    case 7:
      IClassLow = HexagonII::HSIG_S2;
      IClassHigh = HexagonII::HSIG_A;
      break;
    case 8:
      IClassLow = HexagonII::HSIG_S1;
      IClassHigh = HexagonII::HSIG_L1;
      break;
    case 9:
      IClassLow = HexagonII::HSIG_S1;
      IClassHigh = HexagonII::HSIG_L2;
      break;
    case 10:
      IClassLow = HexagonII::HSIG_S1;
      IClassHigh = HexagonII::HSIG_S1;
      break;
    case 11:
      IClassLow = HexagonII::HSIG_S2;
      IClassHigh = HexagonII::HSIG_S1;
      break;
    case 12:
      IClassLow = HexagonII::HSIG_S2;
      IClassHigh = HexagonII::HSIG_L1;
      break;
    case 13:
      IClassLow = HexagonII::HSIG_S2;
      IClassHigh = HexagonII::HSIG_L2;
      break;
    case 14:
      IClassLow = HexagonII::HSIG_S2;
      IClassHigh = HexagonII::HSIG_S2;
      break;
    }

    // Set the MCInst to be a duplex instruction. Which one doesn't matter.
    MI.setOpcode(Hexagon::DuplexIClass0);

    // Decode each instruction in the duplex.
    // Create an MCInst for each instruction.
    unsigned instLow = Instruction & 0x1fff;
    unsigned instHigh = (Instruction >> 16) & 0x1fff;
    unsigned opLow;
    if (GetSubinstOpcode(IClassLow, instLow, opLow, os) !=
        MCDisassembler::Success)
      return MCDisassembler::Fail;
    unsigned opHigh;
    if (GetSubinstOpcode(IClassHigh, instHigh, opHigh, os) !=
        MCDisassembler::Success)
      return MCDisassembler::Fail;
    MCInst *MILow = new (getContext()) MCInst;
    MILow->setOpcode(opLow);
    MCInst *MIHigh = new (getContext()) MCInst;
    MIHigh->setOpcode(opHigh);
    addSubinstOperands(MILow, opLow, instLow);
    addSubinstOperands(MIHigh, opHigh, instHigh);
    // see ConvertToSubInst() in
    // lib/Target/Hexagon/MCTargetDesc/HexagonMCDuplexInfo.cpp

    // Add the duplex instruction MCInsts as operands to the passed in MCInst.
    MCOperand OPLow = MCOperand::createInst(MILow);
    MCOperand OPHigh = MCOperand::createInst(MIHigh);
    MI.addOperand(OPLow);
    MI.addOperand(OPHigh);
    Complete = true;
  } else {
    if ((Instruction & HexagonII::INST_PARSE_MASK) ==
        HexagonII::INST_PARSE_PACKET_END)
      Complete = true;
    // Calling the auto-generated decoder function.
    Result =
        decodeInstruction(DecoderTable32, MI, Instruction, Address, this, STI);

    // If a, "standard" insn isn't found check special cases.
    if (MCDisassembler::Success != Result ||
        MI.getOpcode() == Hexagon::A4_ext) {
      Result = decodeImmext(MI, Instruction, this);
      if (MCDisassembler::Success != Result) {
        Result = decodeSpecial(MI, Instruction);
      }
    } else {
      // If the instruction is a compound instruction, register values will
      // follow the duplex model, so the register values in the MCInst are
      // incorrect. If the instruction is a compound, loop through the
      // operands and change registers appropriately.
      if (HexagonMCInstrInfo::getType(*MCII, MI) == HexagonII::TypeCOMPOUND) {
        for (MCInst::iterator i = MI.begin(), last = MI.end(); i < last; ++i) {
          if (i->isReg()) {
            unsigned reg = i->getReg() - Hexagon::R0;
            i->setReg(getRegFromSubinstEncoding(reg));
          }
        }
      }
    }
  }

  switch(MI.getOpcode()) {
  case Hexagon::J4_cmpeqn1_f_jumpnv_nt:
  case Hexagon::J4_cmpeqn1_f_jumpnv_t:
  case Hexagon::J4_cmpeqn1_fp0_jump_nt:
  case Hexagon::J4_cmpeqn1_fp0_jump_t:
  case Hexagon::J4_cmpeqn1_fp1_jump_nt:
  case Hexagon::J4_cmpeqn1_fp1_jump_t:
  case Hexagon::J4_cmpeqn1_t_jumpnv_nt:
  case Hexagon::J4_cmpeqn1_t_jumpnv_t:
  case Hexagon::J4_cmpeqn1_tp0_jump_nt:
  case Hexagon::J4_cmpeqn1_tp0_jump_t:
  case Hexagon::J4_cmpeqn1_tp1_jump_nt:
  case Hexagon::J4_cmpeqn1_tp1_jump_t:
  case Hexagon::J4_cmpgtn1_f_jumpnv_nt:
  case Hexagon::J4_cmpgtn1_f_jumpnv_t:
  case Hexagon::J4_cmpgtn1_fp0_jump_nt:
  case Hexagon::J4_cmpgtn1_fp0_jump_t:
  case Hexagon::J4_cmpgtn1_fp1_jump_nt:
  case Hexagon::J4_cmpgtn1_fp1_jump_t:
  case Hexagon::J4_cmpgtn1_t_jumpnv_nt:
  case Hexagon::J4_cmpgtn1_t_jumpnv_t:
  case Hexagon::J4_cmpgtn1_tp0_jump_nt:
  case Hexagon::J4_cmpgtn1_tp0_jump_t:
  case Hexagon::J4_cmpgtn1_tp1_jump_nt:
  case Hexagon::J4_cmpgtn1_tp1_jump_t:
    MI.insert(MI.begin() + 1, MCOperand::createExpr(MCConstantExpr::create(-1, getContext())));
    break;
  default:
    break;
  }

  if (HexagonMCInstrInfo::isNewValue(*MCII, MI)) {
    unsigned OpIndex = HexagonMCInstrInfo::getNewValueOp(*MCII, MI);
    MCOperand &MCO = MI.getOperand(OpIndex);
    assert(MCO.isReg() && "New value consumers must be registers");
    unsigned Register =
        getContext().getRegisterInfo()->getEncodingValue(MCO.getReg());
    if ((Register & 0x6) == 0)
      // HexagonPRM 10.11 Bit 1-2 == 0 is reserved
      return MCDisassembler::Fail;
    unsigned Lookback = (Register & 0x6) >> 1;
    unsigned Offset = 1;
    bool Vector = HexagonMCInstrInfo::isVector(*MCII, MI);
    auto Instructions = HexagonMCInstrInfo::bundleInstructions(**CurrentBundle);
    auto i = Instructions.end() - 1;
    for (auto n = Instructions.begin() - 1;; --i, ++Offset) {
      if (i == n)
        // Couldn't find producer
        return MCDisassembler::Fail;
      if (Vector && !HexagonMCInstrInfo::isVector(*MCII, *i->getInst()))
        // Skip scalars when calculating distances for vectors
        ++Lookback;
      if (HexagonMCInstrInfo::isImmext(*i->getInst()))
        ++Lookback;
      if (Offset == Lookback)
        break;
    }
    auto const &Inst = *i->getInst();
    bool SubregBit = (Register & 0x1) != 0;
    if (SubregBit && HexagonMCInstrInfo::hasNewValue2(*MCII, Inst)) {
      // If subreg bit is set we're selecting the second produced newvalue
      unsigned Producer =
          HexagonMCInstrInfo::getNewValueOperand2(*MCII, Inst).getReg();
      assert(Producer != Hexagon::NoRegister);
      MCO.setReg(Producer);
    } else if (HexagonMCInstrInfo::hasNewValue(*MCII, Inst)) {
      unsigned Producer =
          HexagonMCInstrInfo::getNewValueOperand(*MCII, Inst).getReg();
      if (Producer >= Hexagon::W0 && Producer <= Hexagon::W15)
        Producer = ((Producer - Hexagon::W0) << 1) + SubregBit + Hexagon::V0;
      else if (SubregBit)
        // Hexagon PRM 10.11 New-value operands
        // Nt[0] is reserved and should always be encoded as zero.
        return MCDisassembler::Fail;
      assert(Producer != Hexagon::NoRegister);
      MCO.setReg(Producer);
    } else
      return MCDisassembler::Fail;
  }

  adjustExtendedInstructions(MI, MCB);
  MCInst const *Extender =
    HexagonMCInstrInfo::extenderForIndex(MCB,
                                         HexagonMCInstrInfo::bundleSize(MCB));
  if(Extender != nullptr) {
    MCInst const & Inst = HexagonMCInstrInfo::isDuplex(*MCII, MI) ?
                          *MI.getOperand(1).getInst() : MI;
    if (!HexagonMCInstrInfo::isExtendable(*MCII, Inst) &&
        !HexagonMCInstrInfo::isExtended(*MCII, Inst))
      return MCDisassembler::Fail;
  }
  return Result;
}

void HexagonDisassembler::adjustExtendedInstructions(MCInst &MCI,
                                                     MCInst const &MCB) const {
  if (!HexagonMCInstrInfo::hasExtenderForIndex(
          MCB, HexagonMCInstrInfo::bundleSize(MCB))) {
    unsigned opcode;
    // This code is used by the disassembler to disambiguate between GP
    // relative and absolute addressing instructions since they both have
    // same encoding bits. However, an absolute addressing instruction must
    // follow an immediate extender. Disassembler alwaus select absolute
    // addressing instructions first and uses this code to change them into
    // GP relative instruction in the absence of the corresponding immediate
    // extender.
    switch (MCI.getOpcode()) {
    case Hexagon::PS_storerbabs:
      opcode = Hexagon::S2_storerbgp;
      break;
    case Hexagon::PS_storerhabs:
      opcode = Hexagon::S2_storerhgp;
      break;
    case Hexagon::PS_storerfabs:
      opcode = Hexagon::S2_storerfgp;
      break;
    case Hexagon::PS_storeriabs:
      opcode = Hexagon::S2_storerigp;
      break;
    case Hexagon::PS_storerbnewabs:
      opcode = Hexagon::S2_storerbnewgp;
      break;
    case Hexagon::PS_storerhnewabs:
      opcode = Hexagon::S2_storerhnewgp;
      break;
    case Hexagon::PS_storerinewabs:
      opcode = Hexagon::S2_storerinewgp;
      break;
    case Hexagon::PS_storerdabs:
      opcode = Hexagon::S2_storerdgp;
      break;
    case Hexagon::PS_loadrbabs:
      opcode = Hexagon::L2_loadrbgp;
      break;
    case Hexagon::PS_loadrubabs:
      opcode = Hexagon::L2_loadrubgp;
      break;
    case Hexagon::PS_loadrhabs:
      opcode = Hexagon::L2_loadrhgp;
      break;
    case Hexagon::PS_loadruhabs:
      opcode = Hexagon::L2_loadruhgp;
      break;
    case Hexagon::PS_loadriabs:
      opcode = Hexagon::L2_loadrigp;
      break;
    case Hexagon::PS_loadrdabs:
      opcode = Hexagon::L2_loadrdgp;
      break;
    default:
      opcode = MCI.getOpcode();
    }
    MCI.setOpcode(opcode);
  }
}

static DecodeStatus DecodeRegisterClass(MCInst &Inst, unsigned RegNo,
                                        ArrayRef<MCPhysReg> Table) {
  if (RegNo < Table.size()) {
    Inst.addOperand(MCOperand::createReg(Table[RegNo]));
    return MCDisassembler::Success;
  }

  return MCDisassembler::Fail;
}

static DecodeStatus DecodeIntRegsLow8RegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t Address,
                                                   const void *Decoder) {
  return DecodeIntRegsRegisterClass(Inst, RegNo, Address, Decoder);
}

static DecodeStatus DecodeIntRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  static const MCPhysReg IntRegDecoderTable[] = {
      Hexagon::R0,  Hexagon::R1,  Hexagon::R2,  Hexagon::R3,  Hexagon::R4,
      Hexagon::R5,  Hexagon::R6,  Hexagon::R7,  Hexagon::R8,  Hexagon::R9,
      Hexagon::R10, Hexagon::R11, Hexagon::R12, Hexagon::R13, Hexagon::R14,
      Hexagon::R15, Hexagon::R16, Hexagon::R17, Hexagon::R18, Hexagon::R19,
      Hexagon::R20, Hexagon::R21, Hexagon::R22, Hexagon::R23, Hexagon::R24,
      Hexagon::R25, Hexagon::R26, Hexagon::R27, Hexagon::R28, Hexagon::R29,
      Hexagon::R30, Hexagon::R31};

  return DecodeRegisterClass(Inst, RegNo, IntRegDecoderTable);
}

static DecodeStatus DecodeVectorRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg VecRegDecoderTable[] = {
      Hexagon::V0,  Hexagon::V1,  Hexagon::V2,  Hexagon::V3,  Hexagon::V4,
      Hexagon::V5,  Hexagon::V6,  Hexagon::V7,  Hexagon::V8,  Hexagon::V9,
      Hexagon::V10, Hexagon::V11, Hexagon::V12, Hexagon::V13, Hexagon::V14,
      Hexagon::V15, Hexagon::V16, Hexagon::V17, Hexagon::V18, Hexagon::V19,
      Hexagon::V20, Hexagon::V21, Hexagon::V22, Hexagon::V23, Hexagon::V24,
      Hexagon::V25, Hexagon::V26, Hexagon::V27, Hexagon::V28, Hexagon::V29,
      Hexagon::V30, Hexagon::V31};

  return DecodeRegisterClass(Inst, RegNo, VecRegDecoderTable);
}

static DecodeStatus DecodeDoubleRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg DoubleRegDecoderTable[] = {
      Hexagon::D0,  Hexagon::D1,  Hexagon::D2,  Hexagon::D3,
      Hexagon::D4,  Hexagon::D5,  Hexagon::D6,  Hexagon::D7,
      Hexagon::D8,  Hexagon::D9,  Hexagon::D10, Hexagon::D11,
      Hexagon::D12, Hexagon::D13, Hexagon::D14, Hexagon::D15};

  return DecodeRegisterClass(Inst, RegNo >> 1, DoubleRegDecoderTable);
}

static DecodeStatus DecodeVecDblRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                  uint64_t /*Address*/,
                                                  const void *Decoder) {
  static const MCPhysReg VecDblRegDecoderTable[] = {
      Hexagon::W0,  Hexagon::W1,  Hexagon::W2,  Hexagon::W3,
      Hexagon::W4,  Hexagon::W5,  Hexagon::W6,  Hexagon::W7,
      Hexagon::W8,  Hexagon::W9,  Hexagon::W10, Hexagon::W11,
      Hexagon::W12, Hexagon::W13, Hexagon::W14, Hexagon::W15};

  return (DecodeRegisterClass(Inst, RegNo >> 1, VecDblRegDecoderTable));
}

static DecodeStatus DecodePredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                uint64_t /*Address*/,
                                                const void *Decoder) {
  static const MCPhysReg PredRegDecoderTable[] = {Hexagon::P0, Hexagon::P1,
                                                  Hexagon::P2, Hexagon::P3};

  return DecodeRegisterClass(Inst, RegNo, PredRegDecoderTable);
}

static DecodeStatus DecodeVecPredRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                                   uint64_t /*Address*/,
                                                   const void *Decoder) {
  static const MCPhysReg VecPredRegDecoderTable[] = {Hexagon::Q0, Hexagon::Q1,
                                                     Hexagon::Q2, Hexagon::Q3};

  return DecodeRegisterClass(Inst, RegNo, VecPredRegDecoderTable);
}

static DecodeStatus DecodeCtrRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t /*Address*/,
                                               const void *Decoder) {
  static const MCPhysReg CtrlRegDecoderTable[] = {
    Hexagon::SA0, Hexagon::LC0, Hexagon::SA1, Hexagon::LC1,
    Hexagon::P3_0, Hexagon::C5, Hexagon::C6, Hexagon::C7,
    Hexagon::USR, Hexagon::PC, Hexagon::UGP, Hexagon::GP,
    Hexagon::CS0, Hexagon::CS1, Hexagon::UPCL, Hexagon::UPC
  };

  if (RegNo >= array_lengthof(CtrlRegDecoderTable))
    return MCDisassembler::Fail;

  if (CtrlRegDecoderTable[RegNo] == Hexagon::NoRegister)
    return MCDisassembler::Fail;

  unsigned Register = CtrlRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeCtrRegs64RegisterClass(MCInst &Inst, unsigned RegNo,
                                                 uint64_t /*Address*/,
                                                 const void *Decoder) {
  static const MCPhysReg CtrlReg64DecoderTable[] = {
      Hexagon::C1_0,   Hexagon::NoRegister,
      Hexagon::C3_2,   Hexagon::NoRegister,
      Hexagon::C7_6,   Hexagon::NoRegister,
      Hexagon::C9_8,   Hexagon::NoRegister,
      Hexagon::C11_10, Hexagon::NoRegister,
      Hexagon::CS,     Hexagon::NoRegister,
      Hexagon::UPC,    Hexagon::NoRegister
  };

  if (RegNo >= array_lengthof(CtrlReg64DecoderTable))
    return MCDisassembler::Fail;

  if (CtrlReg64DecoderTable[RegNo] == Hexagon::NoRegister)
    return MCDisassembler::Fail;

  unsigned Register = CtrlReg64DecoderTable[RegNo];
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeModRegsRegisterClass(MCInst &Inst, unsigned RegNo,
                                               uint64_t /*Address*/,
                                               const void *Decoder) {
  unsigned Register = 0;
  switch (RegNo) {
  case 0:
    Register = Hexagon::M0;
    break;
  case 1:
    Register = Hexagon::M1;
    break;
  default:
    return MCDisassembler::Fail;
  }
  Inst.addOperand(MCOperand::createReg(Register));
  return MCDisassembler::Success;
}

static uint32_t fullValue(MCInstrInfo const &MCII, MCInst &MCB, MCInst &MI,
                          int64_t Value) {
  MCInst const *Extender = HexagonMCInstrInfo::extenderForIndex(
    MCB, HexagonMCInstrInfo::bundleSize(MCB));
  if(!Extender || MI.size() != HexagonMCInstrInfo::getExtendableOp(MCII, MI))
    return Value;
  unsigned Alignment = HexagonMCInstrInfo::getExtentAlignment(MCII, MI);
  uint32_t Lower6 = static_cast<uint32_t>(Value >> Alignment) & 0x3f;
  int64_t Bits;
  bool Success = Extender->getOperand(0).getExpr()->evaluateAsAbsolute(Bits);
  assert(Success);(void)Success;
  uint32_t Upper26 = static_cast<uint32_t>(Bits);
  uint32_t Operand = Upper26 | Lower6;
  return Operand;
}

template <size_t T>
static void signedDecoder(MCInst &MI, unsigned tmp, const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  int64_t FullValue = fullValue(*Disassembler.MCII,
                                **Disassembler.CurrentBundle,
                                MI, SignExtend64<T>(tmp));
  int64_t Extended = SignExtend64<32>(FullValue);
  HexagonMCInstrInfo::addConstant(MI, Extended,
                                  Disassembler.getContext());
}

static DecodeStatus unsignedImmDecoder(MCInst &MI, unsigned tmp,
                                       uint64_t /*Address*/,
                                       const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  int64_t FullValue = fullValue(*Disassembler.MCII,
                                **Disassembler.CurrentBundle,
                                MI, tmp);
  assert(FullValue >= 0 && "Negative in unsigned decoder");
  HexagonMCInstrInfo::addConstant(MI, FullValue, Disassembler.getContext());
  return MCDisassembler::Success;
}

static DecodeStatus s16_0ImmDecoder(MCInst &MI, unsigned tmp,
                                  uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<16>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s12_0ImmDecoder(MCInst &MI, unsigned tmp,
                                  uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<12>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s11_0ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<11>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s11_1ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder) {
  HexagonMCInstrInfo::addConstant(MI, SignExtend64<12>(tmp), contextFromDecoder(Decoder));
  return MCDisassembler::Success;
}

static DecodeStatus s11_2ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<13>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s11_3ImmDecoder(MCInst &MI, unsigned tmp,
                                    uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<14>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s10_0ImmDecoder(MCInst &MI, unsigned tmp,
                                  uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<10>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s8_0ImmDecoder(MCInst &MI, unsigned tmp, uint64_t /*Address*/,
                                 const void *Decoder) {
  signedDecoder<8>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s6_0ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<6>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s4_0ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<4>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s4_1ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<5>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s4_2ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<6>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s4_3ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<7>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s4_6ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<10>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

static DecodeStatus s3_6ImmDecoder(MCInst &MI, unsigned tmp,
                                   uint64_t /*Address*/, const void *Decoder) {
  signedDecoder<19>(MI, tmp, Decoder);
  return MCDisassembler::Success;
}

// custom decoder for various jump/call immediates
static DecodeStatus brtargetDecoder(MCInst &MI, unsigned tmp, uint64_t Address,
                                    const void *Decoder) {
  HexagonDisassembler const &Disassembler = disassembler(Decoder);
  unsigned Bits = HexagonMCInstrInfo::getExtentBits(*Disassembler.MCII, MI);
  // r13_2 is not extendable, so if there are no extent bits, it's r13_2
  if (Bits == 0)
    Bits = 15;
  uint32_t FullValue = fullValue(*Disassembler.MCII,
                                **Disassembler.CurrentBundle,
                                MI, SignExtend64(tmp, Bits));
  int64_t Extended = SignExtend64<32>(FullValue) + Address;
  if (!Disassembler.tryAddingSymbolicOperand(MI, Extended, Address, true,
                                              0, 4))
    HexagonMCInstrInfo::addConstant(MI, Extended, Disassembler.getContext());
  return MCDisassembler::Success;
}

// Addressing mode dependent load store opcode map.
//   - If an insn is preceded by an extender the address is absolute.
//      - memw(##symbol) = r0
//   - If an insn is not preceded by an extender the address is GP relative.
//      - memw(gp + #symbol) = r0
// Please note that the instructions must be ordered in the descending order
// of their opcode.
// HexagonII::INST_ICLASS_ST
static const unsigned int StoreConditionalOpcodeData[][2] = {
    {S4_pstorerdfnew_abs, 0xafc02084},
    {S4_pstorerdtnew_abs, 0xafc02080},
    {S4_pstorerdf_abs, 0xafc00084},
    {S4_pstorerdt_abs, 0xafc00080},
    {S4_pstorerinewfnew_abs, 0xafa03084},
    {S4_pstorerinewtnew_abs, 0xafa03080},
    {S4_pstorerhnewfnew_abs, 0xafa02884},
    {S4_pstorerhnewtnew_abs, 0xafa02880},
    {S4_pstorerbnewfnew_abs, 0xafa02084},
    {S4_pstorerbnewtnew_abs, 0xafa02080},
    {S4_pstorerinewf_abs, 0xafa01084},
    {S4_pstorerinewt_abs, 0xafa01080},
    {S4_pstorerhnewf_abs, 0xafa00884},
    {S4_pstorerhnewt_abs, 0xafa00880},
    {S4_pstorerbnewf_abs, 0xafa00084},
    {S4_pstorerbnewt_abs, 0xafa00080},
    {S4_pstorerifnew_abs, 0xaf802084},
    {S4_pstoreritnew_abs, 0xaf802080},
    {S4_pstorerif_abs, 0xaf800084},
    {S4_pstorerit_abs, 0xaf800080},
    {S4_pstorerhfnew_abs, 0xaf402084},
    {S4_pstorerhtnew_abs, 0xaf402080},
    {S4_pstorerhf_abs, 0xaf400084},
    {S4_pstorerht_abs, 0xaf400080},
    {S4_pstorerbfnew_abs, 0xaf002084},
    {S4_pstorerbtnew_abs, 0xaf002080},
    {S4_pstorerbf_abs, 0xaf000084},
    {S4_pstorerbt_abs, 0xaf000080}};
// HexagonII::INST_ICLASS_LD

// HexagonII::INST_ICLASS_LD_ST_2
static unsigned int LoadStoreOpcodeData[][2] = {{PS_loadrdabs, 0x49c00000},
                                                {PS_loadriabs, 0x49800000},
                                                {PS_loadruhabs, 0x49600000},
                                                {PS_loadrhabs, 0x49400000},
                                                {PS_loadrubabs, 0x49200000},
                                                {PS_loadrbabs, 0x49000000},
                                                {PS_storerdabs, 0x48c00000},
                                                {PS_storerinewabs, 0x48a01000},
                                                {PS_storerhnewabs, 0x48a00800},
                                                {PS_storerbnewabs, 0x48a00000},
                                                {PS_storeriabs, 0x48800000},
                                                {PS_storerfabs, 0x48600000},
                                                {PS_storerhabs, 0x48400000},
                                                {PS_storerbabs, 0x48000000}};
static const size_t NumCondS = array_lengthof(StoreConditionalOpcodeData);
static const size_t NumLS = array_lengthof(LoadStoreOpcodeData);

static DecodeStatus decodeSpecial(MCInst &MI, uint32_t insn) {
  unsigned MachineOpcode = 0;
  unsigned LLVMOpcode = 0;

  if ((insn & HexagonII::INST_ICLASS_MASK) == HexagonII::INST_ICLASS_ST) {
    for (size_t i = 0; i < NumCondS; ++i) {
      if ((insn & StoreConditionalOpcodeData[i][1]) ==
          StoreConditionalOpcodeData[i][1]) {
        MachineOpcode = StoreConditionalOpcodeData[i][1];
        LLVMOpcode = StoreConditionalOpcodeData[i][0];
        break;
      }
    }
  }
  if ((insn & HexagonII::INST_ICLASS_MASK) == HexagonII::INST_ICLASS_LD_ST_2) {
    for (size_t i = 0; i < NumLS; ++i) {
      if ((insn & LoadStoreOpcodeData[i][1]) == LoadStoreOpcodeData[i][1]) {
        MachineOpcode = LoadStoreOpcodeData[i][1];
        LLVMOpcode = LoadStoreOpcodeData[i][0];
        break;
      }
    }
  }

  if (MachineOpcode) {
    unsigned Value = 0;
    unsigned shift = 0;
    MI.setOpcode(LLVMOpcode);
    // Remove the parse bits from the insn.
    insn &= ~HexagonII::INST_PARSE_MASK;

    switch (LLVMOpcode) {
    default:
      return MCDisassembler::Fail;
      break;

    case Hexagon::S4_pstorerdf_abs:
    case Hexagon::S4_pstorerdt_abs:
    case Hexagon::S4_pstorerdfnew_abs:
    case Hexagon::S4_pstorerdtnew_abs:
      // op: Pv
      Value = insn & UINT64_C(3);
      DecodePredRegsRegisterClass(MI, Value, 0, nullptr);
      // op: u6
      Value = (insn >> 12) & UINT64_C(48);
      Value |= (insn >> 3) & UINT64_C(15);
      MI.addOperand(MCOperand::createImm(Value));
      // op: Rtt
      Value = (insn >> 8) & UINT64_C(31);
      DecodeDoubleRegsRegisterClass(MI, Value, 0, nullptr);
      break;

    case Hexagon::S4_pstorerbnewf_abs:
    case Hexagon::S4_pstorerbnewt_abs:
    case Hexagon::S4_pstorerbnewfnew_abs:
    case Hexagon::S4_pstorerbnewtnew_abs:
    case Hexagon::S4_pstorerhnewf_abs:
    case Hexagon::S4_pstorerhnewt_abs:
    case Hexagon::S4_pstorerhnewfnew_abs:
    case Hexagon::S4_pstorerhnewtnew_abs:
    case Hexagon::S4_pstorerinewf_abs:
    case Hexagon::S4_pstorerinewt_abs:
    case Hexagon::S4_pstorerinewfnew_abs:
    case Hexagon::S4_pstorerinewtnew_abs:
      // op: Pv
      Value = insn & UINT64_C(3);
      DecodePredRegsRegisterClass(MI, Value, 0, nullptr);
      // op: u6
      Value = (insn >> 12) & UINT64_C(48);
      Value |= (insn >> 3) & UINT64_C(15);
      MI.addOperand(MCOperand::createImm(Value));
      // op: Nt
      Value = (insn >> 8) & UINT64_C(7);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      break;

    case Hexagon::S4_pstorerbf_abs:
    case Hexagon::S4_pstorerbt_abs:
    case Hexagon::S4_pstorerbfnew_abs:
    case Hexagon::S4_pstorerbtnew_abs:
    case Hexagon::S4_pstorerhf_abs:
    case Hexagon::S4_pstorerht_abs:
    case Hexagon::S4_pstorerhfnew_abs:
    case Hexagon::S4_pstorerhtnew_abs:
    case Hexagon::S4_pstorerif_abs:
    case Hexagon::S4_pstorerit_abs:
    case Hexagon::S4_pstorerifnew_abs:
    case Hexagon::S4_pstoreritnew_abs:
      // op: Pv
      Value = insn & UINT64_C(3);
      DecodePredRegsRegisterClass(MI, Value, 0, nullptr);
      // op: u6
      Value = (insn >> 12) & UINT64_C(48);
      Value |= (insn >> 3) & UINT64_C(15);
      MI.addOperand(MCOperand::createImm(Value));
      // op: Rt
      Value = (insn >> 8) & UINT64_C(31);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      break;

    case Hexagon::L4_ploadrdf_abs:
    case Hexagon::L4_ploadrdt_abs:
    case Hexagon::L4_ploadrdfnew_abs:
    case Hexagon::L4_ploadrdtnew_abs:
      // op: Rdd
      Value = insn & UINT64_C(31);
      DecodeDoubleRegsRegisterClass(MI, Value, 0, nullptr);
      // op: Pt
      Value = ((insn >> 9) & UINT64_C(3));
      DecodePredRegsRegisterClass(MI, Value, 0, nullptr);
      // op: u6
      Value = ((insn >> 15) & UINT64_C(62));
      Value |= ((insn >> 8) & UINT64_C(1));
      MI.addOperand(MCOperand::createImm(Value));
      break;

    case Hexagon::L4_ploadrbf_abs:
    case Hexagon::L4_ploadrbt_abs:
    case Hexagon::L4_ploadrbfnew_abs:
    case Hexagon::L4_ploadrbtnew_abs:
    case Hexagon::L4_ploadrhf_abs:
    case Hexagon::L4_ploadrht_abs:
    case Hexagon::L4_ploadrhfnew_abs:
    case Hexagon::L4_ploadrhtnew_abs:
    case Hexagon::L4_ploadrubf_abs:
    case Hexagon::L4_ploadrubt_abs:
    case Hexagon::L4_ploadrubfnew_abs:
    case Hexagon::L4_ploadrubtnew_abs:
    case Hexagon::L4_ploadruhf_abs:
    case Hexagon::L4_ploadruht_abs:
    case Hexagon::L4_ploadruhfnew_abs:
    case Hexagon::L4_ploadruhtnew_abs:
    case Hexagon::L4_ploadrif_abs:
    case Hexagon::L4_ploadrit_abs:
    case Hexagon::L4_ploadrifnew_abs:
    case Hexagon::L4_ploadritnew_abs:
      // op: Rd
      Value = insn & UINT64_C(31);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      // op: Pt
      Value = (insn >> 9) & UINT64_C(3);
      DecodePredRegsRegisterClass(MI, Value, 0, nullptr);
      // op: u6
      Value = (insn >> 15) & UINT64_C(62);
      Value |= (insn >> 8) & UINT64_C(1);
      MI.addOperand(MCOperand::createImm(Value));
      break;

    // op: g16_2
    case (Hexagon::PS_loadriabs):
      ++shift;
    // op: g16_1
    case Hexagon::PS_loadrhabs:
    case Hexagon::PS_loadruhabs:
      ++shift;
    // op: g16_0
    case Hexagon::PS_loadrbabs:
    case Hexagon::PS_loadrubabs:
      // op: Rd
      Value |= insn & UINT64_C(31);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      Value = (insn >> 11) & UINT64_C(49152);
      Value |= (insn >> 7) & UINT64_C(15872);
      Value |= (insn >> 5) & UINT64_C(511);
      MI.addOperand(MCOperand::createImm(Value << shift));
      break;

    case Hexagon::PS_loadrdabs:
      Value = insn & UINT64_C(31);
      DecodeDoubleRegsRegisterClass(MI, Value, 0, nullptr);
      Value = (insn >> 11) & UINT64_C(49152);
      Value |= (insn >> 7) & UINT64_C(15872);
      Value |= (insn >> 5) & UINT64_C(511);
      MI.addOperand(MCOperand::createImm(Value << 3));
      break;

    case Hexagon::PS_storerdabs:
      // op: g16_3
      Value = (insn >> 11) & UINT64_C(49152);
      Value |= (insn >> 7) & UINT64_C(15872);
      Value |= (insn >> 5) & UINT64_C(256);
      Value |= insn & UINT64_C(255);
      MI.addOperand(MCOperand::createImm(Value << 3));
      // op: Rtt
      Value = (insn >> 8) & UINT64_C(31);
      DecodeDoubleRegsRegisterClass(MI, Value, 0, nullptr);
      break;

    // op: g16_2
    case Hexagon::PS_storerinewabs:
      ++shift;
    // op: g16_1
    case Hexagon::PS_storerhnewabs:
      ++shift;
    // op: g16_0
    case Hexagon::PS_storerbnewabs:
      Value = (insn >> 11) & UINT64_C(49152);
      Value |= (insn >> 7) & UINT64_C(15872);
      Value |= (insn >> 5) & UINT64_C(256);
      Value |= insn & UINT64_C(255);
      MI.addOperand(MCOperand::createImm(Value << shift));
      // op: Nt
      Value = (insn >> 8) & UINT64_C(7);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      break;

    // op: g16_2
    case Hexagon::PS_storeriabs:
      ++shift;
    // op: g16_1
    case Hexagon::PS_storerhabs:
    case Hexagon::PS_storerfabs:
      ++shift;
    // op: g16_0
    case Hexagon::PS_storerbabs:
      Value = (insn >> 11) & UINT64_C(49152);
      Value |= (insn >> 7) & UINT64_C(15872);
      Value |= (insn >> 5) & UINT64_C(256);
      Value |= insn & UINT64_C(255);
      MI.addOperand(MCOperand::createImm(Value << shift));
      // op: Rt
      Value = (insn >> 8) & UINT64_C(31);
      DecodeIntRegsRegisterClass(MI, Value, 0, nullptr);
      break;
    }
    return MCDisassembler::Success;
  }
  return MCDisassembler::Fail;
}

static DecodeStatus decodeImmext(MCInst &MI, uint32_t insn,
                                 void const *Decoder) {
  // Instruction Class for a constant a extender: bits 31:28 = 0x0000
  if ((~insn & 0xf0000000) == 0xf0000000) {
    unsigned Value;
    // 27:16 High 12 bits of 26-bit extender.
    Value = (insn & 0x0fff0000) << 4;
    // 13:0 Low 14 bits of 26-bit extender.
    Value |= ((insn & 0x3fff) << 6);
    MI.setOpcode(Hexagon::A4_ext);
    HexagonMCInstrInfo::addConstant(MI, Value, contextFromDecoder(Decoder));
    return MCDisassembler::Success;
  }
  return MCDisassembler::Fail;
}

// These values are from HexagonGenMCCodeEmitter.inc and HexagonIsetDx.td
enum subInstBinaryValues {
  SA1_addi_BITS = 0x0000,
  SA1_addi_MASK = 0x1800,
  SA1_addrx_BITS = 0x1800,
  SA1_addrx_MASK = 0x1f00,
  SA1_addsp_BITS = 0x0c00,
  SA1_addsp_MASK = 0x1c00,
  SA1_and1_BITS = 0x1200,
  SA1_and1_MASK = 0x1f00,
  SA1_clrf_BITS = 0x1a70,
  SA1_clrf_MASK = 0x1e70,
  SA1_clrfnew_BITS = 0x1a50,
  SA1_clrfnew_MASK = 0x1e70,
  SA1_clrt_BITS = 0x1a60,
  SA1_clrt_MASK = 0x1e70,
  SA1_clrtnew_BITS = 0x1a40,
  SA1_clrtnew_MASK = 0x1e70,
  SA1_cmpeqi_BITS = 0x1900,
  SA1_cmpeqi_MASK = 0x1f00,
  SA1_combine0i_BITS = 0x1c00,
  SA1_combine0i_MASK = 0x1d18,
  SA1_combine1i_BITS = 0x1c08,
  SA1_combine1i_MASK = 0x1d18,
  SA1_combine2i_BITS = 0x1c10,
  SA1_combine2i_MASK = 0x1d18,
  SA1_combine3i_BITS = 0x1c18,
  SA1_combine3i_MASK = 0x1d18,
  SA1_combinerz_BITS = 0x1d08,
  SA1_combinerz_MASK = 0x1d08,
  SA1_combinezr_BITS = 0x1d00,
  SA1_combinezr_MASK = 0x1d08,
  SA1_dec_BITS = 0x1300,
  SA1_dec_MASK = 0x1f00,
  SA1_inc_BITS = 0x1100,
  SA1_inc_MASK = 0x1f00,
  SA1_seti_BITS = 0x0800,
  SA1_seti_MASK = 0x1c00,
  SA1_setin1_BITS = 0x1a00,
  SA1_setin1_MASK = 0x1e40,
  SA1_sxtb_BITS = 0x1500,
  SA1_sxtb_MASK = 0x1f00,
  SA1_sxth_BITS = 0x1400,
  SA1_sxth_MASK = 0x1f00,
  SA1_tfr_BITS = 0x1000,
  SA1_tfr_MASK = 0x1f00,
  SA1_zxtb_BITS = 0x1700,
  SA1_zxtb_MASK = 0x1f00,
  SA1_zxth_BITS = 0x1600,
  SA1_zxth_MASK = 0x1f00,
  SL1_loadri_io_BITS = 0x0000,
  SL1_loadri_io_MASK = 0x1000,
  SL1_loadrub_io_BITS = 0x1000,
  SL1_loadrub_io_MASK = 0x1000,
  SL2_deallocframe_BITS = 0x1f00,
  SL2_deallocframe_MASK = 0x1fc0,
  SL2_jumpr31_BITS = 0x1fc0,
  SL2_jumpr31_MASK = 0x1fc4,
  SL2_jumpr31_f_BITS = 0x1fc5,
  SL2_jumpr31_f_MASK = 0x1fc7,
  SL2_jumpr31_fnew_BITS = 0x1fc7,
  SL2_jumpr31_fnew_MASK = 0x1fc7,
  SL2_jumpr31_t_BITS = 0x1fc4,
  SL2_jumpr31_t_MASK = 0x1fc7,
  SL2_jumpr31_tnew_BITS = 0x1fc6,
  SL2_jumpr31_tnew_MASK = 0x1fc7,
  SL2_loadrb_io_BITS = 0x1000,
  SL2_loadrb_io_MASK = 0x1800,
  SL2_loadrd_sp_BITS = 0x1e00,
  SL2_loadrd_sp_MASK = 0x1f00,
  SL2_loadrh_io_BITS = 0x0000,
  SL2_loadrh_io_MASK = 0x1800,
  SL2_loadri_sp_BITS = 0x1c00,
  SL2_loadri_sp_MASK = 0x1e00,
  SL2_loadruh_io_BITS = 0x0800,
  SL2_loadruh_io_MASK = 0x1800,
  SL2_return_BITS = 0x1f40,
  SL2_return_MASK = 0x1fc4,
  SL2_return_f_BITS = 0x1f45,
  SL2_return_f_MASK = 0x1fc7,
  SL2_return_fnew_BITS = 0x1f47,
  SL2_return_fnew_MASK = 0x1fc7,
  SL2_return_t_BITS = 0x1f44,
  SL2_return_t_MASK = 0x1fc7,
  SL2_return_tnew_BITS = 0x1f46,
  SL2_return_tnew_MASK = 0x1fc7,
  SS1_storeb_io_BITS = 0x1000,
  SS1_storeb_io_MASK = 0x1000,
  SS1_storew_io_BITS = 0x0000,
  SS1_storew_io_MASK = 0x1000,
  SS2_allocframe_BITS = 0x1c00,
  SS2_allocframe_MASK = 0x1e00,
  SS2_storebi0_BITS = 0x1200,
  SS2_storebi0_MASK = 0x1f00,
  SS2_storebi1_BITS = 0x1300,
  SS2_storebi1_MASK = 0x1f00,
  SS2_stored_sp_BITS = 0x0a00,
  SS2_stored_sp_MASK = 0x1e00,
  SS2_storeh_io_BITS = 0x0000,
  SS2_storeh_io_MASK = 0x1800,
  SS2_storew_sp_BITS = 0x0800,
  SS2_storew_sp_MASK = 0x1e00,
  SS2_storewi0_BITS = 0x1000,
  SS2_storewi0_MASK = 0x1f00,
  SS2_storewi1_BITS = 0x1100,
  SS2_storewi1_MASK = 0x1f00
};

static unsigned GetSubinstOpcode(unsigned IClass, unsigned inst, unsigned &op,
                                 raw_ostream &os) {
  switch (IClass) {
  case HexagonII::HSIG_L1:
    if ((inst & SL1_loadri_io_MASK) == SL1_loadri_io_BITS)
      op = Hexagon::SL1_loadri_io;
    else if ((inst & SL1_loadrub_io_MASK) == SL1_loadrub_io_BITS)
      op = Hexagon::SL1_loadrub_io;
    else {
      os << "<unknown subinstruction>";
      return MCDisassembler::Fail;
    }
    break;
  case HexagonII::HSIG_L2:
    if ((inst & SL2_deallocframe_MASK) == SL2_deallocframe_BITS)
      op = Hexagon::SL2_deallocframe;
    else if ((inst & SL2_jumpr31_MASK) == SL2_jumpr31_BITS)
      op = Hexagon::SL2_jumpr31;
    else if ((inst & SL2_jumpr31_f_MASK) == SL2_jumpr31_f_BITS)
      op = Hexagon::SL2_jumpr31_f;
    else if ((inst & SL2_jumpr31_fnew_MASK) == SL2_jumpr31_fnew_BITS)
      op = Hexagon::SL2_jumpr31_fnew;
    else if ((inst & SL2_jumpr31_t_MASK) == SL2_jumpr31_t_BITS)
      op = Hexagon::SL2_jumpr31_t;
    else if ((inst & SL2_jumpr31_tnew_MASK) == SL2_jumpr31_tnew_BITS)
      op = Hexagon::SL2_jumpr31_tnew;
    else if ((inst & SL2_loadrb_io_MASK) == SL2_loadrb_io_BITS)
      op = Hexagon::SL2_loadrb_io;
    else if ((inst & SL2_loadrd_sp_MASK) == SL2_loadrd_sp_BITS)
      op = Hexagon::SL2_loadrd_sp;
    else if ((inst & SL2_loadrh_io_MASK) == SL2_loadrh_io_BITS)
      op = Hexagon::SL2_loadrh_io;
    else if ((inst & SL2_loadri_sp_MASK) == SL2_loadri_sp_BITS)
      op = Hexagon::SL2_loadri_sp;
    else if ((inst & SL2_loadruh_io_MASK) == SL2_loadruh_io_BITS)
      op = Hexagon::SL2_loadruh_io;
    else if ((inst & SL2_return_MASK) == SL2_return_BITS)
      op = Hexagon::SL2_return;
    else if ((inst & SL2_return_f_MASK) == SL2_return_f_BITS)
      op = Hexagon::SL2_return_f;
    else if ((inst & SL2_return_fnew_MASK) == SL2_return_fnew_BITS)
      op = Hexagon::SL2_return_fnew;
    else if ((inst & SL2_return_t_MASK) == SL2_return_t_BITS)
      op = Hexagon::SL2_return_t;
    else if ((inst & SL2_return_tnew_MASK) == SL2_return_tnew_BITS)
      op = Hexagon::SL2_return_tnew;
    else {
      os << "<unknown subinstruction>";
      return MCDisassembler::Fail;
    }
    break;
  case HexagonII::HSIG_A:
    if ((inst & SA1_addi_MASK) == SA1_addi_BITS)
      op = Hexagon::SA1_addi;
    else if ((inst & SA1_addrx_MASK) == SA1_addrx_BITS)
      op = Hexagon::SA1_addrx;
    else if ((inst & SA1_addsp_MASK) == SA1_addsp_BITS)
      op = Hexagon::SA1_addsp;
    else if ((inst & SA1_and1_MASK) == SA1_and1_BITS)
      op = Hexagon::SA1_and1;
    else if ((inst & SA1_clrf_MASK) == SA1_clrf_BITS)
      op = Hexagon::SA1_clrf;
    else if ((inst & SA1_clrfnew_MASK) == SA1_clrfnew_BITS)
      op = Hexagon::SA1_clrfnew;
    else if ((inst & SA1_clrt_MASK) == SA1_clrt_BITS)
      op = Hexagon::SA1_clrt;
    else if ((inst & SA1_clrtnew_MASK) == SA1_clrtnew_BITS)
      op = Hexagon::SA1_clrtnew;
    else if ((inst & SA1_cmpeqi_MASK) == SA1_cmpeqi_BITS)
      op = Hexagon::SA1_cmpeqi;
    else if ((inst & SA1_combine0i_MASK) == SA1_combine0i_BITS)
      op = Hexagon::SA1_combine0i;
    else if ((inst & SA1_combine1i_MASK) == SA1_combine1i_BITS)
      op = Hexagon::SA1_combine1i;
    else if ((inst & SA1_combine2i_MASK) == SA1_combine2i_BITS)
      op = Hexagon::SA1_combine2i;
    else if ((inst & SA1_combine3i_MASK) == SA1_combine3i_BITS)
      op = Hexagon::SA1_combine3i;
    else if ((inst & SA1_combinerz_MASK) == SA1_combinerz_BITS)
      op = Hexagon::SA1_combinerz;
    else if ((inst & SA1_combinezr_MASK) == SA1_combinezr_BITS)
      op = Hexagon::SA1_combinezr;
    else if ((inst & SA1_dec_MASK) == SA1_dec_BITS)
      op = Hexagon::SA1_dec;
    else if ((inst & SA1_inc_MASK) == SA1_inc_BITS)
      op = Hexagon::SA1_inc;
    else if ((inst & SA1_seti_MASK) == SA1_seti_BITS)
      op = Hexagon::SA1_seti;
    else if ((inst & SA1_setin1_MASK) == SA1_setin1_BITS)
      op = Hexagon::SA1_setin1;
    else if ((inst & SA1_sxtb_MASK) == SA1_sxtb_BITS)
      op = Hexagon::SA1_sxtb;
    else if ((inst & SA1_sxth_MASK) == SA1_sxth_BITS)
      op = Hexagon::SA1_sxth;
    else if ((inst & SA1_tfr_MASK) == SA1_tfr_BITS)
      op = Hexagon::SA1_tfr;
    else if ((inst & SA1_zxtb_MASK) == SA1_zxtb_BITS)
      op = Hexagon::SA1_zxtb;
    else if ((inst & SA1_zxth_MASK) == SA1_zxth_BITS)
      op = Hexagon::SA1_zxth;
    else {
      os << "<unknown subinstruction>";
      return MCDisassembler::Fail;
    }
    break;
  case HexagonII::HSIG_S1:
    if ((inst & SS1_storeb_io_MASK) == SS1_storeb_io_BITS)
      op = Hexagon::SS1_storeb_io;
    else if ((inst & SS1_storew_io_MASK) == SS1_storew_io_BITS)
      op = Hexagon::SS1_storew_io;
    else {
      os << "<unknown subinstruction>";
      return MCDisassembler::Fail;
    }
    break;
  case HexagonII::HSIG_S2:
    if ((inst & SS2_allocframe_MASK) == SS2_allocframe_BITS)
      op = Hexagon::SS2_allocframe;
    else if ((inst & SS2_storebi0_MASK) == SS2_storebi0_BITS)
      op = Hexagon::SS2_storebi0;
    else if ((inst & SS2_storebi1_MASK) == SS2_storebi1_BITS)
      op = Hexagon::SS2_storebi1;
    else if ((inst & SS2_stored_sp_MASK) == SS2_stored_sp_BITS)
      op = Hexagon::SS2_stored_sp;
    else if ((inst & SS2_storeh_io_MASK) == SS2_storeh_io_BITS)
      op = Hexagon::SS2_storeh_io;
    else if ((inst & SS2_storew_sp_MASK) == SS2_storew_sp_BITS)
      op = Hexagon::SS2_storew_sp;
    else if ((inst & SS2_storewi0_MASK) == SS2_storewi0_BITS)
      op = Hexagon::SS2_storewi0;
    else if ((inst & SS2_storewi1_MASK) == SS2_storewi1_BITS)
      op = Hexagon::SS2_storewi1;
    else {
      os << "<unknown subinstruction>";
      return MCDisassembler::Fail;
    }
    break;
  default:
    os << "<unknown>";
    return MCDisassembler::Fail;
  }
  return MCDisassembler::Success;
}

static unsigned getRegFromSubinstEncoding(unsigned encoded_reg) {
  if (encoded_reg < 8)
    return Hexagon::R0 + encoded_reg;
  else if (encoded_reg < 16)
    return Hexagon::R0 + encoded_reg + 8;

  // patently false value
  return Hexagon::NoRegister;
}

static unsigned getDRegFromSubinstEncoding(unsigned encoded_dreg) {
  if (encoded_dreg < 4)
    return Hexagon::D0 + encoded_dreg;
  else if (encoded_dreg < 8)
    return Hexagon::D0 + encoded_dreg + 4;

  // patently false value
  return Hexagon::NoRegister;
}

void HexagonDisassembler::addSubinstOperands(MCInst *MI, unsigned opcode,
                                             unsigned inst) const {
  int64_t operand;
  MCOperand Op;
  switch (opcode) {
  case Hexagon::SL2_deallocframe:
  case Hexagon::SL2_jumpr31:
  case Hexagon::SL2_jumpr31_f:
  case Hexagon::SL2_jumpr31_fnew:
  case Hexagon::SL2_jumpr31_t:
  case Hexagon::SL2_jumpr31_tnew:
  case Hexagon::SL2_return:
  case Hexagon::SL2_return_f:
  case Hexagon::SL2_return_fnew:
  case Hexagon::SL2_return_t:
  case Hexagon::SL2_return_tnew:
    // no operands for these instructions
    break;
  case Hexagon::SS2_allocframe:
    // u 8-4{5_3}
    operand = ((inst & 0x1f0) >> 4) << 3;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL1_loadri_io:
    // Rd 3-0, Rs 7-4, u 11-8{4_2}
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0xf00) >> 6;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL1_loadrub_io:
    // Rd 3-0, Rs 7-4, u 11-8
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0xf00) >> 8;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL2_loadrb_io:
    // Rd 3-0, Rs 7-4, u 10-8
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0x700) >> 8;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL2_loadrh_io:
  case Hexagon::SL2_loadruh_io:
    // Rd 3-0, Rs 7-4, u 10-8{3_1}
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0x700) >> 8) << 1;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL2_loadrd_sp:
    // Rdd 2-0, u 7-3{5_3}
    operand = getDRegFromSubinstEncoding(inst & 0x7);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0x0f8) >> 3) << 3;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SL2_loadri_sp:
    // Rd 3-0, u 8-4{5_2}
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0x1f0) >> 4) << 2;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_addi:
    // Rx 3-0 (x2), s7 10-4
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    MI->addOperand(Op);
    operand = SignExtend64<7>((inst & 0x7f0) >> 4);
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_addrx:
    // Rx 3-0 (x2), Rs 7-4
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SA1_and1:
  case Hexagon::SA1_dec:
  case Hexagon::SA1_inc:
  case Hexagon::SA1_sxtb:
  case Hexagon::SA1_sxth:
  case Hexagon::SA1_tfr:
  case Hexagon::SA1_zxtb:
  case Hexagon::SA1_zxth:
    // Rd 3-0, Rs 7-4
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SA1_addsp:
    // Rd 3-0, u 9-4{6_2}
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0x3f0) >> 4) << 2;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_seti:
    // Rd 3-0, u 9-4
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0x3f0) >> 4;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_clrf:
  case Hexagon::SA1_clrfnew:
  case Hexagon::SA1_clrt:
  case Hexagon::SA1_clrtnew:
  case Hexagon::SA1_setin1:
    // Rd 3-0
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    if (opcode == Hexagon::SA1_setin1)
      break;
    MI->addOperand(MCOperand::createReg(Hexagon::P0));
    break;
  case Hexagon::SA1_cmpeqi:
    // Rs 7-4, u 1-0
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = inst & 0x3;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_combine0i:
  case Hexagon::SA1_combine1i:
  case Hexagon::SA1_combine2i:
  case Hexagon::SA1_combine3i:
    // Rdd 2-0, u 6-5
    operand = getDRegFromSubinstEncoding(inst & 0x7);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0x060) >> 5;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SA1_combinerz:
  case Hexagon::SA1_combinezr:
    // Rdd 2-0, Rs 7-4
    operand = getDRegFromSubinstEncoding(inst & 0x7);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SS1_storeb_io:
    // Rs 7-4, u 11-8, Rt 3-0
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0xf00) >> 8;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SS1_storew_io:
    // Rs 7-4, u 11-8{4_2}, Rt 3-0
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0xf00) >> 8) << 2;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SS2_storebi0:
  case Hexagon::SS2_storebi1:
    // Rs 7-4, u 3-0
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = inst & 0xf;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SS2_storewi0:
  case Hexagon::SS2_storewi1:
    // Rs 7-4, u 3-0{4_2}
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = (inst & 0xf) << 2;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    break;
  case Hexagon::SS2_stored_sp:
    // s 8-3{6_3}, Rtt 2-0
    operand = SignExtend64<9>(((inst & 0x1f8) >> 3) << 3);
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    operand = getDRegFromSubinstEncoding(inst & 0x7);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SS2_storeh_io:
    // Rs 7-4, u 10-8{3_1}, Rt 3-0
    operand = getRegFromSubinstEncoding((inst & 0xf0) >> 4);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    operand = ((inst & 0x700) >> 8) << 1;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  case Hexagon::SS2_storew_sp:
    // u 8-4{5_2}, Rd 3-0
    operand = ((inst & 0x1f0) >> 4) << 2;
    HexagonMCInstrInfo::addConstant(*MI, operand, getContext());
    operand = getRegFromSubinstEncoding(inst & 0xf);
    Op = MCOperand::createReg(operand);
    MI->addOperand(Op);
    break;
  default:
    // don't crash with an invalid subinstruction
    // llvm_unreachable("Invalid subinstruction in duplex instruction");
    break;
  }
}
