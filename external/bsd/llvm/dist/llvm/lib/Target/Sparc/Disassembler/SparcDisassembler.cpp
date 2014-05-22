//===- SparcDisassembler.cpp - Disassembler for Sparc -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is part of the Sparc Disassembler.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "sparc-disassembler"

#include "Sparc.h"
#include "SparcRegisterInfo.h"
#include "SparcSubtarget.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCFixedLenDisassembler.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Support/TargetRegistry.h"

using namespace llvm;

typedef MCDisassembler::DecodeStatus DecodeStatus;

namespace {

/// SparcDisassembler - a disassembler class for Sparc.
class SparcDisassembler : public MCDisassembler {
public:
  /// Constructor     - Initializes the disassembler.
  ///
  SparcDisassembler(const MCSubtargetInfo &STI, const MCRegisterInfo *Info) :
    MCDisassembler(STI), RegInfo(Info)
  {}
  virtual ~SparcDisassembler() {}

  const MCRegisterInfo *getRegInfo() const { return RegInfo.get(); }

  /// getInstruction - See MCDisassembler.
  virtual DecodeStatus getInstruction(MCInst &instr,
                                      uint64_t &size,
                                      const MemoryObject &region,
                                      uint64_t address,
                                      raw_ostream &vStream,
                                      raw_ostream &cStream) const;
private:
  OwningPtr<const MCRegisterInfo> RegInfo;
};

}

namespace llvm {
  extern Target TheSparcTarget, TheSparcV9Target;
}

static MCDisassembler *createSparcDisassembler(
                       const Target &T,
                       const MCSubtargetInfo &STI) {
  return new SparcDisassembler(STI, T.createMCRegInfo(""));
}


extern "C" void LLVMInitializeSparcDisassembler() {
  // Register the disassembler.
  TargetRegistry::RegisterMCDisassembler(TheSparcTarget,
                                         createSparcDisassembler);
  TargetRegistry::RegisterMCDisassembler(TheSparcV9Target,
                                         createSparcDisassembler);
}



static const unsigned IntRegDecoderTable[] = {
  SP::G0,  SP::G1,  SP::G2,  SP::G3,
  SP::G4,  SP::G5,  SP::G6,  SP::G7,
  SP::O0,  SP::O1,  SP::O2,  SP::O3,
  SP::O4,  SP::O5,  SP::O6,  SP::O7,
  SP::L0,  SP::L1,  SP::L2,  SP::L3,
  SP::L4,  SP::L5,  SP::L6,  SP::L7,
  SP::I0,  SP::I1,  SP::I2,  SP::I3,
  SP::I4,  SP::I5,  SP::I6,  SP::I7 };

static const unsigned FPRegDecoderTable[] = {
  SP::F0,   SP::F1,   SP::F2,   SP::F3,
  SP::F4,   SP::F5,   SP::F6,   SP::F7,
  SP::F8,   SP::F9,   SP::F10,  SP::F11,
  SP::F12,  SP::F13,  SP::F14,  SP::F15,
  SP::F16,  SP::F17,  SP::F18,  SP::F19,
  SP::F20,  SP::F21,  SP::F22,  SP::F23,
  SP::F24,  SP::F25,  SP::F26,  SP::F27,
  SP::F28,  SP::F29,  SP::F30,  SP::F31 };

static const unsigned DFPRegDecoderTable[] = {
  SP::D0,   SP::D16,  SP::D1,   SP::D17,
  SP::D2,   SP::D18,  SP::D3,   SP::D19,
  SP::D4,   SP::D20,  SP::D5,   SP::D21,
  SP::D6,   SP::D22,  SP::D7,   SP::D23,
  SP::D8,   SP::D24,  SP::D9,   SP::D25,
  SP::D10,  SP::D26,  SP::D11,  SP::D27,
  SP::D12,  SP::D28,  SP::D13,  SP::D29,
  SP::D14,  SP::D30,  SP::D15,  SP::D31 };

static const unsigned QFPRegDecoderTable[] = {
  SP::Q0,  SP::Q8,   ~0U,  ~0U,
  SP::Q1,  SP::Q9,   ~0U,  ~0U,
  SP::Q2,  SP::Q10,  ~0U,  ~0U,
  SP::Q3,  SP::Q11,  ~0U,  ~0U,
  SP::Q4,  SP::Q12,  ~0U,  ~0U,
  SP::Q5,  SP::Q13,  ~0U,  ~0U,
  SP::Q6,  SP::Q14,  ~0U,  ~0U,
  SP::Q7,  SP::Q15,  ~0U,  ~0U } ;

static DecodeStatus DecodeIntRegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = IntRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}

static DecodeStatus DecodeI64RegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = IntRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}


static DecodeStatus DecodeFPRegsRegisterClass(MCInst &Inst,
                                              unsigned RegNo,
                                              uint64_t Address,
                                              const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = FPRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}


static DecodeStatus DecodeDFPRegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;
  unsigned Reg = DFPRegDecoderTable[RegNo];
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}


static DecodeStatus DecodeQFPRegsRegisterClass(MCInst &Inst,
                                               unsigned RegNo,
                                               uint64_t Address,
                                               const void *Decoder) {
  if (RegNo > 31)
    return MCDisassembler::Fail;

  unsigned Reg = QFPRegDecoderTable[RegNo];
  if (Reg == ~0U)
    return MCDisassembler::Fail;
  Inst.addOperand(MCOperand::CreateReg(Reg));
  return MCDisassembler::Success;
}


#include "SparcGenDisassemblerTables.inc"

/// readInstruction - read four bytes from the MemoryObject
/// and return 32 bit word.
static DecodeStatus readInstruction32(const MemoryObject &region,
                                      uint64_t address,
                                      uint64_t &size,
                                      uint32_t &insn) {
  uint8_t Bytes[4];

  // We want to read exactly 4 Bytes of data.
  if (region.readBytes(address, 4, Bytes) == -1) {
    size = 0;
    return MCDisassembler::Fail;
  }

  // Encoded as a big-endian 32-bit word in the stream.
  insn = (Bytes[3] <<  0) |
    (Bytes[2] <<  8) |
    (Bytes[1] << 16) |
    (Bytes[0] << 24);

  return MCDisassembler::Success;
}


DecodeStatus
SparcDisassembler::getInstruction(MCInst &instr,
                                 uint64_t &Size,
                                 const MemoryObject &Region,
                                 uint64_t Address,
                                 raw_ostream &vStream,
                                 raw_ostream &cStream) const {
  uint32_t Insn;

  DecodeStatus Result = readInstruction32(Region, Address, Size, Insn);
  if (Result == MCDisassembler::Fail)
    return MCDisassembler::Fail;


  // Calling the auto-generated decoder function.
  Result = decodeInstruction(DecoderTableSparc32, instr, Insn, Address,
                             this, STI);

  if (Result != MCDisassembler::Fail) {
    Size = 4;
    return Result;
  }

  return MCDisassembler::Fail;
}
