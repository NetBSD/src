//===-- WebAssemblyAsmPrinter.cpp - WebAssembly LLVM assembly writer ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief This file contains a printer that converts from our internal
/// representation of machine-dependent LLVM code to the WebAssembly assembly
/// language.
///
//===----------------------------------------------------------------------===//

#include "InstPrinter/WebAssemblyInstPrinter.h"
#include "MCTargetDesc/WebAssemblyMCTargetDesc.h"
#include "MCTargetDesc/WebAssemblyTargetStreamer.h"
#include "WebAssembly.h"
#include "WebAssemblyMCInstLower.h"
#include "WebAssemblyMachineFunctionInfo.h"
#include "WebAssemblyRegisterInfo.h"
#include "WebAssemblySubtarget.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/Analysis.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

#define DEBUG_TYPE "asm-printer"

namespace {

class WebAssemblyAsmPrinter final : public AsmPrinter {
  const MachineRegisterInfo *MRI;
  WebAssemblyFunctionInfo *MFI;

public:
  WebAssemblyAsmPrinter(TargetMachine &TM, std::unique_ptr<MCStreamer> Streamer)
      : AsmPrinter(TM, std::move(Streamer)), MRI(nullptr), MFI(nullptr) {}

private:
  StringRef getPassName() const override {
    return "WebAssembly Assembly Printer";
  }

  //===------------------------------------------------------------------===//
  // MachineFunctionPass Implementation.
  //===------------------------------------------------------------------===//

  bool runOnMachineFunction(MachineFunction &MF) override {
    MRI = &MF.getRegInfo();
    MFI = MF.getInfo<WebAssemblyFunctionInfo>();
    return AsmPrinter::runOnMachineFunction(MF);
  }

  //===------------------------------------------------------------------===//
  // AsmPrinter Implementation.
  //===------------------------------------------------------------------===//

  void EmitEndOfAsmFile(Module &M) override;
  void EmitJumpTableInfo() override;
  void EmitConstantPool() override;
  void EmitFunctionBodyStart() override;
  void EmitFunctionBodyEnd() override;
  void EmitInstruction(const MachineInstr *MI) override;
  const MCExpr *lowerConstant(const Constant *CV) override;
  bool PrintAsmOperand(const MachineInstr *MI, unsigned OpNo,
                       unsigned AsmVariant, const char *ExtraCode,
                       raw_ostream &OS) override;
  bool PrintAsmMemoryOperand(const MachineInstr *MI, unsigned OpNo,
                             unsigned AsmVariant, const char *ExtraCode,
                             raw_ostream &OS) override;

  MVT getRegType(unsigned RegNo) const;
  std::string regToString(const MachineOperand &MO);
  WebAssemblyTargetStreamer *getTargetStreamer();
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
// Helpers.
//===----------------------------------------------------------------------===//

MVT WebAssemblyAsmPrinter::getRegType(unsigned RegNo) const {
  const TargetRegisterClass *TRC = MRI->getRegClass(RegNo);
  for (MVT T : {MVT::i32, MVT::i64, MVT::f32, MVT::f64, MVT::v16i8, MVT::v8i16,
                MVT::v4i32, MVT::v4f32})
    if (TRC->hasType(T))
      return T;
  DEBUG(errs() << "Unknown type for register number: " << RegNo);
  llvm_unreachable("Unknown register type");
  return MVT::Other;
}

std::string WebAssemblyAsmPrinter::regToString(const MachineOperand &MO) {
  unsigned RegNo = MO.getReg();
  assert(TargetRegisterInfo::isVirtualRegister(RegNo) &&
         "Unlowered physical register encountered during assembly printing");
  assert(!MFI->isVRegStackified(RegNo));
  unsigned WAReg = MFI->getWAReg(RegNo);
  assert(WAReg != WebAssemblyFunctionInfo::UnusedReg);
  return '$' + utostr(WAReg);
}

WebAssemblyTargetStreamer *WebAssemblyAsmPrinter::getTargetStreamer() {
  MCTargetStreamer *TS = OutStreamer->getTargetStreamer();
  return static_cast<WebAssemblyTargetStreamer *>(TS);
}

//===----------------------------------------------------------------------===//
// WebAssemblyAsmPrinter Implementation.
//===----------------------------------------------------------------------===//

void WebAssemblyAsmPrinter::EmitEndOfAsmFile(Module &M) {
  for (const auto &F : M) {
    // Emit function type info for all undefined functions
    if (F.isDeclarationForLinker() && !F.isIntrinsic()) {
      SmallVector<MVT, 4> Results;
      SmallVector<MVT, 4> Params;
      ComputeSignatureVTs(F, TM, Params, Results);
      getTargetStreamer()->emitIndirectFunctionType(F.getName(), Params,
                                                    Results);
    }
  }
  for (const auto &G : M.globals()) {
    if (!G.hasInitializer() && G.hasExternalLinkage()) {
      getTargetStreamer()->emitGlobalImport(G.getGlobalIdentifier());
    }
  }
}

void WebAssemblyAsmPrinter::EmitConstantPool() {
  assert(MF->getConstantPool()->getConstants().empty() &&
         "WebAssembly disables constant pools");
}

void WebAssemblyAsmPrinter::EmitJumpTableInfo() {
  // Nothing to do; jump tables are incorporated into the instruction stream.
}

void WebAssemblyAsmPrinter::EmitFunctionBodyStart() {
  if (!MFI->getParams().empty())
    getTargetStreamer()->emitParam(MFI->getParams());

  SmallVector<MVT, 4> ResultVTs;
  const Function &F(*MF->getFunction());

  // Emit the function index.
  if (MDNode *Idx = F.getMetadata("wasm.index")) {
    assert(Idx->getNumOperands() == 1);

    getTargetStreamer()->emitIndIdx(AsmPrinter::lowerConstant(
        cast<ConstantAsMetadata>(Idx->getOperand(0))->getValue()));
  }

  ComputeLegalValueVTs(F, TM, F.getReturnType(), ResultVTs);

  // If the return type needs to be legalized it will get converted into
  // passing a pointer.
  if (ResultVTs.size() == 1)
    getTargetStreamer()->emitResult(ResultVTs);

  // FIXME: When ExplicitLocals is enabled by default, we won't need
  // to define the locals here (and MFI can go back to being pointer-to-const).
  for (unsigned Idx = 0, IdxE = MRI->getNumVirtRegs(); Idx != IdxE; ++Idx) {
    unsigned VReg = TargetRegisterInfo::index2VirtReg(Idx);
    unsigned WAReg = MFI->getWAReg(VReg);
    // Don't declare unused registers.
    if (WAReg == WebAssemblyFunctionInfo::UnusedReg)
      continue;
    // Don't redeclare parameters.
    if (WAReg < MFI->getParams().size())
      continue;
    // Don't declare stackified registers.
    if (int(WAReg) < 0)
      continue;
    MFI->addLocal(getRegType(VReg));
  }

  getTargetStreamer()->emitLocal(MFI->getLocals());

  AsmPrinter::EmitFunctionBodyStart();
}

void WebAssemblyAsmPrinter::EmitFunctionBodyEnd() {
  getTargetStreamer()->emitEndFunc();
}

void WebAssemblyAsmPrinter::EmitInstruction(const MachineInstr *MI) {
  DEBUG(dbgs() << "EmitInstruction: " << *MI << '\n');

  switch (MI->getOpcode()) {
  case WebAssembly::ARGUMENT_I32:
  case WebAssembly::ARGUMENT_I64:
  case WebAssembly::ARGUMENT_F32:
  case WebAssembly::ARGUMENT_F64:
  case WebAssembly::ARGUMENT_v16i8:
  case WebAssembly::ARGUMENT_v8i16:
  case WebAssembly::ARGUMENT_v4i32:
  case WebAssembly::ARGUMENT_v4f32:
    // These represent values which are live into the function entry, so there's
    // no instruction to emit.
    break;
  case WebAssembly::FALLTHROUGH_RETURN_I32:
  case WebAssembly::FALLTHROUGH_RETURN_I64:
  case WebAssembly::FALLTHROUGH_RETURN_F32:
  case WebAssembly::FALLTHROUGH_RETURN_F64:
  case WebAssembly::FALLTHROUGH_RETURN_v16i8:
  case WebAssembly::FALLTHROUGH_RETURN_v8i16:
  case WebAssembly::FALLTHROUGH_RETURN_v4i32:
  case WebAssembly::FALLTHROUGH_RETURN_v4f32: {
    // These instructions represent the implicit return at the end of a
    // function body. The operand is always a pop.
    assert(MFI->isVRegStackified(MI->getOperand(0).getReg()));

    if (isVerbose()) {
      OutStreamer->AddComment("fallthrough-return: $pop" +
                              utostr(MFI->getWARegStackId(
                                  MFI->getWAReg(MI->getOperand(0).getReg()))));
      OutStreamer->AddBlankLine();
    }
    break;
  }
  case WebAssembly::FALLTHROUGH_RETURN_VOID:
    // This instruction represents the implicit return at the end of a
    // function body with no return value.
    if (isVerbose()) {
      OutStreamer->AddComment("fallthrough-return");
      OutStreamer->AddBlankLine();
    }
    break;
  default: {
    WebAssemblyMCInstLower MCInstLowering(OutContext, *this);
    MCInst TmpInst;
    MCInstLowering.Lower(MI, TmpInst);
    EmitToStreamer(*OutStreamer, TmpInst);
    break;
  }
  }
}

const MCExpr *WebAssemblyAsmPrinter::lowerConstant(const Constant *CV) {
  if (const GlobalValue *GV = dyn_cast<GlobalValue>(CV))
    if (GV->getValueType()->isFunctionTy())
      return MCSymbolRefExpr::create(
          getSymbol(GV), MCSymbolRefExpr::VK_WebAssembly_FUNCTION, OutContext);
  return AsmPrinter::lowerConstant(CV);
}

bool WebAssemblyAsmPrinter::PrintAsmOperand(const MachineInstr *MI,
                                            unsigned OpNo, unsigned AsmVariant,
                                            const char *ExtraCode,
                                            raw_ostream &OS) {
  if (AsmVariant != 0)
    report_fatal_error("There are no defined alternate asm variants");

  // First try the generic code, which knows about modifiers like 'c' and 'n'.
  if (!AsmPrinter::PrintAsmOperand(MI, OpNo, AsmVariant, ExtraCode, OS))
    return false;

  if (!ExtraCode) {
    const MachineOperand &MO = MI->getOperand(OpNo);
    switch (MO.getType()) {
    case MachineOperand::MO_Immediate:
      OS << MO.getImm();
      return false;
    case MachineOperand::MO_Register:
      OS << regToString(MO);
      return false;
    case MachineOperand::MO_GlobalAddress:
      getSymbol(MO.getGlobal())->print(OS, MAI);
      printOffset(MO.getOffset(), OS);
      return false;
    case MachineOperand::MO_ExternalSymbol:
      GetExternalSymbolSymbol(MO.getSymbolName())->print(OS, MAI);
      printOffset(MO.getOffset(), OS);
      return false;
    case MachineOperand::MO_MachineBasicBlock:
      MO.getMBB()->getSymbol()->print(OS, MAI);
      return false;
    default:
      break;
    }
  }

  return true;
}

bool WebAssemblyAsmPrinter::PrintAsmMemoryOperand(const MachineInstr *MI,
                                                  unsigned OpNo,
                                                  unsigned AsmVariant,
                                                  const char *ExtraCode,
                                                  raw_ostream &OS) {
  if (AsmVariant != 0)
    report_fatal_error("There are no defined alternate asm variants");

  if (!ExtraCode) {
    // TODO: For now, we just hard-code 0 as the constant offset; teach
    // SelectInlineAsmMemoryOperand how to do address mode matching.
    OS << "0(" + regToString(MI->getOperand(OpNo)) + ')';
    return false;
  }

  return AsmPrinter::PrintAsmMemoryOperand(MI, OpNo, AsmVariant, ExtraCode, OS);
}

// Force static initialization.
extern "C" void LLVMInitializeWebAssemblyAsmPrinter() {
  RegisterAsmPrinter<WebAssemblyAsmPrinter> X(getTheWebAssemblyTarget32());
  RegisterAsmPrinter<WebAssemblyAsmPrinter> Y(getTheWebAssemblyTarget64());
}
