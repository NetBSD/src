//===-- SymbolDumper.cpp - CodeView symbol info dumper ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/DebugInfo/CodeView/SymbolDumper.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/DebugInfo/CodeView/CVSymbolVisitor.h"
#include "llvm/DebugInfo/CodeView/EnumTables.h"
#include "llvm/DebugInfo/CodeView/SymbolDeserializer.h"
#include "llvm/DebugInfo/CodeView/SymbolDumpDelegate.h"
#include "llvm/DebugInfo/CodeView/SymbolRecord.h"
#include "llvm/DebugInfo/CodeView/SymbolVisitorCallbackPipeline.h"
#include "llvm/DebugInfo/CodeView/SymbolVisitorCallbacks.h"
#include "llvm/DebugInfo/CodeView/TypeDumper.h"
#include "llvm/DebugInfo/CodeView/TypeIndex.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/ScopedPrinter.h"

#include <system_error>

using namespace llvm;
using namespace llvm::codeview;

namespace {
/// Use this private dumper implementation to keep implementation details about
/// the visitor out of SymbolDumper.h.
class CVSymbolDumperImpl : public SymbolVisitorCallbacks {
public:
  CVSymbolDumperImpl(CVTypeDumper &CVTD, SymbolDumpDelegate *ObjDelegate,
                     ScopedPrinter &W, bool PrintRecordBytes)
      : CVTD(CVTD), ObjDelegate(ObjDelegate), W(W),
        PrintRecordBytes(PrintRecordBytes), InFunctionScope(false) {}

/// CVSymbolVisitor overrides.
#define SYMBOL_RECORD(EnumName, EnumVal, Name)                                 \
  Error visitKnownRecord(CVSymbol &CVR, Name &Record) override;
#define SYMBOL_RECORD_ALIAS(EnumName, EnumVal, Name, AliasName)
#include "llvm/DebugInfo/CodeView/CVSymbolTypes.def"

  Error visitSymbolBegin(CVSymbol &Record) override;
  Error visitSymbolEnd(CVSymbol &Record) override;
  Error visitUnknownSymbol(CVSymbol &Record) override;

private:
  void printLocalVariableAddrRange(const LocalVariableAddrRange &Range,
                                   uint32_t RelocationOffset);
  void printLocalVariableAddrGap(ArrayRef<LocalVariableAddrGap> Gaps);

  CVTypeDumper &CVTD;
  SymbolDumpDelegate *ObjDelegate;
  ScopedPrinter &W;

  bool PrintRecordBytes;
  bool InFunctionScope;
};
}

void CVSymbolDumperImpl::printLocalVariableAddrRange(
    const LocalVariableAddrRange &Range, uint32_t RelocationOffset) {
  DictScope S(W, "LocalVariableAddrRange");
  if (ObjDelegate)
    ObjDelegate->printRelocatedField("OffsetStart", RelocationOffset,
                                     Range.OffsetStart);
  W.printHex("ISectStart", Range.ISectStart);
  W.printHex("Range", Range.Range);
}

void CVSymbolDumperImpl::printLocalVariableAddrGap(
    ArrayRef<LocalVariableAddrGap> Gaps) {
  for (auto &Gap : Gaps) {
    ListScope S(W, "LocalVariableAddrGap");
    W.printHex("GapStartOffset", Gap.GapStartOffset);
    W.printHex("Range", Gap.Range);
  }
}

Error CVSymbolDumperImpl::visitSymbolBegin(CVSymbol &CVR) {
  return Error::success();
}

Error CVSymbolDumperImpl::visitSymbolEnd(CVSymbol &CVR) {
  if (PrintRecordBytes && ObjDelegate)
    ObjDelegate->printBinaryBlockWithRelocs("SymData", CVR.content());
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, BlockSym &Block) {
  DictScope S(W, "BlockStart");

  StringRef LinkageName;
  W.printHex("PtrParent", Block.Parent);
  W.printHex("PtrEnd", Block.End);
  W.printHex("CodeSize", Block.CodeSize);
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset", Block.getRelocationOffset(),
                                     Block.CodeOffset, &LinkageName);
  }
  W.printHex("Segment", Block.Segment);
  W.printString("BlockName", Block.Name);
  W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, Thunk32Sym &Thunk) {
  DictScope S(W, "Thunk32");
  W.printNumber("Parent", Thunk.Parent);
  W.printNumber("End", Thunk.End);
  W.printNumber("Next", Thunk.Next);
  W.printNumber("Off", Thunk.Offset);
  W.printNumber("Seg", Thunk.Segment);
  W.printNumber("Len", Thunk.Length);
  W.printEnum("Ordinal", uint8_t(Thunk.Thunk), getThunkOrdinalNames());
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           TrampolineSym &Tramp) {
  DictScope S(W, "Trampoline");
  W.printEnum("Type", uint16_t(Tramp.Type), getTrampolineNames());
  W.printNumber("Size", Tramp.Size);
  W.printNumber("ThunkOff", Tramp.ThunkOffset);
  W.printNumber("TargetOff", Tramp.TargetOffset);
  W.printNumber("ThunkSection", Tramp.ThunkSection);
  W.printNumber("TargetSection", Tramp.TargetSection);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, SectionSym &Section) {
  DictScope S(W, "Section");
  W.printNumber("SectionNumber", Section.SectionNumber);
  W.printNumber("Alignment", Section.Alignment);
  W.printNumber("Rva", Section.Rva);
  W.printNumber("Length", Section.Length);
  W.printFlags("Characteristics", Section.Characteristics,
               getImageSectionCharacteristicNames(),
               COFF::SectionCharacteristics(0x00F00000));

  W.printString("Name", Section.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           CoffGroupSym &CoffGroup) {
  DictScope S(W, "COFF Group");
  W.printNumber("Size", CoffGroup.Size);
  W.printFlags("Characteristics", CoffGroup.Characteristics,
               getImageSectionCharacteristicNames(),
               COFF::SectionCharacteristics(0x00F00000));
  W.printNumber("Offset", CoffGroup.Offset);
  W.printNumber("Segment", CoffGroup.Segment);
  W.printString("Name", CoffGroup.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           BPRelativeSym &BPRel) {
  DictScope S(W, "BPRelativeSym");

  W.printNumber("Offset", BPRel.Offset);
  CVTD.printTypeIndex("Type", BPRel.Type);
  W.printString("VarName", BPRel.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           BuildInfoSym &BuildInfo) {
  DictScope S(W, "BuildInfo");

  W.printNumber("BuildId", BuildInfo.BuildId);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           CallSiteInfoSym &CallSiteInfo) {
  DictScope S(W, "CallSiteInfo");

  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset",
                                     CallSiteInfo.getRelocationOffset(),
                                     CallSiteInfo.CodeOffset, &LinkageName);
  }
  W.printHex("Segment", CallSiteInfo.Segment);
  CVTD.printTypeIndex("Type", CallSiteInfo.Type);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           EnvBlockSym &EnvBlock) {
  DictScope S(W, "EnvBlock");

  ListScope L(W, "Entries");
  for (auto Entry : EnvBlock.Fields) {
    W.printString(Entry);
  }
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           FileStaticSym &FileStatic) {
  DictScope S(W, "FileStatic");
  W.printNumber("Index", FileStatic.Index);
  W.printNumber("ModFilenameOffset", FileStatic.ModFilenameOffset);
  W.printFlags("Flags", uint16_t(FileStatic.Flags), getLocalFlagNames());
  W.printString("Name", FileStatic.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, ExportSym &Export) {
  DictScope S(W, "Export");
  W.printNumber("Ordinal", Export.Ordinal);
  W.printFlags("Flags", uint16_t(Export.Flags), getExportSymFlagNames());
  W.printString("Name", Export.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           Compile2Sym &Compile2) {
  DictScope S(W, "CompilerFlags2");

  W.printEnum("Language", Compile2.getLanguage(), getSourceLanguageNames());
  W.printFlags("Flags", Compile2.getFlags(), getCompileSym2FlagNames());
  W.printEnum("Machine", unsigned(Compile2.Machine), getCPUTypeNames());
  std::string FrontendVersion;
  {
    raw_string_ostream Out(FrontendVersion);
    Out << Compile2.VersionFrontendMajor << '.' << Compile2.VersionFrontendMinor
        << '.' << Compile2.VersionFrontendBuild;
  }
  std::string BackendVersion;
  {
    raw_string_ostream Out(BackendVersion);
    Out << Compile2.VersionBackendMajor << '.' << Compile2.VersionBackendMinor
        << '.' << Compile2.VersionBackendBuild;
  }
  W.printString("FrontendVersion", FrontendVersion);
  W.printString("BackendVersion", BackendVersion);
  W.printString("VersionName", Compile2.Version);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           Compile3Sym &Compile3) {
  DictScope S(W, "CompilerFlags3");

  W.printEnum("Language", Compile3.getLanguage(), getSourceLanguageNames());
  W.printFlags("Flags", Compile3.getFlags(), getCompileSym3FlagNames());
  W.printEnum("Machine", unsigned(Compile3.Machine), getCPUTypeNames());
  std::string FrontendVersion;
  {
    raw_string_ostream Out(FrontendVersion);
    Out << Compile3.VersionFrontendMajor << '.' << Compile3.VersionFrontendMinor
        << '.' << Compile3.VersionFrontendBuild << '.'
        << Compile3.VersionFrontendQFE;
  }
  std::string BackendVersion;
  {
    raw_string_ostream Out(BackendVersion);
    Out << Compile3.VersionBackendMajor << '.' << Compile3.VersionBackendMinor
        << '.' << Compile3.VersionBackendBuild << '.'
        << Compile3.VersionBackendQFE;
  }
  W.printString("FrontendVersion", FrontendVersion);
  W.printString("BackendVersion", BackendVersion);
  W.printString("VersionName", Compile3.Version);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           ConstantSym &Constant) {
  DictScope S(W, "Constant");

  CVTD.printTypeIndex("Type", Constant.Type);
  W.printNumber("Value", Constant.Value);
  W.printString("Name", Constant.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, DataSym &Data) {
  DictScope S(W, "DataSym");

  W.printEnum("Kind", uint16_t(CVR.kind()), getSymbolTypeNames());
  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("DataOffset", Data.getRelocationOffset(),
                                     Data.DataOffset, &LinkageName);
  }
  CVTD.printTypeIndex("Type", Data.Type);
  W.printString("DisplayName", Data.Name);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR,
    DefRangeFramePointerRelFullScopeSym &DefRangeFramePointerRelFullScope) {
  DictScope S(W, "DefRangeFramePointerRelFullScope");
  W.printNumber("Offset", DefRangeFramePointerRelFullScope.Offset);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, DefRangeFramePointerRelSym &DefRangeFramePointerRel) {
  DictScope S(W, "DefRangeFramePointerRel");

  W.printNumber("Offset", DefRangeFramePointerRel.Offset);
  printLocalVariableAddrRange(DefRangeFramePointerRel.Range,
                              DefRangeFramePointerRel.getRelocationOffset());
  printLocalVariableAddrGap(DefRangeFramePointerRel.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, DefRangeRegisterRelSym &DefRangeRegisterRel) {
  DictScope S(W, "DefRangeRegisterRel");

  W.printNumber("BaseRegister", DefRangeRegisterRel.Hdr.Register);
  W.printBoolean("HasSpilledUDTMember",
                 DefRangeRegisterRel.hasSpilledUDTMember());
  W.printNumber("OffsetInParent", DefRangeRegisterRel.offsetInParent());
  W.printNumber("BasePointerOffset", DefRangeRegisterRel.Hdr.BasePointerOffset);
  printLocalVariableAddrRange(DefRangeRegisterRel.Range,
                              DefRangeRegisterRel.getRelocationOffset());
  printLocalVariableAddrGap(DefRangeRegisterRel.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, DefRangeRegisterSym &DefRangeRegister) {
  DictScope S(W, "DefRangeRegister");

  W.printNumber("Register", DefRangeRegister.Hdr.Register);
  W.printNumber("MayHaveNoName", DefRangeRegister.Hdr.MayHaveNoName);
  printLocalVariableAddrRange(DefRangeRegister.Range,
                              DefRangeRegister.getRelocationOffset());
  printLocalVariableAddrGap(DefRangeRegister.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, DefRangeSubfieldRegisterSym &DefRangeSubfieldRegister) {
  DictScope S(W, "DefRangeSubfieldRegister");

  W.printNumber("Register", DefRangeSubfieldRegister.Hdr.Register);
  W.printNumber("MayHaveNoName", DefRangeSubfieldRegister.Hdr.MayHaveNoName);
  W.printNumber("OffsetInParent", DefRangeSubfieldRegister.Hdr.OffsetInParent);
  printLocalVariableAddrRange(DefRangeSubfieldRegister.Range,
                              DefRangeSubfieldRegister.getRelocationOffset());
  printLocalVariableAddrGap(DefRangeSubfieldRegister.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, DefRangeSubfieldSym &DefRangeSubfield) {
  DictScope S(W, "DefRangeSubfield");

  if (ObjDelegate) {
    StringRef StringTable = ObjDelegate->getStringTable();
    auto ProgramStringTableOffset = DefRangeSubfield.Program;
    if (ProgramStringTableOffset >= StringTable.size())
      return llvm::make_error<CodeViewError>(
          "String table offset outside of bounds of String Table!");
    StringRef Program =
        StringTable.drop_front(ProgramStringTableOffset).split('\0').first;
    W.printString("Program", Program);
  }
  W.printNumber("OffsetInParent", DefRangeSubfield.OffsetInParent);
  printLocalVariableAddrRange(DefRangeSubfield.Range,
                              DefRangeSubfield.getRelocationOffset());
  printLocalVariableAddrGap(DefRangeSubfield.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           DefRangeSym &DefRange) {
  DictScope S(W, "DefRange");

  if (ObjDelegate) {
    StringRef StringTable = ObjDelegate->getStringTable();
    auto ProgramStringTableOffset = DefRange.Program;
    if (ProgramStringTableOffset >= StringTable.size())
      return llvm::make_error<CodeViewError>(
          "String table offset outside of bounds of String Table!");
    StringRef Program =
        StringTable.drop_front(ProgramStringTableOffset).split('\0').first;
    W.printString("Program", Program);
  }
  printLocalVariableAddrRange(DefRange.Range, DefRange.getRelocationOffset());
  printLocalVariableAddrGap(DefRange.Gaps);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           FrameCookieSym &FrameCookie) {
  DictScope S(W, "FrameCookie");

  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset",
                                     FrameCookie.getRelocationOffset(),
                                     FrameCookie.CodeOffset, &LinkageName);
  }
  W.printHex("Register", FrameCookie.Register);
  W.printEnum("CookieKind", uint16_t(FrameCookie.CookieKind),
              getFrameCookieKindNames());
  W.printHex("Flags", FrameCookie.Flags);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           FrameProcSym &FrameProc) {
  DictScope S(W, "FrameProc");

  W.printHex("TotalFrameBytes", FrameProc.TotalFrameBytes);
  W.printHex("PaddingFrameBytes", FrameProc.PaddingFrameBytes);
  W.printHex("OffsetToPadding", FrameProc.OffsetToPadding);
  W.printHex("BytesOfCalleeSavedRegisters",
             FrameProc.BytesOfCalleeSavedRegisters);
  W.printHex("OffsetOfExceptionHandler", FrameProc.OffsetOfExceptionHandler);
  W.printHex("SectionIdOfExceptionHandler",
             FrameProc.SectionIdOfExceptionHandler);
  W.printFlags("Flags", static_cast<uint32_t>(FrameProc.Flags),
               getFrameProcSymFlagNames());
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(
    CVSymbol &CVR, HeapAllocationSiteSym &HeapAllocSite) {
  DictScope S(W, "HeapAllocationSite");

  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset",
                                     HeapAllocSite.getRelocationOffset(),
                                     HeapAllocSite.CodeOffset, &LinkageName);
  }
  W.printHex("Segment", HeapAllocSite.Segment);
  W.printHex("CallInstructionSize", HeapAllocSite.CallInstructionSize);
  CVTD.printTypeIndex("Type", HeapAllocSite.Type);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           InlineSiteSym &InlineSite) {
  DictScope S(W, "InlineSite");

  W.printHex("PtrParent", InlineSite.Parent);
  W.printHex("PtrEnd", InlineSite.End);
  CVTD.printTypeIndex("Inlinee", InlineSite.Inlinee);

  ListScope BinaryAnnotations(W, "BinaryAnnotations");
  for (auto &Annotation : InlineSite.annotations()) {
    switch (Annotation.OpCode) {
    case BinaryAnnotationsOpCode::Invalid:
      return llvm::make_error<CodeViewError>(
          "Invalid binary annotation opcode!");
    case BinaryAnnotationsOpCode::CodeOffset:
    case BinaryAnnotationsOpCode::ChangeCodeOffset:
    case BinaryAnnotationsOpCode::ChangeCodeLength:
      W.printHex(Annotation.Name, Annotation.U1);
      break;
    case BinaryAnnotationsOpCode::ChangeCodeOffsetBase:
    case BinaryAnnotationsOpCode::ChangeLineEndDelta:
    case BinaryAnnotationsOpCode::ChangeRangeKind:
    case BinaryAnnotationsOpCode::ChangeColumnStart:
    case BinaryAnnotationsOpCode::ChangeColumnEnd:
      W.printNumber(Annotation.Name, Annotation.U1);
      break;
    case BinaryAnnotationsOpCode::ChangeLineOffset:
    case BinaryAnnotationsOpCode::ChangeColumnEndDelta:
      W.printNumber(Annotation.Name, Annotation.S1);
      break;
    case BinaryAnnotationsOpCode::ChangeFile:
      if (ObjDelegate) {
        W.printHex("ChangeFile",
                   ObjDelegate->getFileNameForFileOffset(Annotation.U1),
                   Annotation.U1);
      } else {
        W.printHex("ChangeFile", Annotation.U1);
      }

      break;
    case BinaryAnnotationsOpCode::ChangeCodeOffsetAndLineOffset: {
      W.startLine() << "ChangeCodeOffsetAndLineOffset: {CodeOffset: "
                    << W.hex(Annotation.U1) << ", LineOffset: " << Annotation.S1
                    << "}\n";
      break;
    }
    case BinaryAnnotationsOpCode::ChangeCodeLengthAndCodeOffset: {
      W.startLine() << "ChangeCodeLengthAndCodeOffset: {CodeOffset: "
                    << W.hex(Annotation.U2)
                    << ", Length: " << W.hex(Annotation.U1) << "}\n";
      break;
    }
    }
  }
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           RegisterSym &Register) {
  DictScope S(W, "RegisterSym");
  W.printNumber("Type", Register.Index);
  W.printEnum("Seg", uint16_t(Register.Register), getRegisterNames());
  W.printString("Name", Register.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, PublicSym32 &Public) {
  DictScope S(W, "PublicSym");
  W.printNumber("Type", Public.Index);
  W.printNumber("Seg", Public.Segment);
  W.printNumber("Off", Public.Offset);
  W.printString("Name", Public.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, ProcRefSym &ProcRef) {
  DictScope S(W, "ProcRef");
  W.printNumber("SumName", ProcRef.SumName);
  W.printNumber("SymOffset", ProcRef.SymOffset);
  W.printNumber("Mod", ProcRef.Module);
  W.printString("Name", ProcRef.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, LabelSym &Label) {
  DictScope S(W, "Label");

  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset", Label.getRelocationOffset(),
                                     Label.CodeOffset, &LinkageName);
  }
  W.printHex("Segment", Label.Segment);
  W.printHex("Flags", uint8_t(Label.Flags));
  W.printFlags("Flags", uint8_t(Label.Flags), getProcSymFlagNames());
  W.printString("DisplayName", Label.Name);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, LocalSym &Local) {
  DictScope S(W, "Local");

  CVTD.printTypeIndex("Type", Local.Type);
  W.printFlags("Flags", uint16_t(Local.Flags), getLocalFlagNames());
  W.printString("VarName", Local.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, ObjNameSym &ObjName) {
  DictScope S(W, "ObjectName");

  W.printHex("Signature", ObjName.Signature);
  W.printString("ObjectName", ObjName.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, ProcSym &Proc) {
  DictScope S(W, "ProcStart");

  if (InFunctionScope)
    return llvm::make_error<CodeViewError>(
        "Visiting a ProcSym while inside function scope!");

  InFunctionScope = true;

  StringRef LinkageName;
  W.printEnum("Kind", uint16_t(CVR.kind()), getSymbolTypeNames());
  W.printHex("PtrParent", Proc.Parent);
  W.printHex("PtrEnd", Proc.End);
  W.printHex("PtrNext", Proc.Next);
  W.printHex("CodeSize", Proc.CodeSize);
  W.printHex("DbgStart", Proc.DbgStart);
  W.printHex("DbgEnd", Proc.DbgEnd);
  CVTD.printTypeIndex("FunctionType", Proc.FunctionType);
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("CodeOffset", Proc.getRelocationOffset(),
                                     Proc.CodeOffset, &LinkageName);
  }
  W.printHex("Segment", Proc.Segment);
  W.printFlags("Flags", static_cast<uint8_t>(Proc.Flags),
               getProcSymFlagNames());
  W.printString("DisplayName", Proc.Name);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           ScopeEndSym &ScopeEnd) {
  if (CVR.kind() == SymbolKind::S_END)
    DictScope S(W, "BlockEnd");
  else if (CVR.kind() == SymbolKind::S_PROC_ID_END)
    DictScope S(W, "ProcEnd");
  else if (CVR.kind() == SymbolKind::S_INLINESITE_END)
    DictScope S(W, "InlineSiteEnd");

  InFunctionScope = false;
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, CallerSym &Caller) {
  ListScope S(W, CVR.kind() == S_CALLEES ? "Callees" : "Callers");
  for (auto FuncID : Caller.Indices)
    CVTD.printTypeIndex("FuncID", FuncID);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           RegRelativeSym &RegRel) {
  DictScope S(W, "RegRelativeSym");

  W.printHex("Offset", RegRel.Offset);
  CVTD.printTypeIndex("Type", RegRel.Type);
  W.printHex("Register", RegRel.Register);
  W.printString("VarName", RegRel.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR,
                                           ThreadLocalDataSym &Data) {
  DictScope S(W, "ThreadLocalDataSym");

  StringRef LinkageName;
  if (ObjDelegate) {
    ObjDelegate->printRelocatedField("DataOffset", Data.getRelocationOffset(),
                                     Data.DataOffset, &LinkageName);
  }
  CVTD.printTypeIndex("Type", Data.Type);
  W.printString("DisplayName", Data.Name);
  if (!LinkageName.empty())
    W.printString("LinkageName", LinkageName);
  return Error::success();
}

Error CVSymbolDumperImpl::visitKnownRecord(CVSymbol &CVR, UDTSym &UDT) {
  DictScope S(W, "UDT");
  CVTD.printTypeIndex("Type", UDT.Type);
  W.printString("UDTName", UDT.Name);
  return Error::success();
}

Error CVSymbolDumperImpl::visitUnknownSymbol(CVSymbol &CVR) {
  DictScope S(W, "UnknownSym");
  W.printEnum("Kind", uint16_t(CVR.kind()), getSymbolTypeNames());
  W.printNumber("Length", CVR.length());
  return Error::success();
}

Error CVSymbolDumper::dump(CVRecord<SymbolKind> &Record) {
  SymbolVisitorCallbackPipeline Pipeline;
  SymbolDeserializer Deserializer(ObjDelegate.get());
  CVSymbolDumperImpl Dumper(CVTD, ObjDelegate.get(), W, PrintRecordBytes);

  Pipeline.addCallbackToPipeline(Deserializer);
  Pipeline.addCallbackToPipeline(Dumper);
  CVSymbolVisitor Visitor(Pipeline);
  return Visitor.visitSymbolRecord(Record);
}

Error CVSymbolDumper::dump(const CVSymbolArray &Symbols) {
  SymbolVisitorCallbackPipeline Pipeline;
  SymbolDeserializer Deserializer(ObjDelegate.get());
  CVSymbolDumperImpl Dumper(CVTD, ObjDelegate.get(), W, PrintRecordBytes);

  Pipeline.addCallbackToPipeline(Deserializer);
  Pipeline.addCallbackToPipeline(Dumper);
  CVSymbolVisitor Visitor(Pipeline);
  return Visitor.visitSymbolStream(Symbols);
}
