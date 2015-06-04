//===-- llvm/CodeGen/DwarfCompileUnit.cpp - Dwarf Compile Unit ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for constructing a dwarf compile unit.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dwarfdebug"

#include "DwarfCompileUnit.h"
#include "DwarfAccelTable.h"
#include "DwarfDebug.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/DIBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/MC/MCSection.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/Mangler.h"
#include "llvm/Target/TargetFrameLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

/// CompileUnit - Compile unit constructor.
CompileUnit::CompileUnit(unsigned UID, DIE *D, DICompileUnit Node,
                         AsmPrinter *A, DwarfDebug *DW, DwarfUnits *DWU)
    : UniqueID(UID), Node(Node), CUDie(D), Asm(A), DD(DW), DU(DWU),
      IndexTyDie(0), DebugInfoOffset(0) {
  DIEIntegerOne = new (DIEValueAllocator) DIEInteger(1);
  insertDIE(Node, D);
}

/// ~CompileUnit - Destructor for compile unit.
CompileUnit::~CompileUnit() {
  for (unsigned j = 0, M = DIEBlocks.size(); j < M; ++j)
    DIEBlocks[j]->~DIEBlock();
}

/// createDIEEntry - Creates a new DIEEntry to be a proxy for a debug
/// information entry.
DIEEntry *CompileUnit::createDIEEntry(DIE *Entry) {
  DIEEntry *Value = new (DIEValueAllocator) DIEEntry(Entry);
  return Value;
}

/// getDefaultLowerBound - Return the default lower bound for an array. If the
/// DWARF version doesn't handle the language, return -1.
int64_t CompileUnit::getDefaultLowerBound() const {
  switch (getLanguage()) {
  default:
    break;

  case dwarf::DW_LANG_C89:
  case dwarf::DW_LANG_C99:
  case dwarf::DW_LANG_C:
  case dwarf::DW_LANG_C_plus_plus:
  case dwarf::DW_LANG_ObjC:
  case dwarf::DW_LANG_ObjC_plus_plus:
    return 0;

  case dwarf::DW_LANG_Fortran77:
  case dwarf::DW_LANG_Fortran90:
  case dwarf::DW_LANG_Fortran95:
    return 1;

  // The languages below have valid values only if the DWARF version >= 4.
  case dwarf::DW_LANG_Java:
  case dwarf::DW_LANG_Python:
  case dwarf::DW_LANG_UPC:
  case dwarf::DW_LANG_D:
    if (dwarf::DWARF_VERSION >= 4)
      return 0;
    break;

  case dwarf::DW_LANG_Ada83:
  case dwarf::DW_LANG_Ada95:
  case dwarf::DW_LANG_Cobol74:
  case dwarf::DW_LANG_Cobol85:
  case dwarf::DW_LANG_Modula2:
  case dwarf::DW_LANG_Pascal83:
  case dwarf::DW_LANG_PLI:
    if (dwarf::DWARF_VERSION >= 4)
      return 1;
    break;
  }

  return -1;
}

/// Check whether the DIE for this MDNode can be shared across CUs.
static bool isShareableAcrossCUs(DIDescriptor D) {
  // When the MDNode can be part of the type system, the DIE can be
  // shared across CUs.
  return D.isType() ||
         (D.isSubprogram() && !DISubprogram(D).isDefinition());
}

/// getDIE - Returns the debug information entry map slot for the
/// specified debug variable. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
DIE *CompileUnit::getDIE(DIDescriptor D) const {
  if (isShareableAcrossCUs(D))
    return DD->getDIE(D);
  return MDNodeToDieMap.lookup(D);
}

/// insertDIE - Insert DIE into the map. We delegate the request to DwarfDebug
/// when the DIE for this MDNode can be shared across CUs. The mappings
/// will be kept in DwarfDebug for shareable DIEs.
void CompileUnit::insertDIE(DIDescriptor Desc, DIE *D) {
  if (isShareableAcrossCUs(Desc)) {
    DD->insertDIE(Desc, D);
    return;
  }
  MDNodeToDieMap.insert(std::make_pair(Desc, D));
}

/// addFlag - Add a flag that is true.
void CompileUnit::addFlag(DIE *Die, dwarf::Attribute Attribute) {
  if (DD->getDwarfVersion() >= 4)
    Die->addValue(Attribute, dwarf::DW_FORM_flag_present, DIEIntegerOne);
  else
    Die->addValue(Attribute, dwarf::DW_FORM_flag, DIEIntegerOne);
}

/// addUInt - Add an unsigned integer attribute data and value.
///
void CompileUnit::addUInt(DIE *Die, dwarf::Attribute Attribute,
                          Optional<dwarf::Form> Form, uint64_t Integer) {
  if (!Form)
    Form = DIEInteger::BestForm(false, Integer);
  DIEValue *Value = Integer == 1 ? DIEIntegerOne : new (DIEValueAllocator)
                        DIEInteger(Integer);
  Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addUInt(DIEBlock *Block, dwarf::Form Form, uint64_t Integer) {
  addUInt(Block, (dwarf::Attribute)0, Form, Integer);
}

/// addSInt - Add an signed integer attribute data and value.
///
void CompileUnit::addSInt(DIE *Die, dwarf::Attribute Attribute,
                          Optional<dwarf::Form> Form, int64_t Integer) {
  if (!Form)
    Form = DIEInteger::BestForm(true, Integer);
  DIEValue *Value = new (DIEValueAllocator) DIEInteger(Integer);
  Die->addValue(Attribute, *Form, Value);
}

void CompileUnit::addSInt(DIEBlock *Die, Optional<dwarf::Form> Form,
                          int64_t Integer) {
  addSInt(Die, (dwarf::Attribute)0, Form, Integer);
}

/// addString - Add a string attribute data and value. We always emit a
/// reference to the string pool instead of immediate strings so that DIEs have
/// more predictable sizes. In the case of split dwarf we emit an index
/// into another table which gets us the static offset into the string
/// table.
void CompileUnit::addString(DIE *Die, dwarf::Attribute Attribute,
                            StringRef String) {
  DIEValue *Value;
  dwarf::Form Form;
  if (!DD->useSplitDwarf()) {
    MCSymbol *Symb = DU->getStringPoolEntry(String);
    if (Asm->needsRelocationsForDwarfStringPool())
      Value = new (DIEValueAllocator) DIELabel(Symb);
    else {
      MCSymbol *StringPool = DU->getStringPoolSym();
      Value = new (DIEValueAllocator) DIEDelta(Symb, StringPool);
    }
    Form = dwarf::DW_FORM_strp;
  } else {
    unsigned idx = DU->getStringPoolIndex(String);
    Value = new (DIEValueAllocator) DIEInteger(idx);
    Form = dwarf::DW_FORM_GNU_str_index;
  }
  DIEValue *Str = new (DIEValueAllocator) DIEString(Value, String);
  Die->addValue(Attribute, Form, Str);
}

/// addLocalString - Add a string attribute data and value. This is guaranteed
/// to be in the local string pool instead of indirected.
void CompileUnit::addLocalString(DIE *Die, dwarf::Attribute Attribute,
                                 StringRef String) {
  MCSymbol *Symb = DU->getStringPoolEntry(String);
  DIEValue *Value;
  if (Asm->needsRelocationsForDwarfStringPool())
    Value = new (DIEValueAllocator) DIELabel(Symb);
  else {
    MCSymbol *StringPool = DU->getStringPoolSym();
    Value = new (DIEValueAllocator) DIEDelta(Symb, StringPool);
  }
  Die->addValue(Attribute, dwarf::DW_FORM_strp, Value);
}

/// addExpr - Add a Dwarf expression attribute data and value.
///
void CompileUnit::addExpr(DIEBlock *Die, dwarf::Form Form, const MCExpr *Expr) {
  DIEValue *Value = new (DIEValueAllocator) DIEExpr(Expr);
  Die->addValue((dwarf::Attribute)0, Form, Value);
}

/// addLabel - Add a Dwarf label attribute data and value.
///
void CompileUnit::addLabel(DIE *Die, dwarf::Attribute Attribute,
                           dwarf::Form Form, const MCSymbol *Label) {
  DIEValue *Value = new (DIEValueAllocator) DIELabel(Label);
  Die->addValue(Attribute, Form, Value);
}

void CompileUnit::addLabel(DIEBlock *Die, dwarf::Form Form,
                           const MCSymbol *Label) {
  addLabel(Die, (dwarf::Attribute)0, Form, Label);
}

/// addLabelAddress - Add a dwarf label attribute data and value using
/// DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addLabelAddress(DIE *Die, dwarf::Attribute Attribute,
                                  MCSymbol *Label) {
  if (Label)
    DD->addArangeLabel(SymbolCU(this, Label));

  if (!DD->useSplitDwarf()) {
    if (Label != NULL) {
      DIEValue *Value = new (DIEValueAllocator) DIELabel(Label);
      Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
    } else {
      DIEValue *Value = new (DIEValueAllocator) DIEInteger(0);
      Die->addValue(Attribute, dwarf::DW_FORM_addr, Value);
    }
  } else {
    unsigned idx = DU->getAddrPoolIndex(Label);
    DIEValue *Value = new (DIEValueAllocator) DIEInteger(idx);
    Die->addValue(Attribute, dwarf::DW_FORM_GNU_addr_index, Value);
  }
}

/// addOpAddress - Add a dwarf op address data and value using the
/// form given and an op of either DW_FORM_addr or DW_FORM_GNU_addr_index.
///
void CompileUnit::addOpAddress(DIEBlock *Die, const MCSymbol *Sym) {
  DD->addArangeLabel(SymbolCU(this, Sym));
  if (!DD->useSplitDwarf()) {
    addUInt(Die, dwarf::DW_FORM_data1, dwarf::DW_OP_addr);
    addLabel(Die, dwarf::DW_FORM_udata, Sym);
  } else {
    addUInt(Die, dwarf::DW_FORM_data1, dwarf::DW_OP_GNU_addr_index);
    addUInt(Die, dwarf::DW_FORM_GNU_addr_index, DU->getAddrPoolIndex(Sym));
  }
}

/// addDelta - Add a label delta attribute data and value.
///
void CompileUnit::addDelta(DIE *Die, dwarf::Attribute Attribute,
                           dwarf::Form Form, const MCSymbol *Hi,
                           const MCSymbol *Lo) {
  DIEValue *Value = new (DIEValueAllocator) DIEDelta(Hi, Lo);
  Die->addValue(Attribute, Form, Value);
}

/// addDIEEntry - Add a DIE attribute data and value.
///
void CompileUnit::addDIEEntry(DIE *Die, dwarf::Attribute Attribute,
                              DIE *Entry) {
  addDIEEntry(Die, Attribute, createDIEEntry(Entry));
}

void CompileUnit::addDIEEntry(DIE *Die, dwarf::Attribute Attribute,
                              DIEEntry *Entry) {
  const DIE *DieCU = Die->getCompileUnitOrNull();
  const DIE *EntryCU = Entry->getEntry()->getCompileUnitOrNull();
  if (!DieCU)
    // We assume that Die belongs to this CU, if it is not linked to any CU yet.
    DieCU = getCUDie();
  if (!EntryCU)
    EntryCU = getCUDie();
  Die->addValue(Attribute, EntryCU == DieCU ? dwarf::DW_FORM_ref4
                                            : dwarf::DW_FORM_ref_addr,
                Entry);
}

/// Create a DIE with the given Tag, add the DIE to its parent, and
/// call insertDIE if MD is not null.
DIE *CompileUnit::createAndAddDIE(unsigned Tag, DIE &Parent, DIDescriptor N) {
  DIE *Die = new DIE(Tag);
  Parent.addChild(Die);
  if (N)
    insertDIE(N, Die);
  return Die;
}

/// addBlock - Add block data.
///
void CompileUnit::addBlock(DIE *Die, dwarf::Attribute Attribute,
                           DIEBlock *Block) {
  Block->ComputeSize(Asm);
  DIEBlocks.push_back(Block); // Memoize so we can call the destructor later on.
  Die->addValue(Attribute, Block->BestForm(), Block);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIVariable V) {
  // Verify variable.
  if (!V.isVariable())
    return;

  unsigned Line = V.getLineNumber();
  if (Line == 0)
    return;
  unsigned FileID =
      DD->getOrCreateSourceID(V.getContext().getFilename(),
                              V.getContext().getDirectory(), getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIGlobalVariable G) {
  // Verify global variable.
  if (!G.isGlobalVariable())
    return;

  unsigned Line = G.getLineNumber();
  if (Line == 0)
    return;
  unsigned FileID =
      DD->getOrCreateSourceID(G.getFilename(), G.getDirectory(), getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DISubprogram SP) {
  // Verify subprogram.
  if (!SP.isSubprogram())
    return;

  // If the line number is 0, don't add it.
  unsigned Line = SP.getLineNumber();
  if (Line == 0)
    return;

  unsigned FileID = DD->getOrCreateSourceID(SP.getFilename(), SP.getDirectory(),
                                            getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIType Ty) {
  // Verify type.
  if (!Ty.isType())
    return;

  unsigned Line = Ty.getLineNumber();
  if (Line == 0)
    return;
  unsigned FileID = DD->getOrCreateSourceID(Ty.getFilename(), Ty.getDirectory(),
                                            getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DIObjCProperty Ty) {
  // Verify type.
  if (!Ty.isObjCProperty())
    return;

  unsigned Line = Ty.getLineNumber();
  if (Line == 0)
    return;
  DIFile File = Ty.getFile();
  unsigned FileID = DD->getOrCreateSourceID(File.getFilename(),
                                            File.getDirectory(), getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addSourceLine - Add location information to specified debug information
/// entry.
void CompileUnit::addSourceLine(DIE *Die, DINameSpace NS) {
  // Verify namespace.
  if (!NS.Verify())
    return;

  unsigned Line = NS.getLineNumber();
  if (Line == 0)
    return;
  StringRef FN = NS.getFilename();

  unsigned FileID =
      DD->getOrCreateSourceID(FN, NS.getDirectory(), getUniqueID());
  assert(FileID && "Invalid file id");
  addUInt(Die, dwarf::DW_AT_decl_file, None, FileID);
  addUInt(Die, dwarf::DW_AT_decl_line, None, Line);
}

/// addVariableAddress - Add DW_AT_location attribute for a
/// DbgVariable based on provided MachineLocation.
void CompileUnit::addVariableAddress(const DbgVariable &DV, DIE *Die,
                                     MachineLocation Location) {
  if (DV.variableHasComplexAddress())
    addComplexAddress(DV, Die, dwarf::DW_AT_location, Location);
  else if (DV.isBlockByrefVariable())
    addBlockByrefAddress(DV, Die, dwarf::DW_AT_location, Location);
  else
    addAddress(Die, dwarf::DW_AT_location, Location,
               DV.getVariable().isIndirect());
}

/// addRegisterOp - Add register operand.
void CompileUnit::addRegisterOp(DIEBlock *TheDie, unsigned Reg) {
  const TargetRegisterInfo *RI = Asm->TM.getRegisterInfo();
  unsigned DWReg = RI->getDwarfRegNum(Reg, false);
  if (DWReg < 32)
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_reg0 + DWReg);
  else {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_regx);
    addUInt(TheDie, dwarf::DW_FORM_udata, DWReg);
  }
}

/// addRegisterOffset - Add register offset.
void CompileUnit::addRegisterOffset(DIEBlock *TheDie, unsigned Reg,
                                    int64_t Offset) {
  const TargetRegisterInfo *RI = Asm->TM.getRegisterInfo();
  unsigned DWReg = RI->getDwarfRegNum(Reg, false);
  const TargetRegisterInfo *TRI = Asm->TM.getRegisterInfo();
  if (Reg == TRI->getFrameRegister(*Asm->MF))
    // If variable offset is based in frame register then use fbreg.
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_fbreg);
  else if (DWReg < 32)
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_breg0 + DWReg);
  else {
    addUInt(TheDie, dwarf::DW_FORM_data1, dwarf::DW_OP_bregx);
    addUInt(TheDie, dwarf::DW_FORM_udata, DWReg);
  }
  addSInt(TheDie, dwarf::DW_FORM_sdata, Offset);
}

/// addAddress - Add an address attribute to a die based on the location
/// provided.
void CompileUnit::addAddress(DIE *Die, dwarf::Attribute Attribute,
                             const MachineLocation &Location, bool Indirect) {
  DIEBlock *Block = new (DIEValueAllocator) DIEBlock();

  if (Location.isReg() && !Indirect)
    addRegisterOp(Block, Location.getReg());
  else {
    addRegisterOffset(Block, Location.getReg(), Location.getOffset());
    if (Indirect && !Location.isReg()) {
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    }
  }

  // Now attach the location information to the DIE.
  addBlock(Die, Attribute, Block);
}

/// addComplexAddress - Start with the address based on the location provided,
/// and generate the DWARF information necessary to find the actual variable
/// given the extra address information encoded in the DIVariable, starting from
/// the starting location.  Add the DWARF information to the die.
///
void CompileUnit::addComplexAddress(const DbgVariable &DV, DIE *Die,
                                    dwarf::Attribute Attribute,
                                    const MachineLocation &Location) {
  DIEBlock *Block = new (DIEValueAllocator) DIEBlock();
  unsigned N = DV.getNumAddrElements();
  unsigned i = 0;
  if (Location.isReg()) {
    if (N >= 2 && DV.getAddrElement(0) == DIBuilder::OpPlus) {
      // If first address element is OpPlus then emit
      // DW_OP_breg + Offset instead of DW_OP_reg + Offset.
      addRegisterOffset(Block, Location.getReg(), DV.getAddrElement(1));
      i = 2;
    } else
      addRegisterOp(Block, Location.getReg());
  } else
    addRegisterOffset(Block, Location.getReg(), Location.getOffset());

  for (; i < N; ++i) {
    uint64_t Element = DV.getAddrElement(i);
    if (Element == DIBuilder::OpPlus) {
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
      addUInt(Block, dwarf::DW_FORM_udata, DV.getAddrElement(++i));
    } else if (Element == DIBuilder::OpDeref) {
      if (!Location.isReg())
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    } else
      llvm_unreachable("unknown DIBuilder Opcode");
  }

  // Now attach the location information to the DIE.
  addBlock(Die, Attribute, Block);
}

/* Byref variables, in Blocks, are declared by the programmer as "SomeType
   VarName;", but the compiler creates a __Block_byref_x_VarName struct, and
   gives the variable VarName either the struct, or a pointer to the struct, as
   its type.  This is necessary for various behind-the-scenes things the
   compiler needs to do with by-reference variables in Blocks.

   However, as far as the original *programmer* is concerned, the variable
   should still have type 'SomeType', as originally declared.

   The function getBlockByrefType dives into the __Block_byref_x_VarName
   struct to find the original type of the variable, which is then assigned to
   the variable's Debug Information Entry as its real type.  So far, so good.
   However now the debugger will expect the variable VarName to have the type
   SomeType.  So we need the location attribute for the variable to be an
   expression that explains to the debugger how to navigate through the
   pointers and struct to find the actual variable of type SomeType.

   The following function does just that.  We start by getting
   the "normal" location for the variable. This will be the location
   of either the struct __Block_byref_x_VarName or the pointer to the
   struct __Block_byref_x_VarName.

   The struct will look something like:

   struct __Block_byref_x_VarName {
     ... <various fields>
     struct __Block_byref_x_VarName *forwarding;
     ... <various other fields>
     SomeType VarName;
     ... <maybe more fields>
   };

   If we are given the struct directly (as our starting point) we
   need to tell the debugger to:

   1).  Add the offset of the forwarding field.

   2).  Follow that pointer to get the real __Block_byref_x_VarName
   struct to use (the real one may have been copied onto the heap).

   3).  Add the offset for the field VarName, to find the actual variable.

   If we started with a pointer to the struct, then we need to
   dereference that pointer first, before the other steps.
   Translating this into DWARF ops, we will need to append the following
   to the current location description for the variable:

   DW_OP_deref                    -- optional, if we start with a pointer
   DW_OP_plus_uconst <forward_fld_offset>
   DW_OP_deref
   DW_OP_plus_uconst <varName_fld_offset>

   That is what this function does.  */

/// addBlockByrefAddress - Start with the address based on the location
/// provided, and generate the DWARF information necessary to find the
/// actual Block variable (navigating the Block struct) based on the
/// starting location.  Add the DWARF information to the die.  For
/// more information, read large comment just above here.
///
void CompileUnit::addBlockByrefAddress(const DbgVariable &DV, DIE *Die,
                                       dwarf::Attribute Attribute,
                                       const MachineLocation &Location) {
  DIType Ty = DV.getType();
  DIType TmpTy = Ty;
  uint16_t Tag = Ty.getTag();
  bool isPointer = false;

  StringRef varName = DV.getName();

  if (Tag == dwarf::DW_TAG_pointer_type) {
    DIDerivedType DTy(Ty);
    TmpTy = resolve(DTy.getTypeDerivedFrom());
    isPointer = true;
  }

  DICompositeType blockStruct(TmpTy);

  // Find the __forwarding field and the variable field in the __Block_byref
  // struct.
  DIArray Fields = blockStruct.getTypeArray();
  DIDerivedType varField;
  DIDerivedType forwardingField;

  for (unsigned i = 0, N = Fields.getNumElements(); i < N; ++i) {
    DIDerivedType DT(Fields.getElement(i));
    StringRef fieldName = DT.getName();
    if (fieldName == "__forwarding")
      forwardingField = DT;
    else if (fieldName == varName)
      varField = DT;
  }

  // Get the offsets for the forwarding field and the variable field.
  unsigned forwardingFieldOffset = forwardingField.getOffsetInBits() >> 3;
  unsigned varFieldOffset = varField.getOffsetInBits() >> 2;

  // Decode the original location, and use that as the start of the byref
  // variable's location.
  DIEBlock *Block = new (DIEValueAllocator) DIEBlock();

  if (Location.isReg())
    addRegisterOp(Block, Location.getReg());
  else
    addRegisterOffset(Block, Location.getReg(), Location.getOffset());

  // If we started with a pointer to the __Block_byref... struct, then
  // the first thing we need to do is dereference the pointer (DW_OP_deref).
  if (isPointer)
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);

  // Next add the offset for the '__forwarding' field:
  // DW_OP_plus_uconst ForwardingFieldOffset.  Note there's no point in
  // adding the offset if it's 0.
  if (forwardingFieldOffset > 0) {
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
    addUInt(Block, dwarf::DW_FORM_udata, forwardingFieldOffset);
  }

  // Now dereference the __forwarding field to get to the real __Block_byref
  // struct:  DW_OP_deref.
  addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);

  // Now that we've got the real __Block_byref... struct, add the offset
  // for the variable's field to get to the location of the actual variable:
  // DW_OP_plus_uconst varFieldOffset.  Again, don't add if it's 0.
  if (varFieldOffset > 0) {
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);
    addUInt(Block, dwarf::DW_FORM_udata, varFieldOffset);
  }

  // Now attach the location information to the DIE.
  addBlock(Die, Attribute, Block);
}

/// isTypeSigned - Return true if the type is signed.
static bool isTypeSigned(DwarfDebug *DD, DIType Ty, int *SizeInBits) {
  if (Ty.isDerivedType())
    return isTypeSigned(DD, DD->resolve(DIDerivedType(Ty).getTypeDerivedFrom()),
                        SizeInBits);
  if (Ty.isBasicType())
    if (DIBasicType(Ty).getEncoding() == dwarf::DW_ATE_signed ||
        DIBasicType(Ty).getEncoding() == dwarf::DW_ATE_signed_char) {
      *SizeInBits = Ty.getSizeInBits();
      return true;
    }
  return false;
}

/// Return true if type encoding is unsigned.
static bool isUnsignedDIType(DwarfDebug *DD, DIType Ty) {
  DIDerivedType DTy(Ty);
  if (DTy.isDerivedType())
    return isUnsignedDIType(DD, DD->resolve(DTy.getTypeDerivedFrom()));

  DIBasicType BTy(Ty);
  if (BTy.isBasicType()) {
    unsigned Encoding = BTy.getEncoding();
    if (Encoding == dwarf::DW_ATE_unsigned ||
        Encoding == dwarf::DW_ATE_unsigned_char ||
        Encoding == dwarf::DW_ATE_boolean)
      return true;
  }
  return false;
}

/// If this type is derived from a base type then return base type size.
static uint64_t getBaseTypeSize(DwarfDebug *DD, DIDerivedType Ty) {
  unsigned Tag = Ty.getTag();

  if (Tag != dwarf::DW_TAG_member && Tag != dwarf::DW_TAG_typedef &&
      Tag != dwarf::DW_TAG_const_type && Tag != dwarf::DW_TAG_volatile_type &&
      Tag != dwarf::DW_TAG_restrict_type)
    return Ty.getSizeInBits();

  DIType BaseType = DD->resolve(Ty.getTypeDerivedFrom());

  // If this type is not derived from any type then take conservative approach.
  if (!BaseType.isValid())
    return Ty.getSizeInBits();

  // If this is a derived type, go ahead and get the base type, unless it's a
  // reference then it's just the size of the field. Pointer types have no need
  // of this since they're a different type of qualification on the type.
  if (BaseType.getTag() == dwarf::DW_TAG_reference_type ||
      BaseType.getTag() == dwarf::DW_TAG_rvalue_reference_type)
    return Ty.getSizeInBits();

  if (BaseType.isDerivedType())
    return getBaseTypeSize(DD, DIDerivedType(BaseType));

  return BaseType.getSizeInBits();
}

/// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE *Die, const MachineOperand &MO,
                                   DIType Ty) {
  // FIXME: This is a bit conservative/simple - it emits negative values at
  // their maximum bit width which is a bit unfortunate (& doesn't prefer
  // udata/sdata over dataN as suggested by the DWARF spec)
  assert(MO.isImm() && "Invalid machine operand!");
  int SizeInBits = -1;
  bool SignedConstant = isTypeSigned(DD, Ty, &SizeInBits);
  dwarf::Form Form;

  // If we're a signed constant definitely use sdata.
  if (SignedConstant) {
    addSInt(Die, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, MO.getImm());
    return;
  }

  // Else use data for now unless it's larger than we can deal with.
  switch (SizeInBits) {
  case 8:
    Form = dwarf::DW_FORM_data1;
    break;
  case 16:
    Form = dwarf::DW_FORM_data2;
    break;
  case 32:
    Form = dwarf::DW_FORM_data4;
    break;
  case 64:
    Form = dwarf::DW_FORM_data8;
    break;
  default:
    Form = dwarf::DW_FORM_udata;
    addUInt(Die, dwarf::DW_AT_const_value, Form, MO.getImm());
    return;
  }
  addUInt(Die, dwarf::DW_AT_const_value, Form, MO.getImm());
}

/// addConstantFPValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantFPValue(DIE *Die, const MachineOperand &MO) {
  assert(MO.isFPImm() && "Invalid machine operand!");
  DIEBlock *Block = new (DIEValueAllocator) DIEBlock();
  APFloat FPImm = MO.getFPImm()->getValueAPF();

  // Get the raw data form of the floating point.
  const APInt FltVal = FPImm.bitcastToAPInt();
  const char *FltPtr = (const char *)FltVal.getRawData();

  int NumBytes = FltVal.getBitWidth() / 8; // 8 bits per byte.
  bool LittleEndian = Asm->getDataLayout().isLittleEndian();
  int Incr = (LittleEndian ? 1 : -1);
  int Start = (LittleEndian ? 0 : NumBytes - 1);
  int Stop = (LittleEndian ? NumBytes : -1);

  // Output the constant to DWARF one byte at a time.
  for (; Start != Stop; Start += Incr)
    addUInt(Block, dwarf::DW_FORM_data1, (unsigned char)0xFF & FltPtr[Start]);

  addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addConstantFPValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantFPValue(DIE *Die, const ConstantFP *CFP) {
  // Pass this down to addConstantValue as an unsigned bag of bits.
  addConstantValue(Die, CFP->getValueAPF().bitcastToAPInt(), true);
}

/// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE *Die, const ConstantInt *CI,
                                   bool Unsigned) {
  addConstantValue(Die, CI->getValue(), Unsigned);
}

// addConstantValue - Add constant value entry in variable DIE.
void CompileUnit::addConstantValue(DIE *Die, const APInt &Val, bool Unsigned) {
  unsigned CIBitWidth = Val.getBitWidth();
  if (CIBitWidth <= 64) {
    // If we're a signed constant definitely use sdata.
    if (!Unsigned) {
      addSInt(Die, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata,
              Val.getSExtValue());
      return;
    }

    // Else use data for now unless it's larger than we can deal with.
    dwarf::Form Form;
    switch (CIBitWidth) {
    case 8:
      Form = dwarf::DW_FORM_data1;
      break;
    case 16:
      Form = dwarf::DW_FORM_data2;
      break;
    case 32:
      Form = dwarf::DW_FORM_data4;
      break;
    case 64:
      Form = dwarf::DW_FORM_data8;
      break;
    default:
      addUInt(Die, dwarf::DW_AT_const_value, dwarf::DW_FORM_udata,
              Val.getZExtValue());
      return;
    }
    addUInt(Die, dwarf::DW_AT_const_value, Form, Val.getZExtValue());
    return;
  }

  DIEBlock *Block = new (DIEValueAllocator) DIEBlock();

  // Get the raw data form of the large APInt.
  const uint64_t *Ptr64 = Val.getRawData();

  int NumBytes = Val.getBitWidth() / 8; // 8 bits per byte.
  bool LittleEndian = Asm->getDataLayout().isLittleEndian();

  // Output the constant to DWARF one byte at a time.
  for (int i = 0; i < NumBytes; i++) {
    uint8_t c;
    if (LittleEndian)
      c = Ptr64[i / 8] >> (8 * (i & 7));
    else
      c = Ptr64[(NumBytes - 1 - i) / 8] >> (8 * ((NumBytes - 1 - i) & 7));
    addUInt(Block, dwarf::DW_FORM_data1, c);
  }

  addBlock(Die, dwarf::DW_AT_const_value, Block);
}

/// addTemplateParams - Add template parameters into buffer.
void CompileUnit::addTemplateParams(DIE &Buffer, DIArray TParams) {
  // Add template parameters.
  for (unsigned i = 0, e = TParams.getNumElements(); i != e; ++i) {
    DIDescriptor Element = TParams.getElement(i);
    if (Element.isTemplateTypeParameter())
      constructTemplateTypeParameterDIE(Buffer,
                                        DITemplateTypeParameter(Element));
    else if (Element.isTemplateValueParameter())
      constructTemplateValueParameterDIE(Buffer,
                                         DITemplateValueParameter(Element));
  }
}

/// getOrCreateContextDIE - Get context owner's DIE.
DIE *CompileUnit::getOrCreateContextDIE(DIScope Context) {
  if (!Context || Context.isFile())
    return getCUDie();
  if (Context.isType())
    return getOrCreateTypeDIE(DIType(Context));
  if (Context.isNameSpace())
    return getOrCreateNameSpace(DINameSpace(Context));
  if (Context.isSubprogram())
    return getOrCreateSubprogramDIE(DISubprogram(Context));
  return getDIE(Context);
}

/// getOrCreateTypeDIE - Find existing DIE or create new DIE for the
/// given DIType.
DIE *CompileUnit::getOrCreateTypeDIE(const MDNode *TyNode) {
  if (!TyNode)
    return NULL;

  DIType Ty(TyNode);
  assert(Ty.isType());

  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *ContextDIE = getOrCreateContextDIE(resolve(Ty.getContext()));
  assert(ContextDIE);

  DIE *TyDIE = getDIE(Ty);
  if (TyDIE)
    return TyDIE;

  // Create new type.
  TyDIE = createAndAddDIE(Ty.getTag(), *ContextDIE, Ty);

  if (Ty.isBasicType())
    constructTypeDIE(*TyDIE, DIBasicType(Ty));
  else if (Ty.isCompositeType())
    constructTypeDIE(*TyDIE, DICompositeType(Ty));
  else {
    assert(Ty.isDerivedType() && "Unknown kind of DIType");
    constructTypeDIE(*TyDIE, DIDerivedType(Ty));
  }
  // If this is a named finished type then include it in the list of types
  // for the accelerator tables.
  if (!Ty.getName().empty() && !Ty.isForwardDecl()) {
    bool IsImplementation = 0;
    if (Ty.isCompositeType()) {
      DICompositeType CT(Ty);
      // A runtime language of 0 actually means C/C++ and that any
      // non-negative value is some version of Objective-C/C++.
      IsImplementation = (CT.getRunTimeLang() == 0) || CT.isObjcClassComplete();
    }
    unsigned Flags = IsImplementation ? dwarf::DW_FLAG_type_implementation : 0;
    addAccelType(Ty.getName(), std::make_pair(TyDIE, Flags));
  }

  return TyDIE;
}

/// addType - Add a new type attribute to the specified entity.
void CompileUnit::addType(DIE *Entity, DIType Ty, dwarf::Attribute Attribute) {
  assert(Ty && "Trying to add a type that doesn't exist?");

  // Check for pre-existence.
  DIEEntry *Entry = getDIEEntry(Ty);
  // If it exists then use the existing value.
  if (Entry) {
    addDIEEntry(Entity, Attribute, Entry);
    return;
  }

  // Construct type.
  DIE *Buffer = getOrCreateTypeDIE(Ty);

  // Set up proxy.
  Entry = createDIEEntry(Buffer);
  insertDIEEntry(Ty, Entry);
  addDIEEntry(Entity, Attribute, Entry);

  // If this is a complete composite type then include it in the
  // list of global types.
  addGlobalType(Ty);
}

// Accelerator table mutators - add each name along with its companion
// DIE to the proper table while ensuring that the name that we're going
// to reference is in the string table. We do this since the names we
// add may not only be identical to the names in the DIE.
void CompileUnit::addAccelName(StringRef Name, DIE *Die) {
  DU->getStringPoolEntry(Name);
  std::vector<DIE *> &DIEs = AccelNames[Name];
  DIEs.push_back(Die);
}

void CompileUnit::addAccelObjC(StringRef Name, DIE *Die) {
  DU->getStringPoolEntry(Name);
  std::vector<DIE *> &DIEs = AccelObjC[Name];
  DIEs.push_back(Die);
}

void CompileUnit::addAccelNamespace(StringRef Name, DIE *Die) {
  DU->getStringPoolEntry(Name);
  std::vector<DIE *> &DIEs = AccelNamespace[Name];
  DIEs.push_back(Die);
}

void CompileUnit::addAccelType(StringRef Name, std::pair<DIE *, unsigned> Die) {
  DU->getStringPoolEntry(Name);
  std::vector<std::pair<DIE *, unsigned> > &DIEs = AccelTypes[Name];
  DIEs.push_back(Die);
}

/// addGlobalName - Add a new global name to the compile unit.
void CompileUnit::addGlobalName(StringRef Name, DIE *Die, DIScope Context) {
  std::string FullName = getParentContextString(Context) + Name.str();
  GlobalNames[FullName] = Die;
}

/// addGlobalType - Add a new global type to the compile unit.
///
void CompileUnit::addGlobalType(DIType Ty) {
  DIScope Context = resolve(Ty.getContext());
  if (!Ty.getName().empty() && !Ty.isForwardDecl() &&
      (!Context || Context.isCompileUnit() || Context.isFile() ||
       Context.isNameSpace()))
    if (DIEEntry *Entry = getDIEEntry(Ty)) {
      std::string FullName =
          getParentContextString(Context) + Ty.getName().str();
      GlobalTypes[FullName] = Entry->getEntry();
    }
}

/// getParentContextString - Walks the metadata parent chain in a language
/// specific manner (using the compile unit language) and returns
/// it as a string. This is done at the metadata level because DIEs may
/// not currently have been added to the parent context and walking the
/// DIEs looking for names is more expensive than walking the metadata.
std::string CompileUnit::getParentContextString(DIScope Context) const {
  if (!Context)
    return "";

  // FIXME: Decide whether to implement this for non-C++ languages.
  if (getLanguage() != dwarf::DW_LANG_C_plus_plus)
    return "";

  std::string CS;
  SmallVector<DIScope, 1> Parents;
  while (!Context.isCompileUnit()) {
    Parents.push_back(Context);
    if (Context.getContext())
      Context = resolve(Context.getContext());
    else
      // Structure, etc types will have a NULL context if they're at the top
      // level.
      break;
  }

  // Reverse iterate over our list to go from the outermost construct to the
  // innermost.
  for (SmallVectorImpl<DIScope>::reverse_iterator I = Parents.rbegin(),
                                                  E = Parents.rend();
       I != E; ++I) {
    DIScope Ctx = *I;
    StringRef Name = Ctx.getName();
    if (!Name.empty()) {
      CS += Name;
      CS += "::";
    }
  }
  return CS;
}

/// addPubTypes - Add subprogram argument types for pubtypes section.
void CompileUnit::addPubTypes(DISubprogram SP) {
  DICompositeType SPTy = SP.getType();
  uint16_t SPTag = SPTy.getTag();
  if (SPTag != dwarf::DW_TAG_subroutine_type)
    return;

  DIArray Args = SPTy.getTypeArray();
  for (unsigned i = 0, e = Args.getNumElements(); i != e; ++i) {
    DIType ATy(Args.getElement(i));
    if (!ATy.isType())
      continue;
    addGlobalType(ATy);
  }
}

/// constructTypeDIE - Construct basic type die from DIBasicType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DIBasicType BTy) {
  // Get core information.
  StringRef Name = BTy.getName();
  // Add name if not anonymous or intermediate type.
  if (!Name.empty())
    addString(&Buffer, dwarf::DW_AT_name, Name);

  // An unspecified type only has a name attribute.
  if (BTy.getTag() == dwarf::DW_TAG_unspecified_type)
    return;

  addUInt(&Buffer, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1,
          BTy.getEncoding());

  uint64_t Size = BTy.getSizeInBits() >> 3;
  addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);
}

/// constructTypeDIE - Construct derived type die from DIDerivedType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DIDerivedType DTy) {
  // Get core information.
  StringRef Name = DTy.getName();
  uint64_t Size = DTy.getSizeInBits() >> 3;
  uint16_t Tag = Buffer.getTag();

  // Map to main type, void will not have a type.
  DIType FromTy = resolve(DTy.getTypeDerivedFrom());
  if (FromTy)
    addType(&Buffer, FromTy);

  // Add name if not anonymous or intermediate type.
  if (!Name.empty())
    addString(&Buffer, dwarf::DW_AT_name, Name);

  // Add size if non-zero (derived types might be zero-sized.)
  if (Size && Tag != dwarf::DW_TAG_pointer_type)
    addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);

  if (Tag == dwarf::DW_TAG_ptr_to_member_type)
    addDIEEntry(&Buffer, dwarf::DW_AT_containing_type,
                getOrCreateTypeDIE(resolve(DTy.getClassType())));
  // Add source line info if available and TyDesc is not a forward declaration.
  if (!DTy.isForwardDecl())
    addSourceLine(&Buffer, DTy);
}

/// Return true if the type is appropriately scoped to be contained inside
/// its own type unit.
static bool isTypeUnitScoped(DIType Ty, const DwarfDebug *DD) {
  DIScope Parent = DD->resolve(Ty.getContext());
  while (Parent) {
    // Don't generate a hash for anything scoped inside a function.
    if (Parent.isSubprogram())
      return false;
    Parent = DD->resolve(Parent.getContext());
  }
  return true;
}

/// Return true if the type should be split out into a type unit.
static bool shouldCreateTypeUnit(DICompositeType CTy, const DwarfDebug *DD) {
  uint16_t Tag = CTy.getTag();

  switch (Tag) {
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_union_type:
  case dwarf::DW_TAG_enumeration_type:
  case dwarf::DW_TAG_class_type:
    // If this is a class, structure, union, or enumeration type
    // that is a definition (not a declaration), and not scoped
    // inside a function then separate this out as a type unit.
    return !CTy.isForwardDecl() && isTypeUnitScoped(CTy, DD);
  default:
    return false;
  }
}

/// constructTypeDIE - Construct type DIE from DICompositeType.
void CompileUnit::constructTypeDIE(DIE &Buffer, DICompositeType CTy) {
  // Get core information.
  StringRef Name = CTy.getName();

  uint64_t Size = CTy.getSizeInBits() >> 3;
  uint16_t Tag = Buffer.getTag();

  switch (Tag) {
  case dwarf::DW_TAG_array_type:
    constructArrayTypeDIE(Buffer, CTy);
    break;
  case dwarf::DW_TAG_enumeration_type:
    constructEnumTypeDIE(Buffer, CTy);
    break;
  case dwarf::DW_TAG_subroutine_type: {
    // Add return type. A void return won't have a type.
    DIArray Elements = CTy.getTypeArray();
    DIType RTy(Elements.getElement(0));
    if (RTy)
      addType(&Buffer, RTy);

    bool isPrototyped = true;
    // Add arguments.
    for (unsigned i = 1, N = Elements.getNumElements(); i < N; ++i) {
      DIDescriptor Ty = Elements.getElement(i);
      if (Ty.isUnspecifiedParameter()) {
        createAndAddDIE(dwarf::DW_TAG_unspecified_parameters, Buffer);
        isPrototyped = false;
      } else {
        DIE *Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, Buffer);
        addType(Arg, DIType(Ty));
        if (DIType(Ty).isArtificial())
          addFlag(Arg, dwarf::DW_AT_artificial);
      }
    }
    // Add prototype flag if we're dealing with a C language and the
    // function has been prototyped.
    uint16_t Language = getLanguage();
    if (isPrototyped &&
        (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 ||
         Language == dwarf::DW_LANG_ObjC))
      addFlag(&Buffer, dwarf::DW_AT_prototyped);
  } break;
  case dwarf::DW_TAG_structure_type:
  case dwarf::DW_TAG_union_type:
  case dwarf::DW_TAG_class_type: {
    // Add elements to structure type.
    DIArray Elements = CTy.getTypeArray();
    for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
      DIDescriptor Element = Elements.getElement(i);
      DIE *ElemDie = NULL;
      if (Element.isSubprogram()) {
        DISubprogram SP(Element);
        ElemDie = getOrCreateSubprogramDIE(SP);
        if (SP.isProtected())
          addUInt(ElemDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
                  dwarf::DW_ACCESS_protected);
        else if (SP.isPrivate())
          addUInt(ElemDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
                  dwarf::DW_ACCESS_private);
        else
          addUInt(ElemDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
                  dwarf::DW_ACCESS_public);
        if (SP.isExplicit())
          addFlag(ElemDie, dwarf::DW_AT_explicit);
      } else if (Element.isDerivedType()) {
        DIDerivedType DDTy(Element);
        if (DDTy.getTag() == dwarf::DW_TAG_friend) {
          ElemDie = createAndAddDIE(dwarf::DW_TAG_friend, Buffer);
          addType(ElemDie, resolve(DDTy.getTypeDerivedFrom()),
                  dwarf::DW_AT_friend);
        } else if (DDTy.isStaticMember()) {
          getOrCreateStaticMemberDIE(DDTy);
        } else {
          constructMemberDIE(Buffer, DDTy);
        }
      } else if (Element.isObjCProperty()) {
        DIObjCProperty Property(Element);
        ElemDie = createAndAddDIE(Property.getTag(), Buffer);
        StringRef PropertyName = Property.getObjCPropertyName();
        addString(ElemDie, dwarf::DW_AT_APPLE_property_name, PropertyName);
        addType(ElemDie, Property.getType());
        addSourceLine(ElemDie, Property);
        StringRef GetterName = Property.getObjCPropertyGetterName();
        if (!GetterName.empty())
          addString(ElemDie, dwarf::DW_AT_APPLE_property_getter, GetterName);
        StringRef SetterName = Property.getObjCPropertySetterName();
        if (!SetterName.empty())
          addString(ElemDie, dwarf::DW_AT_APPLE_property_setter, SetterName);
        unsigned PropertyAttributes = 0;
        if (Property.isReadOnlyObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_readonly;
        if (Property.isReadWriteObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_readwrite;
        if (Property.isAssignObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_assign;
        if (Property.isRetainObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_retain;
        if (Property.isCopyObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_copy;
        if (Property.isNonAtomicObjCProperty())
          PropertyAttributes |= dwarf::DW_APPLE_PROPERTY_nonatomic;
        if (PropertyAttributes)
          addUInt(ElemDie, dwarf::DW_AT_APPLE_property_attribute, None,
                  PropertyAttributes);

        DIEEntry *Entry = getDIEEntry(Element);
        if (!Entry) {
          Entry = createDIEEntry(ElemDie);
          insertDIEEntry(Element, Entry);
        }
      } else
        continue;
    }

    if (CTy.isAppleBlockExtension())
      addFlag(&Buffer, dwarf::DW_AT_APPLE_block);

    DICompositeType ContainingType(resolve(CTy.getContainingType()));
    if (ContainingType)
      addDIEEntry(&Buffer, dwarf::DW_AT_containing_type,
                  getOrCreateTypeDIE(ContainingType));

    if (CTy.isObjcClassComplete())
      addFlag(&Buffer, dwarf::DW_AT_APPLE_objc_complete_type);

    // Add template parameters to a class, structure or union types.
    // FIXME: The support isn't in the metadata for this yet.
    if (Tag == dwarf::DW_TAG_class_type ||
        Tag == dwarf::DW_TAG_structure_type || Tag == dwarf::DW_TAG_union_type)
      addTemplateParams(Buffer, CTy.getTemplateParams());

    break;
  }
  default:
    break;
  }

  // Add name if not anonymous or intermediate type.
  if (!Name.empty())
    addString(&Buffer, dwarf::DW_AT_name, Name);

  if (Tag == dwarf::DW_TAG_enumeration_type ||
      Tag == dwarf::DW_TAG_class_type || Tag == dwarf::DW_TAG_structure_type ||
      Tag == dwarf::DW_TAG_union_type) {
    // Add size if non-zero (derived types might be zero-sized.)
    // TODO: Do we care about size for enum forward declarations?
    if (Size)
      addUInt(&Buffer, dwarf::DW_AT_byte_size, None, Size);
    else if (!CTy.isForwardDecl())
      // Add zero size if it is not a forward declaration.
      addUInt(&Buffer, dwarf::DW_AT_byte_size, None, 0);

    // If we're a forward decl, say so.
    if (CTy.isForwardDecl())
      addFlag(&Buffer, dwarf::DW_AT_declaration);

    // Add source line info if available.
    if (!CTy.isForwardDecl())
      addSourceLine(&Buffer, CTy);

    // No harm in adding the runtime language to the declaration.
    unsigned RLang = CTy.getRunTimeLang();
    if (RLang)
      addUInt(&Buffer, dwarf::DW_AT_APPLE_runtime_class, dwarf::DW_FORM_data1,
              RLang);
  }
  // If this is a type applicable to a type unit it then add it to the
  // list of types we'll compute a hash for later.
  if (shouldCreateTypeUnit(CTy, DD))
    DD->addTypeUnitType(&Buffer);
}

/// constructTemplateTypeParameterDIE - Construct new DIE for the given
/// DITemplateTypeParameter.
void
CompileUnit::constructTemplateTypeParameterDIE(DIE &Buffer,
                                               DITemplateTypeParameter TP) {
  DIE *ParamDIE =
      createAndAddDIE(dwarf::DW_TAG_template_type_parameter, Buffer);
  // Add the type if it exists, it could be void and therefore no type.
  if (TP.getType())
    addType(ParamDIE, resolve(TP.getType()));
  if (!TP.getName().empty())
    addString(ParamDIE, dwarf::DW_AT_name, TP.getName());
}

/// constructTemplateValueParameterDIE - Construct new DIE for the given
/// DITemplateValueParameter.
void
CompileUnit::constructTemplateValueParameterDIE(DIE &Buffer,
                                                DITemplateValueParameter VP) {
  DIE *ParamDIE = createAndAddDIE(VP.getTag(), Buffer);

  // Add the type if there is one, template template and template parameter
  // packs will not have a type.
  if (VP.getTag() == dwarf::DW_TAG_template_value_parameter)
    addType(ParamDIE, resolve(VP.getType()));
  if (!VP.getName().empty())
    addString(ParamDIE, dwarf::DW_AT_name, VP.getName());
  if (Value *Val = VP.getValue()) {
    if (ConstantInt *CI = dyn_cast<ConstantInt>(Val))
      addConstantValue(ParamDIE, CI,
                       isUnsignedDIType(DD, resolve(VP.getType())));
    else if (GlobalValue *GV = dyn_cast<GlobalValue>(Val)) {
      // For declaration non-type template parameters (such as global values and
      // functions)
      DIEBlock *Block = new (DIEValueAllocator) DIEBlock();
      addOpAddress(Block, Asm->getSymbol(GV));
      // Emit DW_OP_stack_value to use the address as the immediate value of the
      // parameter, rather than a pointer to it.
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_stack_value);
      addBlock(ParamDIE, dwarf::DW_AT_location, Block);
    } else if (VP.getTag() == dwarf::DW_TAG_GNU_template_template_param) {
      assert(isa<MDString>(Val));
      addString(ParamDIE, dwarf::DW_AT_GNU_template_name,
                cast<MDString>(Val)->getString());
    } else if (VP.getTag() == dwarf::DW_TAG_GNU_template_parameter_pack) {
      assert(isa<MDNode>(Val));
      DIArray A(cast<MDNode>(Val));
      addTemplateParams(*ParamDIE, A);
    }
  }
}

/// getOrCreateNameSpace - Create a DIE for DINameSpace.
DIE *CompileUnit::getOrCreateNameSpace(DINameSpace NS) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *ContextDIE = getOrCreateContextDIE(NS.getContext());

  DIE *NDie = getDIE(NS);
  if (NDie)
    return NDie;
  NDie = createAndAddDIE(dwarf::DW_TAG_namespace, *ContextDIE, NS);

  if (!NS.getName().empty()) {
    addString(NDie, dwarf::DW_AT_name, NS.getName());
    addAccelNamespace(NS.getName(), NDie);
    addGlobalName(NS.getName(), NDie, NS.getContext());
  } else
    addAccelNamespace("(anonymous namespace)", NDie);
  addSourceLine(NDie, NS);
  return NDie;
}

/// getOrCreateSubprogramDIE - Create new DIE using SP.
DIE *CompileUnit::getOrCreateSubprogramDIE(DISubprogram SP) {
  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE (as is the case for member function
  // declarations).
  DIE *ContextDIE = getOrCreateContextDIE(resolve(SP.getContext()));

  DIE *SPDie = getDIE(SP);
  if (SPDie)
    return SPDie;

  DISubprogram SPDecl = SP.getFunctionDeclaration();
  if (SPDecl.isSubprogram())
    // Add subprogram definitions to the CU die directly.
    ContextDIE = CUDie.get();

  // DW_TAG_inlined_subroutine may refer to this DIE.
  SPDie = createAndAddDIE(dwarf::DW_TAG_subprogram, *ContextDIE, SP);

  DIE *DeclDie = NULL;
  if (SPDecl.isSubprogram())
    DeclDie = getOrCreateSubprogramDIE(SPDecl);

  // Add function template parameters.
  addTemplateParams(*SPDie, SP.getTemplateParams());

  // If this DIE is going to refer declaration info using AT_specification
  // then there is no need to add other attributes.
  if (DeclDie) {
    // Refer function declaration directly.
    addDIEEntry(SPDie, dwarf::DW_AT_specification, DeclDie);

    return SPDie;
  }

  // Add the linkage name if we have one.
  StringRef LinkageName = SP.getLinkageName();
  if (!LinkageName.empty())
    addString(SPDie, dwarf::DW_AT_MIPS_linkage_name,
              GlobalValue::getRealLinkageName(LinkageName));

  // Constructors and operators for anonymous aggregates do not have names.
  if (!SP.getName().empty())
    addString(SPDie, dwarf::DW_AT_name, SP.getName());

  addSourceLine(SPDie, SP);

  // Add the prototype if we have a prototype and we have a C like
  // language.
  uint16_t Language = getLanguage();
  if (SP.isPrototyped() &&
      (Language == dwarf::DW_LANG_C89 || Language == dwarf::DW_LANG_C99 ||
       Language == dwarf::DW_LANG_ObjC))
    addFlag(SPDie, dwarf::DW_AT_prototyped);

  DICompositeType SPTy = SP.getType();
  assert(SPTy.getTag() == dwarf::DW_TAG_subroutine_type &&
         "the type of a subprogram should be a subroutine");

  DIArray Args = SPTy.getTypeArray();
  // Add a return type. If this is a type like a C/C++ void type we don't add a
  // return type.
  if (Args.getElement(0))
    addType(SPDie, DIType(Args.getElement(0)));

  unsigned VK = SP.getVirtuality();
  if (VK) {
    addUInt(SPDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1, VK);
    DIEBlock *Block = getDIEBlock();
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
    addUInt(Block, dwarf::DW_FORM_udata, SP.getVirtualIndex());
    addBlock(SPDie, dwarf::DW_AT_vtable_elem_location, Block);
    ContainingTypeMap.insert(
        std::make_pair(SPDie, resolve(SP.getContainingType())));
  }

  if (!SP.isDefinition()) {
    addFlag(SPDie, dwarf::DW_AT_declaration);

    // Add arguments. Do not add arguments for subprogram definition. They will
    // be handled while processing variables.
    for (unsigned i = 1, N = Args.getNumElements(); i < N; ++i) {
      DIE *Arg = createAndAddDIE(dwarf::DW_TAG_formal_parameter, *SPDie);
      DIType ATy(Args.getElement(i));
      addType(Arg, ATy);
      if (ATy.isArtificial())
        addFlag(Arg, dwarf::DW_AT_artificial);
    }
  }

  if (SP.isArtificial())
    addFlag(SPDie, dwarf::DW_AT_artificial);

  if (!SP.isLocalToUnit())
    addFlag(SPDie, dwarf::DW_AT_external);

  if (SP.isOptimized())
    addFlag(SPDie, dwarf::DW_AT_APPLE_optimized);

  if (unsigned isa = Asm->getISAEncoding()) {
    addUInt(SPDie, dwarf::DW_AT_APPLE_isa, dwarf::DW_FORM_flag, isa);
  }

  return SPDie;
}

// Return const expression if value is a GEP to access merged global
// constant. e.g.
// i8* getelementptr ({ i8, i8, i8, i8 }* @_MergedGlobals, i32 0, i32 0)
static const ConstantExpr *getMergedGlobalExpr(const Value *V) {
  const ConstantExpr *CE = dyn_cast_or_null<ConstantExpr>(V);
  if (!CE || CE->getNumOperands() != 3 ||
      CE->getOpcode() != Instruction::GetElementPtr)
    return NULL;

  // First operand points to a global struct.
  Value *Ptr = CE->getOperand(0);
  if (!isa<GlobalValue>(Ptr) ||
      !isa<StructType>(cast<PointerType>(Ptr->getType())->getElementType()))
    return NULL;

  // Second operand is zero.
  const ConstantInt *CI = dyn_cast_or_null<ConstantInt>(CE->getOperand(1));
  if (!CI || !CI->isZero())
    return NULL;

  // Third operand is offset.
  if (!isa<ConstantInt>(CE->getOperand(2)))
    return NULL;

  return CE;
}

/// createGlobalVariableDIE - create global variable DIE.
void CompileUnit::createGlobalVariableDIE(DIGlobalVariable GV) {

  // Check for pre-existence.
  if (getDIE(GV))
    return;

  if (!GV.isGlobalVariable())
    return;

  DIScope GVContext = GV.getContext();
  DIType GTy = GV.getType();

  // If this is a static data member definition, some attributes belong
  // to the declaration DIE.
  DIE *VariableDIE = NULL;
  bool IsStaticMember = false;
  DIDerivedType SDMDecl = GV.getStaticDataMemberDeclaration();
  if (SDMDecl.Verify()) {
    assert(SDMDecl.isStaticMember() && "Expected static member decl");
    // We need the declaration DIE that is in the static member's class.
    VariableDIE = getOrCreateStaticMemberDIE(SDMDecl);
    IsStaticMember = true;
  }

  // If this is not a static data member definition, create the variable
  // DIE and add the initial set of attributes to it.
  if (!VariableDIE) {
    // Construct the context before querying for the existence of the DIE in
    // case such construction creates the DIE.
    DIE *ContextDIE = getOrCreateContextDIE(GVContext);

    // Add to map.
    VariableDIE = createAndAddDIE(GV.getTag(), *ContextDIE, GV);

    // Add name and type.
    addString(VariableDIE, dwarf::DW_AT_name, GV.getDisplayName());
    addType(VariableDIE, GTy);

    // Add scoping info.
    if (!GV.isLocalToUnit())
      addFlag(VariableDIE, dwarf::DW_AT_external);

    // Add line number info.
    addSourceLine(VariableDIE, GV);
  }

  // Add location.
  bool addToAccelTable = false;
  DIE *VariableSpecDIE = NULL;
  bool isGlobalVariable = GV.getGlobal() != NULL;
  if (isGlobalVariable) {
    addToAccelTable = true;
    DIEBlock *Block = new (DIEValueAllocator) DIEBlock();
    const MCSymbol *Sym = Asm->getSymbol(GV.getGlobal());
    if (GV.getGlobal()->isThreadLocal()) {
      // FIXME: Make this work with -gsplit-dwarf.
      unsigned PointerSize = Asm->getDataLayout().getPointerSize();
      assert((PointerSize == 4 || PointerSize == 8) &&
             "Add support for other sizes if necessary");
      const MCExpr *Expr =
          Asm->getObjFileLowering().getDebugThreadLocalSymbol(Sym);
      // Based on GCC's support for TLS:
      if (!DD->useSplitDwarf()) {
        // 1) Start with a constNu of the appropriate pointer size
        addUInt(Block, dwarf::DW_FORM_data1,
                PointerSize == 4 ? dwarf::DW_OP_const4u : dwarf::DW_OP_const8u);
        // 2) containing the (relocated) offset of the TLS variable
        //    within the module's TLS block.
        addExpr(Block, dwarf::DW_FORM_udata, Expr);
      } else {
        addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_GNU_const_index);
        addUInt(Block, dwarf::DW_FORM_udata, DU->getAddrPoolIndex(Expr));
      }
      // 3) followed by a custom OP to make the debugger do a TLS lookup.
      addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_GNU_push_tls_address);
    } else
      addOpAddress(Block, Sym);
    // Do not create specification DIE if context is either compile unit
    // or a subprogram.
    if (GVContext && GV.isDefinition() && !GVContext.isCompileUnit() &&
        !GVContext.isFile() && !DD->isSubprogramContext(GVContext)) {
      // Create specification DIE.
      VariableSpecDIE = createAndAddDIE(dwarf::DW_TAG_variable, *CUDie);
      addDIEEntry(VariableSpecDIE, dwarf::DW_AT_specification, VariableDIE);
      addBlock(VariableSpecDIE, dwarf::DW_AT_location, Block);
      // A static member's declaration is already flagged as such.
      if (!SDMDecl.Verify())
        addFlag(VariableDIE, dwarf::DW_AT_declaration);
    } else {
      addBlock(VariableDIE, dwarf::DW_AT_location, Block);
    }
    // Add the linkage name.
    StringRef LinkageName = GV.getLinkageName();
    if (!LinkageName.empty())
      // From DWARF4: DIEs to which DW_AT_linkage_name may apply include:
      // TAG_common_block, TAG_constant, TAG_entry_point, TAG_subprogram and
      // TAG_variable.
      addString(IsStaticMember && VariableSpecDIE ? VariableSpecDIE
                                                  : VariableDIE,
                dwarf::DW_AT_MIPS_linkage_name,
                GlobalValue::getRealLinkageName(LinkageName));
  } else if (const ConstantInt *CI =
                 dyn_cast_or_null<ConstantInt>(GV.getConstant())) {
    // AT_const_value was added when the static member was created. To avoid
    // emitting AT_const_value multiple times, we only add AT_const_value when
    // it is not a static member.
    if (!IsStaticMember)
      addConstantValue(VariableDIE, CI, isUnsignedDIType(DD, GTy));
  } else if (const ConstantExpr *CE = getMergedGlobalExpr(GV->getOperand(11))) {
    addToAccelTable = true;
    // GV is a merged global.
    DIEBlock *Block = new (DIEValueAllocator) DIEBlock();
    Value *Ptr = CE->getOperand(0);
    addOpAddress(Block, Asm->getSymbol(cast<GlobalValue>(Ptr)));
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
    SmallVector<Value *, 3> Idx(CE->op_begin() + 1, CE->op_end());
    addUInt(Block, dwarf::DW_FORM_udata,
            Asm->getDataLayout().getIndexedOffset(Ptr->getType(), Idx));
    addUInt(Block, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);
    addBlock(VariableDIE, dwarf::DW_AT_location, Block);
  }

  if (addToAccelTable) {
    DIE *AddrDIE = VariableSpecDIE ? VariableSpecDIE : VariableDIE;
    addAccelName(GV.getName(), AddrDIE);

    // If the linkage name is different than the name, go ahead and output
    // that as well into the name table.
    if (GV.getLinkageName() != "" && GV.getName() != GV.getLinkageName())
      addAccelName(GV.getLinkageName(), AddrDIE);
  }

  if (!GV.isLocalToUnit())
    addGlobalName(GV.getName(), VariableSpecDIE ? VariableSpecDIE : VariableDIE,
                  GV.getContext());
}

/// constructSubrangeDIE - Construct subrange DIE from DISubrange.
void CompileUnit::constructSubrangeDIE(DIE &Buffer, DISubrange SR,
                                       DIE *IndexTy) {
  DIE *DW_Subrange = createAndAddDIE(dwarf::DW_TAG_subrange_type, Buffer);
  addDIEEntry(DW_Subrange, dwarf::DW_AT_type, IndexTy);

  // The LowerBound value defines the lower bounds which is typically zero for
  // C/C++. The Count value is the number of elements.  Values are 64 bit. If
  // Count == -1 then the array is unbounded and we do not emit
  // DW_AT_lower_bound and DW_AT_upper_bound attributes. If LowerBound == 0 and
  // Count == 0, then the array has zero elements in which case we do not emit
  // an upper bound.
  int64_t LowerBound = SR.getLo();
  int64_t DefaultLowerBound = getDefaultLowerBound();
  int64_t Count = SR.getCount();

  if (DefaultLowerBound == -1 || LowerBound != DefaultLowerBound)
    addUInt(DW_Subrange, dwarf::DW_AT_lower_bound, None, LowerBound);

  if (Count != -1 && Count != 0)
    // FIXME: An unbounded array should reference the expression that defines
    // the array.
    addUInt(DW_Subrange, dwarf::DW_AT_upper_bound, None,
            LowerBound + Count - 1);
}

/// constructArrayTypeDIE - Construct array type DIE from DICompositeType.
void CompileUnit::constructArrayTypeDIE(DIE &Buffer, DICompositeType CTy) {
  if (CTy.isVector())
    addFlag(&Buffer, dwarf::DW_AT_GNU_vector);

  // Emit the element type.
  addType(&Buffer, resolve(CTy.getTypeDerivedFrom()));

  // Get an anonymous type for index type.
  // FIXME: This type should be passed down from the front end
  // as different languages may have different sizes for indexes.
  DIE *IdxTy = getIndexTyDie();
  if (!IdxTy) {
    // Construct an anonymous type for index type.
    IdxTy = createAndAddDIE(dwarf::DW_TAG_base_type, *CUDie.get());
    addString(IdxTy, dwarf::DW_AT_name, "int");
    addUInt(IdxTy, dwarf::DW_AT_byte_size, None, sizeof(int32_t));
    addUInt(IdxTy, dwarf::DW_AT_encoding, dwarf::DW_FORM_data1,
            dwarf::DW_ATE_signed);
    setIndexTyDie(IdxTy);
  }

  // Add subranges to array type.
  DIArray Elements = CTy.getTypeArray();
  for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
    DIDescriptor Element = Elements.getElement(i);
    if (Element.getTag() == dwarf::DW_TAG_subrange_type)
      constructSubrangeDIE(Buffer, DISubrange(Element), IdxTy);
  }
}

/// constructEnumTypeDIE - Construct an enum type DIE from DICompositeType.
void CompileUnit::constructEnumTypeDIE(DIE &Buffer, DICompositeType CTy) {
  DIArray Elements = CTy.getTypeArray();

  // Add enumerators to enumeration type.
  for (unsigned i = 0, N = Elements.getNumElements(); i < N; ++i) {
    DIEnumerator Enum(Elements.getElement(i));
    if (Enum.isEnumerator()) {
      DIE *Enumerator = createAndAddDIE(dwarf::DW_TAG_enumerator, Buffer);
      StringRef Name = Enum.getName();
      addString(Enumerator, dwarf::DW_AT_name, Name);
      int64_t Value = Enum.getEnumValue();
      addSInt(Enumerator, dwarf::DW_AT_const_value, dwarf::DW_FORM_sdata, Value);
    }
  }
  DIType DTy = resolve(CTy.getTypeDerivedFrom());
  if (DTy) {
    addType(&Buffer, DTy);
    addFlag(&Buffer, dwarf::DW_AT_enum_class);
  }
}

/// constructContainingTypeDIEs - Construct DIEs for types that contain
/// vtables.
void CompileUnit::constructContainingTypeDIEs() {
  for (DenseMap<DIE *, const MDNode *>::iterator CI = ContainingTypeMap.begin(),
                                                 CE = ContainingTypeMap.end();
       CI != CE; ++CI) {
    DIE *SPDie = CI->first;
    DIDescriptor D(CI->second);
    if (!D)
      continue;
    DIE *NDie = getDIE(D);
    if (!NDie)
      continue;
    addDIEEntry(SPDie, dwarf::DW_AT_containing_type, NDie);
  }
}

/// constructVariableDIE - Construct a DIE for the given DbgVariable.
DIE *CompileUnit::constructVariableDIE(DbgVariable &DV, bool isScopeAbstract) {
  StringRef Name = DV.getName();

  // Define variable debug information entry.
  DIE *VariableDie = new DIE(DV.getTag());
  DbgVariable *AbsVar = DV.getAbstractVariable();
  DIE *AbsDIE = AbsVar ? AbsVar->getDIE() : NULL;
  if (AbsDIE)
    addDIEEntry(VariableDie, dwarf::DW_AT_abstract_origin, AbsDIE);
  else {
    if (!Name.empty())
      addString(VariableDie, dwarf::DW_AT_name, Name);
    addSourceLine(VariableDie, DV.getVariable());
    addType(VariableDie, DV.getType());
  }

  if (DV.isArtificial())
    addFlag(VariableDie, dwarf::DW_AT_artificial);

  if (isScopeAbstract) {
    DV.setDIE(VariableDie);
    return VariableDie;
  }

  // Add variable address.

  unsigned Offset = DV.getDotDebugLocOffset();
  if (Offset != ~0U) {
    addLabel(VariableDie, dwarf::DW_AT_location,
             DD->getDwarfVersion() >= 4 ? dwarf::DW_FORM_sec_offset
                                        : dwarf::DW_FORM_data4,
             Asm->GetTempSymbol("debug_loc", Offset));
    DV.setDIE(VariableDie);
    return VariableDie;
  }

  // Check if variable is described by a DBG_VALUE instruction.
  if (const MachineInstr *DVInsn = DV.getMInsn()) {
    assert(DVInsn->getNumOperands() == 3);
    if (DVInsn->getOperand(0).isReg()) {
      const MachineOperand RegOp = DVInsn->getOperand(0);
      // If the second operand is an immediate, this is an indirect value.
      if (DVInsn->getOperand(1).isImm()) {
        MachineLocation Location(RegOp.getReg(),
                                 DVInsn->getOperand(1).getImm());
        addVariableAddress(DV, VariableDie, Location);
      } else if (RegOp.getReg())
        addVariableAddress(DV, VariableDie, MachineLocation(RegOp.getReg()));
    } else if (DVInsn->getOperand(0).isImm())
      addConstantValue(VariableDie, DVInsn->getOperand(0), DV.getType());
    else if (DVInsn->getOperand(0).isFPImm())
      addConstantFPValue(VariableDie, DVInsn->getOperand(0));
    else if (DVInsn->getOperand(0).isCImm())
      addConstantValue(VariableDie, DVInsn->getOperand(0).getCImm(),
                       isUnsignedDIType(DD, DV.getType()));

    DV.setDIE(VariableDie);
    return VariableDie;
  } else {
    // .. else use frame index.
    int FI = DV.getFrameIndex();
    if (FI != ~0) {
      unsigned FrameReg = 0;
      const TargetFrameLowering *TFI = Asm->TM.getFrameLowering();
      int Offset = TFI->getFrameIndexReference(*Asm->MF, FI, FrameReg);
      MachineLocation Location(FrameReg, Offset);
      addVariableAddress(DV, VariableDie, Location);
    }
  }

  DV.setDIE(VariableDie);
  return VariableDie;
}

/// constructMemberDIE - Construct member DIE from DIDerivedType.
void CompileUnit::constructMemberDIE(DIE &Buffer, DIDerivedType DT) {
  DIE *MemberDie = createAndAddDIE(DT.getTag(), Buffer);
  StringRef Name = DT.getName();
  if (!Name.empty())
    addString(MemberDie, dwarf::DW_AT_name, Name);

  addType(MemberDie, resolve(DT.getTypeDerivedFrom()));

  addSourceLine(MemberDie, DT);

  DIEBlock *MemLocationDie = new (DIEValueAllocator) DIEBlock();
  addUInt(MemLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_plus_uconst);

  if (DT.getTag() == dwarf::DW_TAG_inheritance && DT.isVirtual()) {

    // For C++, virtual base classes are not at fixed offset. Use following
    // expression to extract appropriate offset from vtable.
    // BaseAddr = ObAddr + *((*ObAddr) - Offset)

    DIEBlock *VBaseLocationDie = new (DIEValueAllocator) DIEBlock();
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_dup);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_constu);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_udata, DT.getOffsetInBits());
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_minus);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_deref);
    addUInt(VBaseLocationDie, dwarf::DW_FORM_data1, dwarf::DW_OP_plus);

    addBlock(MemberDie, dwarf::DW_AT_data_member_location, VBaseLocationDie);
  } else {
    uint64_t Size = DT.getSizeInBits();
    uint64_t FieldSize = getBaseTypeSize(DD, DT);
    uint64_t OffsetInBytes;

    if (Size != FieldSize) {
      // Handle bitfield.
      addUInt(MemberDie, dwarf::DW_AT_byte_size, None,
              getBaseTypeSize(DD, DT) >> 3);
      addUInt(MemberDie, dwarf::DW_AT_bit_size, None, DT.getSizeInBits());

      uint64_t Offset = DT.getOffsetInBits();
      uint64_t AlignMask = ~(DT.getAlignInBits() - 1);
      uint64_t HiMark = (Offset + FieldSize) & AlignMask;
      uint64_t FieldOffset = (HiMark - FieldSize);
      Offset -= FieldOffset;

      // Maybe we need to work from the other end.
      if (Asm->getDataLayout().isLittleEndian())
        Offset = FieldSize - (Offset + Size);
      addUInt(MemberDie, dwarf::DW_AT_bit_offset, None, Offset);

      // Here WD_AT_data_member_location points to the anonymous
      // field that includes this bit field.
      OffsetInBytes = FieldOffset >> 3;
    } else
      // This is not a bitfield.
      OffsetInBytes = DT.getOffsetInBits() >> 3;
    addUInt(MemberDie, dwarf::DW_AT_data_member_location, None, OffsetInBytes);
  }

  if (DT.isProtected())
    addUInt(MemberDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_protected);
  else if (DT.isPrivate())
    addUInt(MemberDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_private);
  // Otherwise C++ member and base classes are considered public.
  else
    addUInt(MemberDie, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_public);
  if (DT.isVirtual())
    addUInt(MemberDie, dwarf::DW_AT_virtuality, dwarf::DW_FORM_data1,
            dwarf::DW_VIRTUALITY_virtual);

  // Objective-C properties.
  if (MDNode *PNode = DT.getObjCProperty())
    if (DIEEntry *PropertyDie = getDIEEntry(PNode))
      MemberDie->addValue(dwarf::DW_AT_APPLE_property, dwarf::DW_FORM_ref4,
                          PropertyDie);

  if (DT.isArtificial())
    addFlag(MemberDie, dwarf::DW_AT_artificial);
}

/// getOrCreateStaticMemberDIE - Create new DIE for C++ static member.
DIE *CompileUnit::getOrCreateStaticMemberDIE(DIDerivedType DT) {
  if (!DT.Verify())
    return NULL;

  // Construct the context before querying for the existence of the DIE in case
  // such construction creates the DIE.
  DIE *ContextDIE = getOrCreateContextDIE(resolve(DT.getContext()));
  assert(dwarf::isType(ContextDIE->getTag()) &&
         "Static member should belong to a type.");

  DIE *StaticMemberDIE = getDIE(DT);
  if (StaticMemberDIE)
    return StaticMemberDIE;

  StaticMemberDIE = createAndAddDIE(DT.getTag(), *ContextDIE, DT);

  DIType Ty = resolve(DT.getTypeDerivedFrom());

  addString(StaticMemberDIE, dwarf::DW_AT_name, DT.getName());
  addType(StaticMemberDIE, Ty);
  addSourceLine(StaticMemberDIE, DT);
  addFlag(StaticMemberDIE, dwarf::DW_AT_external);
  addFlag(StaticMemberDIE, dwarf::DW_AT_declaration);

  // FIXME: We could omit private if the parent is a class_type, and
  // public if the parent is something else.
  if (DT.isProtected())
    addUInt(StaticMemberDIE, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_protected);
  else if (DT.isPrivate())
    addUInt(StaticMemberDIE, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_private);
  else
    addUInt(StaticMemberDIE, dwarf::DW_AT_accessibility, dwarf::DW_FORM_data1,
            dwarf::DW_ACCESS_public);

  if (const ConstantInt *CI = dyn_cast_or_null<ConstantInt>(DT.getConstant()))
    addConstantValue(StaticMemberDIE, CI, isUnsignedDIType(DD, Ty));
  if (const ConstantFP *CFP = dyn_cast_or_null<ConstantFP>(DT.getConstant()))
    addConstantFPValue(StaticMemberDIE, CFP);

  return StaticMemberDIE;
}

void CompileUnit::emitHeader(const MCSection *ASection,
                             const MCSymbol *ASectionSym) {
  Asm->OutStreamer.AddComment("DWARF version number");
  Asm->EmitInt16(DD->getDwarfVersion());
  Asm->OutStreamer.AddComment("Offset Into Abbrev. Section");
  Asm->EmitSectionOffset(Asm->GetTempSymbol(ASection->getLabelBeginName()),
                         ASectionSym);
  Asm->OutStreamer.AddComment("Address Size (in bytes)");
  Asm->EmitInt8(Asm->getDataLayout().getPointerSize());
}
