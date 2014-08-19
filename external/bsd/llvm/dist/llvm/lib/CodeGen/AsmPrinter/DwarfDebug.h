//===-- llvm/CodeGen/DwarfDebug.h - Dwarf Debug Framework ------*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for writing dwarf debug info into asm files.
//
//===----------------------------------------------------------------------===//

#ifndef CODEGEN_ASMPRINTER_DWARFDEBUG_H__
#define CODEGEN_ASMPRINTER_DWARFDEBUG_H__

#include "DwarfFile.h"
#include "AsmPrinterHandler.h"
#include "DIE.h"
#include "DbgValueHistoryCalculator.h"
#include "DebugLocEntry.h"
#include "DebugLocList.h"
#include "DwarfAccelTable.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/CodeGen/LexicalScopes.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCDwarf.h"
#include "llvm/Support/Allocator.h"

#include <memory>

namespace llvm {

class AsmPrinter;
class ByteStreamer;
class ConstantInt;
class ConstantFP;
class DwarfCompileUnit;
class DwarfDebug;
class DwarfTypeUnit;
class DwarfUnit;
class MachineModuleInfo;

//===----------------------------------------------------------------------===//
/// \brief This class is used to record source line correspondence.
class SrcLineInfo {
  unsigned Line;     // Source line number.
  unsigned Column;   // Source column.
  unsigned SourceID; // Source ID number.
  MCSymbol *Label;   // Label in code ID number.
public:
  SrcLineInfo(unsigned L, unsigned C, unsigned S, MCSymbol *label)
      : Line(L), Column(C), SourceID(S), Label(label) {}

  // Accessors
  unsigned getLine() const { return Line; }
  unsigned getColumn() const { return Column; }
  unsigned getSourceID() const { return SourceID; }
  MCSymbol *getLabel() const { return Label; }
};

//===----------------------------------------------------------------------===//
/// \brief This class is used to track local variable information.
class DbgVariable {
  DIVariable Var;             // Variable Descriptor.
  DIE *TheDIE;                // Variable DIE.
  unsigned DotDebugLocOffset; // Offset in DotDebugLocEntries.
  const MachineInstr *MInsn;  // DBG_VALUE instruction of the variable.
  int FrameIndex;
  DwarfDebug *DD;

public:
  /// Construct a DbgVariable from a DIVariable.
  DbgVariable(DIVariable V, DwarfDebug *DD)
      : Var(V), TheDIE(nullptr), DotDebugLocOffset(~0U), MInsn(nullptr),
        FrameIndex(~0), DD(DD) {}

  /// Construct a DbgVariable from a DEBUG_VALUE.
  /// AbstractVar may be NULL.
  DbgVariable(const MachineInstr *DbgValue, DwarfDebug *DD)
      : Var(DbgValue->getDebugVariable()), TheDIE(nullptr),
        DotDebugLocOffset(~0U), MInsn(DbgValue), FrameIndex(~0), DD(DD) {}

  // Accessors.
  DIVariable getVariable() const { return Var; }
  void setDIE(DIE &D) { TheDIE = &D; }
  DIE *getDIE() const { return TheDIE; }
  void setDotDebugLocOffset(unsigned O) { DotDebugLocOffset = O; }
  unsigned getDotDebugLocOffset() const { return DotDebugLocOffset; }
  StringRef getName() const { return Var.getName(); }
  const MachineInstr *getMInsn() const { return MInsn; }
  int getFrameIndex() const { return FrameIndex; }
  void setFrameIndex(int FI) { FrameIndex = FI; }
  // Translate tag to proper Dwarf tag.
  dwarf::Tag getTag() const {
    if (Var.getTag() == dwarf::DW_TAG_arg_variable)
      return dwarf::DW_TAG_formal_parameter;

    return dwarf::DW_TAG_variable;
  }
  /// \brief Return true if DbgVariable is artificial.
  bool isArtificial() const {
    if (Var.isArtificial())
      return true;
    if (getType().isArtificial())
      return true;
    return false;
  }

  bool isObjectPointer() const {
    if (Var.isObjectPointer())
      return true;
    if (getType().isObjectPointer())
      return true;
    return false;
  }

  bool variableHasComplexAddress() const {
    assert(Var.isVariable() && "Invalid complex DbgVariable!");
    return Var.hasComplexAddress();
  }
  bool isBlockByrefVariable() const;
  unsigned getNumAddrElements() const {
    assert(Var.isVariable() && "Invalid complex DbgVariable!");
    return Var.getNumAddrElements();
  }
  uint64_t getAddrElement(unsigned i) const { return Var.getAddrElement(i); }
  DIType getType() const;

private:
  /// resolve - Look in the DwarfDebug map for the MDNode that
  /// corresponds to the reference.
  template <typename T> T resolve(DIRef<T> Ref) const;
};


/// \brief Helper used to pair up a symbol and its DWARF compile unit.
struct SymbolCU {
  SymbolCU(DwarfCompileUnit *CU, const MCSymbol *Sym) : Sym(Sym), CU(CU) {}
  const MCSymbol *Sym;
  DwarfCompileUnit *CU;
};

/// \brief Collects and handles dwarf debug information.
class DwarfDebug : public AsmPrinterHandler {
  // Target of Dwarf emission.
  AsmPrinter *Asm;

  // Collected machine module information.
  MachineModuleInfo *MMI;

  // All DIEValues are allocated through this allocator.
  BumpPtrAllocator DIEValueAllocator;

  // Handle to the compile unit used for the inline extension handling,
  // this is just so that the DIEValue allocator has a place to store
  // the particular elements.
  // FIXME: Store these off of DwarfDebug instead?
  DwarfCompileUnit *FirstCU;

  // Maps MDNode with its corresponding DwarfCompileUnit.
  MapVector<const MDNode *, DwarfCompileUnit *> CUMap;

  // Maps subprogram MDNode with its corresponding DwarfCompileUnit.
  DenseMap<const MDNode *, DwarfCompileUnit *> SPMap;

  // Maps a CU DIE with its corresponding DwarfCompileUnit.
  DenseMap<const DIE *, DwarfCompileUnit *> CUDieMap;

  /// Maps MDNodes for type system with the corresponding DIEs. These DIEs can
  /// be shared across CUs, that is why we keep the map here instead
  /// of in DwarfCompileUnit.
  DenseMap<const MDNode *, DIE *> MDTypeNodeToDieMap;

  // List of all labels used in aranges generation.
  std::vector<SymbolCU> ArangeLabels;

  // Size of each symbol emitted (for those symbols that have a specific size).
  DenseMap<const MCSymbol *, uint64_t> SymSize;

  // Provides a unique id per text section.
  typedef DenseMap<const MCSection *, SmallVector<SymbolCU, 8> > SectionMapType;
  SectionMapType SectionMap;

  // List of arguments for current function.
  SmallVector<DbgVariable *, 8> CurrentFnArguments;

  LexicalScopes LScopes;

  // Collection of abstract subprogram DIEs.
  DenseMap<const MDNode *, DIE *> AbstractSPDies;

  // Collection of dbg variables of a scope.
  typedef DenseMap<LexicalScope *, SmallVector<DbgVariable *, 8> >
  ScopeVariablesMap;
  ScopeVariablesMap ScopeVariables;

  // Collection of abstract variables.
  DenseMap<const MDNode *, std::unique_ptr<DbgVariable>> AbstractVariables;
  SmallVector<std::unique_ptr<DbgVariable>, 64> ConcreteVariables;

  // Collection of DebugLocEntry. Stored in a linked list so that DIELocLists
  // can refer to them in spite of insertions into this list.
  SmallVector<DebugLocList, 4> DotDebugLocEntries;

  // Collection of subprogram DIEs that are marked (at the end of the module)
  // as DW_AT_inline.
  SmallPtrSet<DIE *, 4> InlinedSubprogramDIEs;

  // This is a collection of subprogram MDNodes that are processed to
  // create DIEs.
  SmallPtrSet<const MDNode *, 16> ProcessedSPNodes;

  // Maps instruction with label emitted before instruction.
  DenseMap<const MachineInstr *, MCSymbol *> LabelsBeforeInsn;

  // Maps instruction with label emitted after instruction.
  DenseMap<const MachineInstr *, MCSymbol *> LabelsAfterInsn;

  // History of DBG_VALUE and clobber instructions for each user variable.
  // Variables are listed in order of appearance.
  DbgValueHistoryMap DbgValues;

  // Previous instruction's location information. This is used to determine
  // label location to indicate scope boundries in dwarf debug info.
  DebugLoc PrevInstLoc;
  MCSymbol *PrevLabel;

  // This location indicates end of function prologue and beginning of function
  // body.
  DebugLoc PrologEndLoc;

  // If nonnull, stores the current machine function we're processing.
  const MachineFunction *CurFn;

  // If nonnull, stores the current machine instruction we're processing.
  const MachineInstr *CurMI;

  // If nonnull, stores the section that the previous function was allocated to
  // emitting.
  const MCSection *PrevSection;

  // If nonnull, stores the CU in which the previous subprogram was contained.
  const DwarfCompileUnit *PrevCU;

  // Section Symbols: these are assembler temporary labels that are emitted at
  // the beginning of each supported dwarf section.  These are used to form
  // section offsets and are created by EmitSectionLabels.
  MCSymbol *DwarfInfoSectionSym, *DwarfAbbrevSectionSym;
  MCSymbol *DwarfStrSectionSym, *TextSectionSym, *DwarfDebugRangeSectionSym;
  MCSymbol *DwarfDebugLocSectionSym, *DwarfLineSectionSym, *DwarfAddrSectionSym;
  MCSymbol *FunctionBeginSym, *FunctionEndSym;
  MCSymbol *DwarfInfoDWOSectionSym, *DwarfAbbrevDWOSectionSym;
  MCSymbol *DwarfTypesDWOSectionSym;
  MCSymbol *DwarfStrDWOSectionSym;
  MCSymbol *DwarfGnuPubNamesSectionSym, *DwarfGnuPubTypesSectionSym;

  // As an optimization, there is no need to emit an entry in the directory
  // table for the same directory as DW_AT_comp_dir.
  StringRef CompilationDir;

  // Counter for assigning globally unique IDs for ranges.
  unsigned GlobalRangeCount;

  // Holder for the file specific debug information.
  DwarfFile InfoHolder;

  // Holders for the various debug information flags that we might need to
  // have exposed. See accessor functions below for description.

  // Holder for imported entities.
  typedef SmallVector<std::pair<const MDNode *, const MDNode *>, 32>
  ImportedEntityMap;
  ImportedEntityMap ScopesWithImportedEntities;

  // Map from MDNodes for user-defined types to the type units that describe
  // them.
  DenseMap<const MDNode *, const DwarfTypeUnit *> DwarfTypeUnits;

  SmallVector<std::pair<std::unique_ptr<DwarfTypeUnit>, DICompositeType>, 1> TypeUnitsUnderConstruction;

  // Whether to emit the pubnames/pubtypes sections.
  bool HasDwarfPubSections;

  // Whether or not to use AT_ranges for compilation units.
  bool HasCURanges;

  // Whether we emitted a function into a section other than the default
  // text.
  bool UsedNonDefaultText;

  // Version of dwarf we're emitting.
  unsigned DwarfVersion;

  // Maps from a type identifier to the actual MDNode.
  DITypeIdentifierMap TypeIdentifierMap;

  // DWARF5 Experimental Options
  bool HasDwarfAccelTables;
  bool HasSplitDwarf;

  // Separated Dwarf Variables
  // In general these will all be for bits that are left in the
  // original object file, rather than things that are meant
  // to be in the .dwo sections.

  // Holder for the skeleton information.
  DwarfFile SkeletonHolder;

  /// Store file names for type units under fission in a line table header that
  /// will be emitted into debug_line.dwo.
  // FIXME: replace this with a map from comp_dir to table so that we can emit
  // multiple tables during LTO each of which uses directory 0, referencing the
  // comp_dir of all the type units that use it.
  MCDwarfDwoLineTable SplitTypeUnitFileTable;

  // True iff there are multiple CUs in this module.
  bool SingleCU;

  AddressPool AddrPool;

  DwarfAccelTable AccelNames;
  DwarfAccelTable AccelObjC;
  DwarfAccelTable AccelNamespace;
  DwarfAccelTable AccelTypes;

  DenseMap<const Function *, DISubprogram> FunctionDIs;

  MCDwarfDwoLineTable *getDwoLineTable(const DwarfCompileUnit &);

  void addScopeVariable(LexicalScope *LS, DbgVariable *Var);

  const SmallVectorImpl<std::unique_ptr<DwarfUnit>> &getUnits() {
    return InfoHolder.getUnits();
  }

  /// \brief Find abstract variable associated with Var.
  DbgVariable *getExistingAbstractVariable(const DIVariable &DV,
                                           DIVariable &Cleansed);
  DbgVariable *getExistingAbstractVariable(const DIVariable &DV);
  void createAbstractVariable(const DIVariable &DV, LexicalScope *Scope);
  void ensureAbstractVariableIsCreated(const DIVariable &Var,
                                       const MDNode *Scope);
  void ensureAbstractVariableIsCreatedIfScoped(const DIVariable &Var,
                                               const MDNode *Scope);

  /// \brief Find DIE for the given subprogram and attach appropriate
  /// DW_AT_low_pc and DW_AT_high_pc attributes. If there are global
  /// variables in this scope then create and insert DIEs for these
  /// variables.
  DIE &updateSubprogramScopeDIE(DwarfCompileUnit &SPCU, DISubprogram SP);

  /// \brief A helper function to check whether the DIE for a given Scope is
  /// going to be null.
  bool isLexicalScopeDIENull(LexicalScope *Scope);

  /// \brief A helper function to construct a RangeSpanList for a given
  /// lexical scope.
  void addScopeRangeList(DwarfCompileUnit &TheCU, DIE &ScopeDIE,
                         const SmallVectorImpl<InsnRange> &Range);

  /// \brief Construct new DW_TAG_lexical_block for this scope and
  /// attach DW_AT_low_pc/DW_AT_high_pc labels.
  std::unique_ptr<DIE> constructLexicalScopeDIE(DwarfCompileUnit &TheCU,
                                                LexicalScope *Scope);

  /// \brief This scope represents inlined body of a function. Construct
  /// DIE to represent this concrete inlined copy of the function.
  std::unique_ptr<DIE> constructInlinedScopeDIE(DwarfCompileUnit &TheCU,
                                                LexicalScope *Scope);

  /// \brief Construct a DIE for this scope.
  std::unique_ptr<DIE> constructScopeDIE(DwarfCompileUnit &TheCU,
                                         LexicalScope *Scope);
  void createAndAddScopeChildren(DwarfCompileUnit &TheCU, LexicalScope *Scope,
                                 DIE &ScopeDIE);
  /// \brief Construct a DIE for this abstract scope.
  void constructAbstractSubprogramScopeDIE(DwarfCompileUnit &TheCU,
                                           LexicalScope *Scope);
  /// \brief Construct a DIE for this subprogram scope.
  DIE &constructSubprogramScopeDIE(DwarfCompileUnit &TheCU,
                                   LexicalScope *Scope);
  /// A helper function to create children of a Scope DIE.
  DIE *createScopeChildrenDIE(DwarfCompileUnit &TheCU, LexicalScope *Scope,
                              SmallVectorImpl<std::unique_ptr<DIE>> &Children);

  /// \brief Emit initial Dwarf sections with a label at the start of each one.
  void emitSectionLabels();

  /// \brief Compute the size and offset of a DIE given an incoming Offset.
  unsigned computeSizeAndOffset(DIE *Die, unsigned Offset);

  /// \brief Compute the size and offset of all the DIEs.
  void computeSizeAndOffsets();

  /// \brief Collect info for variables that were optimized out.
  void collectDeadVariables();

  void finishVariableDefinitions();

  void finishSubprogramDefinitions();

  /// \brief Finish off debug information after all functions have been
  /// processed.
  void finalizeModuleInfo();

  /// \brief Emit labels to close any remaining sections that have been left
  /// open.
  void endSections();

  /// \brief Emit the debug info section.
  void emitDebugInfo();

  /// \brief Emit the abbreviation section.
  void emitAbbreviations();

  /// \brief Emit the last address of the section and the end of
  /// the line matrix.
  void emitEndOfLineMatrix(unsigned SectionEnd);

  /// \brief Emit visible names into a hashed accelerator table section.
  void emitAccelNames();

  /// \brief Emit objective C classes and categories into a hashed
  /// accelerator table section.
  void emitAccelObjC();

  /// \brief Emit namespace dies into a hashed accelerator table.
  void emitAccelNamespaces();

  /// \brief Emit type dies into a hashed accelerator table.
  void emitAccelTypes();

  /// \brief Emit visible names into a debug pubnames section.
  /// \param GnuStyle determines whether or not we want to emit
  /// additional information into the table ala newer gcc for gdb
  /// index.
  void emitDebugPubNames(bool GnuStyle = false);

  /// \brief Emit visible types into a debug pubtypes section.
  /// \param GnuStyle determines whether or not we want to emit
  /// additional information into the table ala newer gcc for gdb
  /// index.
  void emitDebugPubTypes(bool GnuStyle = false);

  void
  emitDebugPubSection(bool GnuStyle, const MCSection *PSec, StringRef Name,
                      const StringMap<const DIE *> &(DwarfUnit::*Accessor)()
                      const);

  /// \brief Emit visible names into a debug str section.
  void emitDebugStr();

  /// \brief Emit visible names into a debug loc section.
  void emitDebugLoc();

  /// \brief Emit visible names into a debug loc dwo section.
  void emitDebugLocDWO();

  /// \brief Emit visible names into a debug aranges section.
  void emitDebugARanges();

  /// \brief Emit visible names into a debug ranges section.
  void emitDebugRanges();

  /// \brief Emit inline info using custom format.
  void emitDebugInlineInfo();

  /// DWARF 5 Experimental Split Dwarf Emitters

  /// \brief Initialize common features of skeleton units.
  void initSkeletonUnit(const DwarfUnit &U, DIE &Die,
                        std::unique_ptr<DwarfUnit> NewU);

  /// \brief Construct the split debug info compile unit for the debug info
  /// section.
  DwarfCompileUnit &constructSkeletonCU(const DwarfCompileUnit &CU);

  /// \brief Construct the split debug info compile unit for the debug info
  /// section.
  DwarfTypeUnit &constructSkeletonTU(DwarfTypeUnit &TU);

  /// \brief Emit the debug info dwo section.
  void emitDebugInfoDWO();

  /// \brief Emit the debug abbrev dwo section.
  void emitDebugAbbrevDWO();

  /// \brief Emit the debug line dwo section.
  void emitDebugLineDWO();

  /// \brief Emit the debug str dwo section.
  void emitDebugStrDWO();

  /// Flags to let the linker know we have emitted new style pubnames. Only
  /// emit it here if we don't have a skeleton CU for split dwarf.
  void addGnuPubAttributes(DwarfUnit &U, DIE &D) const;

  /// \brief Create new DwarfCompileUnit for the given metadata node with tag
  /// DW_TAG_compile_unit.
  DwarfCompileUnit &constructDwarfCompileUnit(DICompileUnit DIUnit);

  /// \brief Construct imported_module or imported_declaration DIE.
  void constructImportedEntityDIE(DwarfCompileUnit &TheCU, const MDNode *N);

  /// \brief Construct import_module DIE.
  void constructImportedEntityDIE(DwarfCompileUnit &TheCU, const MDNode *N,
                                  DIE &Context);

  /// \brief Construct import_module DIE.
  void constructImportedEntityDIE(DwarfCompileUnit &TheCU,
                                  const DIImportedEntity &Module, DIE &Context);

  /// \brief Register a source line with debug info. Returns the unique
  /// label that was emitted and which provides correspondence to the
  /// source line list.
  void recordSourceLine(unsigned Line, unsigned Col, const MDNode *Scope,
                        unsigned Flags);

  /// \brief Indentify instructions that are marking the beginning of or
  /// ending of a scope.
  void identifyScopeMarkers();

  /// \brief If Var is an current function argument that add it in
  /// CurrentFnArguments list.
  bool addCurrentFnArgument(DbgVariable *Var, LexicalScope *Scope);

  /// \brief Populate LexicalScope entries with variables' info.
  void collectVariableInfo(SmallPtrSet<const MDNode *, 16> &ProcessedVars);

  /// \brief Build the location list for all DBG_VALUEs in the
  /// function that describe the same variable.
  void buildLocationList(SmallVectorImpl<DebugLocEntry> &DebugLoc,
                         const DbgValueHistoryMap::InstrRanges &Ranges);

  /// \brief Collect variable information from the side table maintained
  /// by MMI.
  void collectVariableInfoFromMMITable(SmallPtrSet<const MDNode *, 16> &P);

  /// \brief Ensure that a label will be emitted before MI.
  void requestLabelBeforeInsn(const MachineInstr *MI) {
    LabelsBeforeInsn.insert(std::make_pair(MI, nullptr));
  }

  /// \brief Return Label preceding the instruction.
  MCSymbol *getLabelBeforeInsn(const MachineInstr *MI);

  /// \brief Ensure that a label will be emitted after MI.
  void requestLabelAfterInsn(const MachineInstr *MI) {
    LabelsAfterInsn.insert(std::make_pair(MI, nullptr));
  }

  /// \brief Return Label immediately following the instruction.
  MCSymbol *getLabelAfterInsn(const MachineInstr *MI);

  void attachRangesOrLowHighPC(DwarfCompileUnit &Unit, DIE &D,
                               const SmallVectorImpl<InsnRange> &Ranges);
  void attachLowHighPC(DwarfCompileUnit &Unit, DIE &D, MCSymbol *Begin,
                       MCSymbol *End);

public:
  //===--------------------------------------------------------------------===//
  // Main entry points.
  //
  DwarfDebug(AsmPrinter *A, Module *M);

  ~DwarfDebug() override;

  void insertDIE(const MDNode *TypeMD, DIE *Die) {
    MDTypeNodeToDieMap.insert(std::make_pair(TypeMD, Die));
  }
  DIE *getDIE(const MDNode *TypeMD) {
    return MDTypeNodeToDieMap.lookup(TypeMD);
  }

  /// \brief Emit all Dwarf sections that should come prior to the
  /// content.
  void beginModule();

  /// \brief Emit all Dwarf sections that should come after the content.
  void endModule() override;

  /// \brief Gather pre-function debug information.
  void beginFunction(const MachineFunction *MF) override;

  /// \brief Gather and emit post-function debug information.
  void endFunction(const MachineFunction *MF) override;

  /// \brief Process beginning of an instruction.
  void beginInstruction(const MachineInstr *MI) override;

  /// \brief Process end of an instruction.
  void endInstruction() override;

  /// \brief Add a DIE to the set of types that we're going to pull into
  /// type units.
  void addDwarfTypeUnitType(DwarfCompileUnit &CU, StringRef Identifier,
                            DIE &Die, DICompositeType CTy);

  /// \brief Add a label so that arange data can be generated for it.
  void addArangeLabel(SymbolCU SCU) { ArangeLabels.push_back(SCU); }

  /// \brief For symbols that have a size designated (e.g. common symbols),
  /// this tracks that size.
  void setSymbolSize(const MCSymbol *Sym, uint64_t Size) override {
    SymSize[Sym] = Size;
  }

  /// \brief Recursively Emits a debug information entry.
  void emitDIE(DIE &Die);

  // Experimental DWARF5 features.

  /// \brief Returns whether or not to emit tables that dwarf consumers can
  /// use to accelerate lookup.
  bool useDwarfAccelTables() const { return HasDwarfAccelTables; }

  /// \brief Returns whether or not to change the current debug info for the
  /// split dwarf proposal support.
  bool useSplitDwarf() const { return HasSplitDwarf; }

  /// Returns the Dwarf Version.
  unsigned getDwarfVersion() const { return DwarfVersion; }

  /// Returns the section symbol for the .debug_loc section.
  MCSymbol *getDebugLocSym() const { return DwarfDebugLocSectionSym; }

  /// Returns the previous section that was emitted into.
  const MCSection *getPrevSection() const { return PrevSection; }

  /// Returns the previous CU that was being updated
  const DwarfCompileUnit *getPrevCU() const { return PrevCU; }

  /// Returns the entries for the .debug_loc section.
  const SmallVectorImpl<DebugLocList> &
  getDebugLocEntries() const {
    return DotDebugLocEntries;
  }

  /// \brief Emit an entry for the debug loc section. This can be used to
  /// handle an entry that's going to be emitted into the debug loc section.
  void emitDebugLocEntry(ByteStreamer &Streamer, const DebugLocEntry &Entry);
  /// \brief emit a single value for the debug loc section.
  void emitDebugLocValue(ByteStreamer &Streamer,
                         const DebugLocEntry::Value &Value);
  /// Emits an optimal (=sorted) sequence of DW_OP_pieces.
  void emitLocPieces(ByteStreamer &Streamer,
                     const DITypeIdentifierMap &Map,
                     ArrayRef<DebugLocEntry::Value> Values);

  /// Emit the location for a debug loc entry, including the size header.
  void emitDebugLocEntryLocation(const DebugLocEntry &Entry);

  /// Find the MDNode for the given reference.
  template <typename T> T resolve(DIRef<T> Ref) const {
    return Ref.resolve(TypeIdentifierMap);
  }

  /// \brief Return the TypeIdentifierMap.
  const DITypeIdentifierMap &getTypeIdentifierMap() const {
    return TypeIdentifierMap;
  }

  /// Find the DwarfCompileUnit for the given CU Die.
  DwarfCompileUnit *lookupUnit(const DIE *CU) const {
    return CUDieMap.lookup(CU);
  }
  /// isSubprogramContext - Return true if Context is either a subprogram
  /// or another context nested inside a subprogram.
  bool isSubprogramContext(const MDNode *Context);

  void addSubprogramNames(DISubprogram SP, DIE &Die);

  AddressPool &getAddressPool() { return AddrPool; }

  void addAccelName(StringRef Name, const DIE &Die);

  void addAccelObjC(StringRef Name, const DIE &Die);

  void addAccelNamespace(StringRef Name, const DIE &Die);

  void addAccelType(StringRef Name, const DIE &Die, char Flags);
};
} // End of namespace llvm

#endif
