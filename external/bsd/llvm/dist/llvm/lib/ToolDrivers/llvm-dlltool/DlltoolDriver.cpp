//===- DlltoolDriver.cpp - dlltool.exe-compatible driver ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Defines an interface to a dlltool.exe-compatible driver.
//
//===----------------------------------------------------------------------===//

#include "llvm/ToolDrivers/llvm-dlltool/DlltoolDriver.h"
#include "llvm/Object/ArchiveWriter.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/COFFImportFile.h"
#include "llvm/Object/COFFModuleDefinition.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/Option.h"
#include "llvm/Support/Path.h"

#include <string>
#include <vector>

using namespace llvm;
using namespace llvm::object;
using namespace llvm::COFF;

namespace {

enum {
  OPT_INVALID = 0,
#define OPTION(_1, _2, ID, _4, _5, _6, _7, _8, _9, _10, _11, _12) OPT_##ID,
#include "Options.inc"
#undef OPTION
};

#define PREFIX(NAME, VALUE) const char *const NAME[] = VALUE;
#include "Options.inc"
#undef PREFIX

static const llvm::opt::OptTable::Info infoTable[] = {
#define OPTION(X1, X2, ID, KIND, GROUP, ALIAS, X7, X8, X9, X10, X11, X12)      \
  {X1, X2, X10,         X11,         OPT_##ID, llvm::opt::Option::KIND##Class, \
   X9, X8, OPT_##GROUP, OPT_##ALIAS, X7,       X12},
#include "Options.inc"
#undef OPTION
};

class DllOptTable : public llvm::opt::OptTable {
public:
  DllOptTable() : OptTable(infoTable, false) {}
};

} // namespace

std::vector<std::unique_ptr<MemoryBuffer>> OwningMBs;

// Opens a file. Path has to be resolved already.
// Newly created memory buffers are owned by this driver.
MemoryBufferRef openFile(StringRef Path) {
  ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> MB = MemoryBuffer::getFile(Path);

  if (std::error_code EC = MB.getError())
    llvm::errs() << "fail openFile: " << EC.message() << "\n";

  MemoryBufferRef MBRef = MB.get()->getMemBufferRef();
  OwningMBs.push_back(std::move(MB.get())); // take ownership
  return MBRef;
}

static MachineTypes getEmulation(StringRef S) {
  return StringSwitch<MachineTypes>(S)
      .Case("i386", IMAGE_FILE_MACHINE_I386)
      .Case("i386:x86-64", IMAGE_FILE_MACHINE_AMD64)
      .Case("arm", IMAGE_FILE_MACHINE_ARMNT)
      .Default(IMAGE_FILE_MACHINE_UNKNOWN);
}

static std::string getImplibPath(std::string Path) {
  SmallString<128> Out = StringRef("lib");
  Out.append(Path);
  sys::path::replace_extension(Out, ".a");
  return Out.str();
}

int llvm::dlltoolDriverMain(llvm::ArrayRef<const char *> ArgsArr) {
  DllOptTable Table;
  unsigned MissingIndex;
  unsigned MissingCount;
  llvm::opt::InputArgList Args =
      Table.ParseArgs(ArgsArr.slice(1), MissingIndex, MissingCount);
  if (MissingCount) {
    llvm::errs() << Args.getArgString(MissingIndex) << ": missing argument\n";
    return 1;
  }

  // Handle when no input or output is specified
  if (Args.hasArgNoClaim(OPT_INPUT) ||
      (!Args.hasArgNoClaim(OPT_d) && !Args.hasArgNoClaim(OPT_l))) {
    Table.PrintHelp(outs(), ArgsArr[0], "dlltool", false);
    llvm::outs() << "\nTARGETS: i386, i386:x86-64, arm\n";
    return 1;
  }

  if (!Args.hasArgNoClaim(OPT_m) && Args.hasArgNoClaim(OPT_d)) {
    llvm::errs() << "error: no target machine specified\n"
                 << "supported targets: i386, i386:x86-64, arm\n";
    return 1;
  }

  for (auto *Arg : Args.filtered(OPT_UNKNOWN))
    llvm::errs() << "ignoring unknown argument: " << Arg->getSpelling() << "\n";

  MemoryBufferRef MB;
  if (auto *Arg = Args.getLastArg(OPT_d))
    MB = openFile(Arg->getValue());

  if (!MB.getBufferSize()) {
    llvm::errs() << "definition file empty\n";
    return 1;
  }

  COFF::MachineTypes Machine = IMAGE_FILE_MACHINE_UNKNOWN;
  if (auto *Arg = Args.getLastArg(OPT_m))
    Machine = getEmulation(Arg->getValue());

  if (Machine == IMAGE_FILE_MACHINE_UNKNOWN) {
    llvm::errs() << "unknown target\n";
    return 1;
  }

  Expected<COFFModuleDefinition> Def =
      parseCOFFModuleDefinition(MB, Machine, true);

  if (!Def) {
    llvm::errs() << "error parsing definition\n"
                 << errorToErrorCode(Def.takeError()).message();
    return 1;
  }

  // Do this after the parser because parseCOFFModuleDefinition sets OutputFile.
  if (auto *Arg = Args.getLastArg(OPT_D))
    Def->OutputFile = Arg->getValue();

  if (Def->OutputFile.empty()) {
    llvm::errs() << "no output file specified\n";
    return 1;
  }

  std::string Path = Args.getLastArgValue(OPT_l);
  if (Path.empty())
    Path = getImplibPath(Def->OutputFile);

  if (writeImportLibrary(Def->OutputFile, Path, Def->Exports, Machine))
    return 1;
  return 0;
}
