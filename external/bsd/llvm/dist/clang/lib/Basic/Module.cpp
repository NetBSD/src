//===--- Module.cpp - Describe a module -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Module class, which describes a module in the source
// code.
//
//===----------------------------------------------------------------------===//

#include "clang/Basic/Module.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

Module::Module(StringRef Name, SourceLocation DefinitionLoc, Module *Parent, 
               bool IsFramework, bool IsExplicit)
  : Name(Name), DefinitionLoc(DefinitionLoc), Parent(Parent),
    Umbrella(), ASTFile(0), IsAvailable(true), IsFromModuleFile(false),
    IsFramework(IsFramework), IsExplicit(IsExplicit), IsSystem(false),
    InferSubmodules(false), InferExplicitSubmodules(false), 
    InferExportWildcard(false), ConfigMacrosExhaustive(false),
    NameVisibility(Hidden)
{ 
  if (Parent) {
    if (!Parent->isAvailable())
      IsAvailable = false;
    if (Parent->IsSystem)
      IsSystem = true;
    
    Parent->SubModuleIndex[Name] = Parent->SubModules.size();
    Parent->SubModules.push_back(this);
  }
}

Module::~Module() {
  for (submodule_iterator I = submodule_begin(), IEnd = submodule_end();
       I != IEnd; ++I) {
    delete *I;
  }
}

/// \brief Determine whether a translation unit built using the current
/// language options has the given feature.
static bool hasFeature(StringRef Feature, const LangOptions &LangOpts,
                       const TargetInfo &Target) {
  return llvm::StringSwitch<bool>(Feature)
           .Case("altivec", LangOpts.AltiVec)
           .Case("blocks", LangOpts.Blocks)
           .Case("cplusplus", LangOpts.CPlusPlus)
           .Case("cplusplus11", LangOpts.CPlusPlus11)
           .Case("objc", LangOpts.ObjC1)
           .Case("objc_arc", LangOpts.ObjCAutoRefCount)
           .Case("opencl", LangOpts.OpenCL)
           .Case("tls", Target.isTLSSupported())
           .Default(Target.hasFeature(Feature));
}

bool
Module::isAvailable(const LangOptions &LangOpts, const TargetInfo &Target,
                    Requirement &Req, HeaderDirective &MissingHeader) const {
  if (IsAvailable)
    return true;

  for (const Module *Current = this; Current; Current = Current->Parent) {
    if (!Current->MissingHeaders.empty()) {
      MissingHeader = Current->MissingHeaders.front();
      return false;
    }
    for (unsigned I = 0, N = Current->Requirements.size(); I != N; ++I) {
      if (hasFeature(Current->Requirements[I].first, LangOpts, Target) !=
              Current->Requirements[I].second) {
        Req = Current->Requirements[I];
        return false;
      }
    }
  }

  llvm_unreachable("could not find a reason why module is unavailable");
}

bool Module::isSubModuleOf(Module *Other) const {
  const Module *This = this;
  do {
    if (This == Other)
      return true;
    
    This = This->Parent;
  } while (This);
  
  return false;
}

const Module *Module::getTopLevelModule() const {
  const Module *Result = this;
  while (Result->Parent)
    Result = Result->Parent;
  
  return Result;
}

std::string Module::getFullModuleName() const {
  SmallVector<StringRef, 2> Names;
  
  // Build up the set of module names (from innermost to outermost).
  for (const Module *M = this; M; M = M->Parent)
    Names.push_back(M->Name);
  
  std::string Result;
  for (SmallVectorImpl<StringRef>::reverse_iterator I = Names.rbegin(),
                                                 IEnd = Names.rend();
       I != IEnd; ++I) {
    if (!Result.empty())
      Result += '.';
    
    Result += *I;
  }
  
  return Result;
}

const DirectoryEntry *Module::getUmbrellaDir() const {
  if (const FileEntry *Header = getUmbrellaHeader())
    return Header->getDir();
  
  return Umbrella.dyn_cast<const DirectoryEntry *>();
}

ArrayRef<const FileEntry *> Module::getTopHeaders(FileManager &FileMgr) {
  if (!TopHeaderNames.empty()) {
    for (std::vector<std::string>::iterator
           I = TopHeaderNames.begin(), E = TopHeaderNames.end(); I != E; ++I) {
      if (const FileEntry *FE = FileMgr.getFile(*I))
        TopHeaders.insert(FE);
    }
    TopHeaderNames.clear();
  }

  return llvm::makeArrayRef(TopHeaders.begin(), TopHeaders.end());
}

void Module::addRequirement(StringRef Feature, bool RequiredState,
                            const LangOptions &LangOpts,
                            const TargetInfo &Target) {
  Requirements.push_back(Requirement(Feature, RequiredState));

  // If this feature is currently available, we're done.
  if (hasFeature(Feature, LangOpts, Target) == RequiredState)
    return;

  if (!IsAvailable)
    return;

  SmallVector<Module *, 2> Stack;
  Stack.push_back(this);
  while (!Stack.empty()) {
    Module *Current = Stack.back();
    Stack.pop_back();

    if (!Current->IsAvailable)
      continue;

    Current->IsAvailable = false;
    for (submodule_iterator Sub = Current->submodule_begin(),
                         SubEnd = Current->submodule_end();
         Sub != SubEnd; ++Sub) {
      if ((*Sub)->IsAvailable)
        Stack.push_back(*Sub);
    }
  }
}

Module *Module::findSubmodule(StringRef Name) const {
  llvm::StringMap<unsigned>::const_iterator Pos = SubModuleIndex.find(Name);
  if (Pos == SubModuleIndex.end())
    return 0;
  
  return SubModules[Pos->getValue()];
}

static void printModuleId(raw_ostream &OS, const ModuleId &Id) {
  for (unsigned I = 0, N = Id.size(); I != N; ++I) {
    if (I)
      OS << ".";
    OS << Id[I].first;
  }
}

void Module::getExportedModules(SmallVectorImpl<Module *> &Exported) const {
  // All non-explicit submodules are exported.
  for (std::vector<Module *>::const_iterator I = SubModules.begin(),
                                             E = SubModules.end();
       I != E; ++I) {
    Module *Mod = *I;
    if (!Mod->IsExplicit)
      Exported.push_back(Mod);
  }

  // Find re-exported modules by filtering the list of imported modules.
  bool AnyWildcard = false;
  bool UnrestrictedWildcard = false;
  SmallVector<Module *, 4> WildcardRestrictions;
  for (unsigned I = 0, N = Exports.size(); I != N; ++I) {
    Module *Mod = Exports[I].getPointer();
    if (!Exports[I].getInt()) {
      // Export a named module directly; no wildcards involved.
      Exported.push_back(Mod);

      continue;
    }

    // Wildcard export: export all of the imported modules that match
    // the given pattern.
    AnyWildcard = true;
    if (UnrestrictedWildcard)
      continue;

    if (Module *Restriction = Exports[I].getPointer())
      WildcardRestrictions.push_back(Restriction);
    else {
      WildcardRestrictions.clear();
      UnrestrictedWildcard = true;
    }
  }

  // If there were any wildcards, push any imported modules that were
  // re-exported by the wildcard restriction.
  if (!AnyWildcard)
    return;

  for (unsigned I = 0, N = Imports.size(); I != N; ++I) {
    Module *Mod = Imports[I];
    bool Acceptable = UnrestrictedWildcard;
    if (!Acceptable) {
      // Check whether this module meets one of the restrictions.
      for (unsigned R = 0, NR = WildcardRestrictions.size(); R != NR; ++R) {
        Module *Restriction = WildcardRestrictions[R];
        if (Mod == Restriction || Mod->isSubModuleOf(Restriction)) {
          Acceptable = true;
          break;
        }
      }
    }

    if (!Acceptable)
      continue;

    Exported.push_back(Mod);
  }
}

void Module::buildVisibleModulesCache() const {
  assert(VisibleModulesCache.empty() && "cache does not need building");

  // This module is visible to itself.
  VisibleModulesCache.insert(this);

  // Every imported module is visible.
  SmallVector<Module *, 16> Stack(Imports.begin(), Imports.end());
  while (!Stack.empty()) {
    Module *CurrModule = Stack.pop_back_val();

    // Every module transitively exported by an imported module is visible.
    if (VisibleModulesCache.insert(CurrModule).second)
      CurrModule->getExportedModules(Stack);
  }
}

void Module::print(raw_ostream &OS, unsigned Indent) const {
  OS.indent(Indent);
  if (IsFramework)
    OS << "framework ";
  if (IsExplicit)
    OS << "explicit ";
  OS << "module " << Name;

  if (IsSystem) {
    OS.indent(Indent + 2);
    OS << " [system]";
  }

  OS << " {\n";
  
  if (!Requirements.empty()) {
    OS.indent(Indent + 2);
    OS << "requires ";
    for (unsigned I = 0, N = Requirements.size(); I != N; ++I) {
      if (I)
        OS << ", ";
      if (!Requirements[I].second)
        OS << "!";
      OS << Requirements[I].first;
    }
    OS << "\n";
  }
  
  if (const FileEntry *UmbrellaHeader = getUmbrellaHeader()) {
    OS.indent(Indent + 2);
    OS << "umbrella header \"";
    OS.write_escaped(UmbrellaHeader->getName());
    OS << "\"\n";
  } else if (const DirectoryEntry *UmbrellaDir = getUmbrellaDir()) {
    OS.indent(Indent + 2);
    OS << "umbrella \"";
    OS.write_escaped(UmbrellaDir->getName());
    OS << "\"\n";    
  }

  if (!ConfigMacros.empty() || ConfigMacrosExhaustive) {
    OS.indent(Indent + 2);
    OS << "config_macros ";
    if (ConfigMacrosExhaustive)
      OS << "[exhaustive]";
    for (unsigned I = 0, N = ConfigMacros.size(); I != N; ++I) {
      if (I)
        OS << ", ";
      OS << ConfigMacros[I];
    }
    OS << "\n";
  }

  for (unsigned I = 0, N = NormalHeaders.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "header \"";
    OS.write_escaped(NormalHeaders[I]->getName());
    OS << "\"\n";
  }

  for (unsigned I = 0, N = ExcludedHeaders.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "exclude header \"";
    OS.write_escaped(ExcludedHeaders[I]->getName());
    OS << "\"\n";
  }

  for (unsigned I = 0, N = PrivateHeaders.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "private header \"";
    OS.write_escaped(PrivateHeaders[I]->getName());
    OS << "\"\n";
  }
  
  for (submodule_const_iterator MI = submodule_begin(), MIEnd = submodule_end();
       MI != MIEnd; ++MI)
    (*MI)->print(OS, Indent + 2);
  
  for (unsigned I = 0, N = Exports.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "export ";
    if (Module *Restriction = Exports[I].getPointer()) {
      OS << Restriction->getFullModuleName();
      if (Exports[I].getInt())
        OS << ".*";
    } else {
      OS << "*";
    }
    OS << "\n";
  }

  for (unsigned I = 0, N = UnresolvedExports.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "export ";
    printModuleId(OS, UnresolvedExports[I].Id);
    if (UnresolvedExports[I].Wildcard) {
      if (UnresolvedExports[I].Id.empty())
        OS << "*";
      else
        OS << ".*";
    }
    OS << "\n";
  }

  for (unsigned I = 0, N = DirectUses.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "use ";
    OS << DirectUses[I]->getFullModuleName();
    OS << "\n";
  }

  for (unsigned I = 0, N = UnresolvedDirectUses.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "use ";
    printModuleId(OS, UnresolvedDirectUses[I]);
    OS << "\n";
  }

  for (unsigned I = 0, N = LinkLibraries.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "link ";
    if (LinkLibraries[I].IsFramework)
      OS << "framework ";
    OS << "\"";
    OS.write_escaped(LinkLibraries[I].Library);
    OS << "\"";
  }

  for (unsigned I = 0, N = UnresolvedConflicts.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "conflict ";
    printModuleId(OS, UnresolvedConflicts[I].Id);
    OS << ", \"";
    OS.write_escaped(UnresolvedConflicts[I].Message);
    OS << "\"\n";
  }

  for (unsigned I = 0, N = Conflicts.size(); I != N; ++I) {
    OS.indent(Indent + 2);
    OS << "conflict ";
    OS << Conflicts[I].Other->getFullModuleName();
    OS << ", \"";
    OS.write_escaped(Conflicts[I].Message);
    OS << "\"\n";
  }

  if (InferSubmodules) {
    OS.indent(Indent + 2);
    if (InferExplicitSubmodules)
      OS << "explicit ";
    OS << "module * {\n";
    if (InferExportWildcard) {
      OS.indent(Indent + 4);
      OS << "export *\n";
    }
    OS.indent(Indent + 2);
    OS << "}\n";
  }
  
  OS.indent(Indent);
  OS << "}\n";
}

void Module::dump() const {
  print(llvm::errs());
}


