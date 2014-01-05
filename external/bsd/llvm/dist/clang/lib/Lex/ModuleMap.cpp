//===--- ModuleMap.cpp - Describe the layout of modules ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ModuleMap implementation, which describes the layout
// of a module as it relates to headers.
//
//===----------------------------------------------------------------------===//
#include "clang/Lex/ModuleMap.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/LiteralSupport.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <stdlib.h>
#if defined(LLVM_ON_UNIX)
#include <limits.h>
#endif
using namespace clang;

Module::ExportDecl 
ModuleMap::resolveExport(Module *Mod, 
                         const Module::UnresolvedExportDecl &Unresolved,
                         bool Complain) const {
  // We may have just a wildcard.
  if (Unresolved.Id.empty()) {
    assert(Unresolved.Wildcard && "Invalid unresolved export");
    return Module::ExportDecl(0, true);
  }
  
  // Resolve the module-id.
  Module *Context = resolveModuleId(Unresolved.Id, Mod, Complain);
  if (!Context)
    return Module::ExportDecl();

  return Module::ExportDecl(Context, Unresolved.Wildcard);
}

Module *ModuleMap::resolveModuleId(const ModuleId &Id, Module *Mod,
                                   bool Complain) const {
  // Find the starting module.
  Module *Context = lookupModuleUnqualified(Id[0].first, Mod);
  if (!Context) {
    if (Complain)
      Diags.Report(Id[0].second, diag::err_mmap_missing_module_unqualified)
      << Id[0].first << Mod->getFullModuleName();

    return 0;
  }

  // Dig into the module path.
  for (unsigned I = 1, N = Id.size(); I != N; ++I) {
    Module *Sub = lookupModuleQualified(Id[I].first, Context);
    if (!Sub) {
      if (Complain)
        Diags.Report(Id[I].second, diag::err_mmap_missing_module_qualified)
        << Id[I].first << Context->getFullModuleName()
        << SourceRange(Id[0].second, Id[I-1].second);

      return 0;
    }

    Context = Sub;
  }

  return Context;
}

ModuleMap::ModuleMap(SourceManager &SourceMgr, DiagnosticsEngine &Diags,
                     const LangOptions &LangOpts, const TargetInfo *Target,
                     HeaderSearch &HeaderInfo)
    : SourceMgr(SourceMgr), Diags(Diags), LangOpts(LangOpts), Target(Target),
      HeaderInfo(HeaderInfo), BuiltinIncludeDir(0), CompilingModule(0),
      SourceModule(0) {}

ModuleMap::~ModuleMap() {
  for (llvm::StringMap<Module *>::iterator I = Modules.begin(), 
                                        IEnd = Modules.end();
       I != IEnd; ++I) {
    delete I->getValue();
  }
}

void ModuleMap::setTarget(const TargetInfo &Target) {
  assert((!this->Target || this->Target == &Target) && 
         "Improper target override");
  this->Target = &Target;
}

/// \brief "Sanitize" a filename so that it can be used as an identifier.
static StringRef sanitizeFilenameAsIdentifier(StringRef Name,
                                              SmallVectorImpl<char> &Buffer) {
  if (Name.empty())
    return Name;

  if (!isValidIdentifier(Name)) {
    // If we don't already have something with the form of an identifier,
    // create a buffer with the sanitized name.
    Buffer.clear();
    if (isDigit(Name[0]))
      Buffer.push_back('_');
    Buffer.reserve(Buffer.size() + Name.size());
    for (unsigned I = 0, N = Name.size(); I != N; ++I) {
      if (isIdentifierBody(Name[I]))
        Buffer.push_back(Name[I]);
      else
        Buffer.push_back('_');
    }

    Name = StringRef(Buffer.data(), Buffer.size());
  }

  while (llvm::StringSwitch<bool>(Name)
#define KEYWORD(Keyword,Conditions) .Case(#Keyword, true)
#define ALIAS(Keyword, AliasOf, Conditions) .Case(Keyword, true)
#include "clang/Basic/TokenKinds.def"
           .Default(false)) {
    if (Name.data() != Buffer.data())
      Buffer.append(Name.begin(), Name.end());
    Buffer.push_back('_');
    Name = StringRef(Buffer.data(), Buffer.size());
  }

  return Name;
}

/// \brief Determine whether the given file name is the name of a builtin
/// header, supplied by Clang to replace, override, or augment existing system
/// headers.
static bool isBuiltinHeader(StringRef FileName) {
  return llvm::StringSwitch<bool>(FileName)
           .Case("float.h", true)
           .Case("iso646.h", true)
           .Case("limits.h", true)
           .Case("stdalign.h", true)
           .Case("stdarg.h", true)
           .Case("stdbool.h", true)
           .Case("stddef.h", true)
           .Case("stdint.h", true)
           .Case("tgmath.h", true)
           .Case("unwind.h", true)
           .Default(false);
}

ModuleMap::HeadersMap::iterator
ModuleMap::findKnownHeader(const FileEntry *File) {
  HeadersMap::iterator Known = Headers.find(File);
  if (Known == Headers.end() && File->getDir() == BuiltinIncludeDir &&
      isBuiltinHeader(llvm::sys::path::filename(File->getName()))) {
    HeaderInfo.loadTopLevelSystemModules();
    return Headers.find(File);
  }
  return Known;
}

// Returns 'true' if 'RequestingModule directly uses 'RequestedModule'.
static bool directlyUses(const Module *RequestingModule,
                         const Module *RequestedModule) {
  return std::find(RequestingModule->DirectUses.begin(),
                   RequestingModule->DirectUses.end(),
                   RequestedModule) != RequestingModule->DirectUses.end();
}

static bool violatesPrivateInclude(Module *RequestingModule,
                                   const FileEntry *IncFileEnt,
                                   ModuleMap::ModuleHeaderRole Role,
                                   Module *RequestedModule) {
  #ifndef NDEBUG
  // Check for consistency between the module header role
  // as obtained from the lookup and as obtained from the module.
  // This check is not cheap, so enable it only for debugging.
  SmallVectorImpl<const FileEntry *> &PvtHdrs
      = RequestedModule->PrivateHeaders;
  SmallVectorImpl<const FileEntry *>::iterator Look
      = std::find(PvtHdrs.begin(), PvtHdrs.end(), IncFileEnt);
  bool IsPrivate = Look != PvtHdrs.end();
  assert((IsPrivate && Role == ModuleMap::PrivateHeader)
               || (!IsPrivate && Role != ModuleMap::PrivateHeader));
  #endif
  return Role == ModuleMap::PrivateHeader &&
         RequestedModule->getTopLevelModule() != RequestingModule;
}

void ModuleMap::diagnoseHeaderInclusion(Module *RequestingModule,
                                        SourceLocation FilenameLoc,
                                        StringRef Filename,
                                        const FileEntry *File) {
  // No errors for indirect modules. This may be a bit of a problem for modules
  // with no source files.
  if (RequestingModule != SourceModule)
    return;

  if (RequestingModule)
    resolveUses(RequestingModule, /*Complain=*/false);

  HeadersMap::iterator Known = findKnownHeader(File);
  if (Known == Headers.end())
    return;

  Module *Private = NULL;
  Module *NotUsed = NULL;
  for (SmallVectorImpl<KnownHeader>::iterator I = Known->second.begin(),
                                              E = Known->second.end();
       I != E; ++I) {
    // Excluded headers don't really belong to a module.
    if (I->getRole() == ModuleMap::ExcludedHeader)
      continue;

    // If 'File' is part of 'RequestingModule' we can definitely include it.
    if (I->getModule() == RequestingModule)
      return;

    // Remember private headers for later printing of a diagnostic.
    if (violatesPrivateInclude(RequestingModule, File, I->getRole(),
                               I->getModule())) {
      Private = I->getModule();
      continue;
    }

    // If uses need to be specified explicitly, we are only allowed to return
    // modules that are explicitly used by the requesting module.
    if (RequestingModule && LangOpts.ModulesDeclUse &&
        !directlyUses(RequestingModule, I->getModule())) {
      NotUsed = I->getModule();
      continue;
    }

    // We have found a module that we can happily use.
    return;
  }

  // We have found a header, but it is private.
  if (Private != NULL) {
    Diags.Report(FilenameLoc, diag::error_use_of_private_header_outside_module)
        << Filename;
    return;
  }

  // We have found a module, but we don't use it.
  if (NotUsed != NULL) {
    Diags.Report(FilenameLoc, diag::error_undeclared_use_of_module)
        << RequestingModule->getFullModuleName() << Filename;
    return;
  }

  // Headers for which we have not found a module are fine to include.
}

ModuleMap::KnownHeader
ModuleMap::findModuleForHeader(const FileEntry *File,
                               Module *RequestingModule) {
  HeadersMap::iterator Known = findKnownHeader(File);

  if (Known != Headers.end()) {
    ModuleMap::KnownHeader Result = KnownHeader();

    // Iterate over all modules that 'File' is part of to find the best fit.
    for (SmallVectorImpl<KnownHeader>::iterator I = Known->second.begin(),
                                                E = Known->second.end();
         I != E; ++I) {
      // Cannot use a module if the header is excluded in it.
      if (I->getRole() == ModuleMap::ExcludedHeader)
        continue;

      // Cannot use a module if it is unavailable.
      if (!I->getModule()->isAvailable())
        continue;

      // If 'File' is part of 'RequestingModule', 'RequestingModule' is the
      // module we are looking for.
      if (I->getModule() == RequestingModule)
        return *I;

      // If uses need to be specified explicitly, we are only allowed to return
      // modules that are explicitly used by the requesting module.
      if (RequestingModule && LangOpts.ModulesDeclUse &&
          !directlyUses(RequestingModule, I->getModule()))
        continue;

      Result = *I;
      // If 'File' is a public header of this module, this is as good as we
      // are going to get.
      if (I->getRole() == ModuleMap::NormalHeader)
        break;
    }
    return Result;
  }

  const DirectoryEntry *Dir = File->getDir();
  SmallVector<const DirectoryEntry *, 2> SkippedDirs;

  // Note: as an egregious but useful hack we use the real path here, because
  // frameworks moving from top-level frameworks to embedded frameworks tend
  // to be symlinked from the top-level location to the embedded location,
  // and we need to resolve lookups as if we had found the embedded location.
  StringRef DirName = SourceMgr.getFileManager().getCanonicalName(Dir);

  // Keep walking up the directory hierarchy, looking for a directory with
  // an umbrella header.
  do {    
    llvm::DenseMap<const DirectoryEntry *, Module *>::iterator KnownDir
      = UmbrellaDirs.find(Dir);
    if (KnownDir != UmbrellaDirs.end()) {
      Module *Result = KnownDir->second;
      
      // Search up the module stack until we find a module with an umbrella
      // directory.
      Module *UmbrellaModule = Result;
      while (!UmbrellaModule->getUmbrellaDir() && UmbrellaModule->Parent)
        UmbrellaModule = UmbrellaModule->Parent;

      if (UmbrellaModule->InferSubmodules) {
        // Infer submodules for each of the directories we found between
        // the directory of the umbrella header and the directory where 
        // the actual header is located.
        bool Explicit = UmbrellaModule->InferExplicitSubmodules;
        
        for (unsigned I = SkippedDirs.size(); I != 0; --I) {
          // Find or create the module that corresponds to this directory name.
          SmallString<32> NameBuf;
          StringRef Name = sanitizeFilenameAsIdentifier(
                             llvm::sys::path::stem(SkippedDirs[I-1]->getName()),
                             NameBuf);
          Result = findOrCreateModule(Name, Result, /*IsFramework=*/false,
                                      Explicit).first;
          
          // Associate the module and the directory.
          UmbrellaDirs[SkippedDirs[I-1]] = Result;

          // If inferred submodules export everything they import, add a 
          // wildcard to the set of exports.
          if (UmbrellaModule->InferExportWildcard && Result->Exports.empty())
            Result->Exports.push_back(Module::ExportDecl(0, true));
        }
        
        // Infer a submodule with the same name as this header file.
        SmallString<32> NameBuf;
        StringRef Name = sanitizeFilenameAsIdentifier(
                           llvm::sys::path::stem(File->getName()), NameBuf);
        Result = findOrCreateModule(Name, Result, /*IsFramework=*/false,
                                    Explicit).first;
        Result->addTopHeader(File);
        
        // If inferred submodules export everything they import, add a 
        // wildcard to the set of exports.
        if (UmbrellaModule->InferExportWildcard && Result->Exports.empty())
          Result->Exports.push_back(Module::ExportDecl(0, true));
      } else {
        // Record each of the directories we stepped through as being part of
        // the module we found, since the umbrella header covers them all.
        for (unsigned I = 0, N = SkippedDirs.size(); I != N; ++I)
          UmbrellaDirs[SkippedDirs[I]] = Result;
      }
      
      Headers[File].push_back(KnownHeader(Result, NormalHeader));

      // If a header corresponds to an unavailable module, don't report
      // that it maps to anything.
      if (!Result->isAvailable())
        return KnownHeader();

      return Headers[File].back();
    }
    
    SkippedDirs.push_back(Dir);
    
    // Retrieve our parent path.
    DirName = llvm::sys::path::parent_path(DirName);
    if (DirName.empty())
      break;
    
    // Resolve the parent path to a directory entry.
    Dir = SourceMgr.getFileManager().getDirectory(DirName);
  } while (Dir);
  
  return KnownHeader();
}

bool ModuleMap::isHeaderInUnavailableModule(const FileEntry *Header) const {
  HeadersMap::const_iterator Known = Headers.find(Header);
  if (Known != Headers.end()) {
    for (SmallVectorImpl<KnownHeader>::const_iterator
             I = Known->second.begin(),
             E = Known->second.end();
         I != E; ++I) {
      if (I->isAvailable())
        return false;
    }
    return true;
  }
  
  const DirectoryEntry *Dir = Header->getDir();
  SmallVector<const DirectoryEntry *, 2> SkippedDirs;
  StringRef DirName = Dir->getName();

  // Keep walking up the directory hierarchy, looking for a directory with
  // an umbrella header.
  do {    
    llvm::DenseMap<const DirectoryEntry *, Module *>::const_iterator KnownDir
      = UmbrellaDirs.find(Dir);
    if (KnownDir != UmbrellaDirs.end()) {
      Module *Found = KnownDir->second;
      if (!Found->isAvailable())
        return true;

      // Search up the module stack until we find a module with an umbrella
      // directory.
      Module *UmbrellaModule = Found;
      while (!UmbrellaModule->getUmbrellaDir() && UmbrellaModule->Parent)
        UmbrellaModule = UmbrellaModule->Parent;

      if (UmbrellaModule->InferSubmodules) {
        for (unsigned I = SkippedDirs.size(); I != 0; --I) {
          // Find or create the module that corresponds to this directory name.
          SmallString<32> NameBuf;
          StringRef Name = sanitizeFilenameAsIdentifier(
                             llvm::sys::path::stem(SkippedDirs[I-1]->getName()),
                             NameBuf);
          Found = lookupModuleQualified(Name, Found);
          if (!Found)
            return false;
          if (!Found->isAvailable())
            return true;
        }
        
        // Infer a submodule with the same name as this header file.
        SmallString<32> NameBuf;
        StringRef Name = sanitizeFilenameAsIdentifier(
                           llvm::sys::path::stem(Header->getName()),
                           NameBuf);
        Found = lookupModuleQualified(Name, Found);
        if (!Found)
          return false;
      }

      return !Found->isAvailable();
    }
    
    SkippedDirs.push_back(Dir);
    
    // Retrieve our parent path.
    DirName = llvm::sys::path::parent_path(DirName);
    if (DirName.empty())
      break;
    
    // Resolve the parent path to a directory entry.
    Dir = SourceMgr.getFileManager().getDirectory(DirName);
  } while (Dir);
  
  return false;
}

Module *ModuleMap::findModule(StringRef Name) const {
  llvm::StringMap<Module *>::const_iterator Known = Modules.find(Name);
  if (Known != Modules.end())
    return Known->getValue();
  
  return 0;
}

Module *ModuleMap::lookupModuleUnqualified(StringRef Name,
                                           Module *Context) const {
  for(; Context; Context = Context->Parent) {
    if (Module *Sub = lookupModuleQualified(Name, Context))
      return Sub;
  }
  
  return findModule(Name);
}

Module *ModuleMap::lookupModuleQualified(StringRef Name, Module *Context) const{
  if (!Context)
    return findModule(Name);
  
  return Context->findSubmodule(Name);
}

std::pair<Module *, bool> 
ModuleMap::findOrCreateModule(StringRef Name, Module *Parent, bool IsFramework,
                              bool IsExplicit) {
  // Try to find an existing module with this name.
  if (Module *Sub = lookupModuleQualified(Name, Parent))
    return std::make_pair(Sub, false);
  
  // Create a new module with this name.
  Module *Result = new Module(Name, SourceLocation(), Parent, IsFramework, 
                              IsExplicit);
  if (LangOpts.CurrentModule == Name) {
    SourceModule = Result;
    SourceModuleName = Name;
  }
  if (!Parent) {
    Modules[Name] = Result;
    if (!LangOpts.CurrentModule.empty() && !CompilingModule &&
        Name == LangOpts.CurrentModule) {
      CompilingModule = Result;
    }
  }
  return std::make_pair(Result, true);
}

bool ModuleMap::canInferFrameworkModule(const DirectoryEntry *ParentDir,
                                        StringRef Name, bool &IsSystem) const {
  // Check whether we have already looked into the parent directory
  // for a module map.
  llvm::DenseMap<const DirectoryEntry *, InferredDirectory>::const_iterator
    inferred = InferredDirectories.find(ParentDir);
  if (inferred == InferredDirectories.end())
    return false;

  if (!inferred->second.InferModules)
    return false;

  // We're allowed to infer for this directory, but make sure it's okay
  // to infer this particular module.
  bool canInfer = std::find(inferred->second.ExcludedModules.begin(),
                            inferred->second.ExcludedModules.end(),
                            Name) == inferred->second.ExcludedModules.end();

  if (canInfer && inferred->second.InferSystemModules)
    IsSystem = true;

  return canInfer;
}

/// \brief For a framework module, infer the framework against which we
/// should link.
static void inferFrameworkLink(Module *Mod, const DirectoryEntry *FrameworkDir,
                               FileManager &FileMgr) {
  assert(Mod->IsFramework && "Can only infer linking for framework modules");
  assert(!Mod->isSubFramework() &&
         "Can only infer linking for top-level frameworks");

  SmallString<128> LibName;
  LibName += FrameworkDir->getName();
  llvm::sys::path::append(LibName, Mod->Name);
  if (FileMgr.getFile(LibName)) {
    Mod->LinkLibraries.push_back(Module::LinkLibrary(Mod->Name,
                                                     /*IsFramework=*/true));
  }
}

Module *
ModuleMap::inferFrameworkModule(StringRef ModuleName,
                                const DirectoryEntry *FrameworkDir,
                                bool IsSystem,
                                Module *Parent) {
  // Check whether we've already found this module.
  if (Module *Mod = lookupModuleQualified(ModuleName, Parent))
    return Mod;
  
  FileManager &FileMgr = SourceMgr.getFileManager();

  // If the framework has a parent path from which we're allowed to infer
  // a framework module, do so.
  if (!Parent) {
    // Determine whether we're allowed to infer a module map.

    // Note: as an egregious but useful hack we use the real path here, because
    // we might be looking at an embedded framework that symlinks out to a
    // top-level framework, and we need to infer as if we were naming the
    // top-level framework.
    StringRef FrameworkDirName
      = SourceMgr.getFileManager().getCanonicalName(FrameworkDir);

    bool canInfer = false;
    if (llvm::sys::path::has_parent_path(FrameworkDirName)) {
      // Figure out the parent path.
      StringRef Parent = llvm::sys::path::parent_path(FrameworkDirName);
      if (const DirectoryEntry *ParentDir = FileMgr.getDirectory(Parent)) {
        // Check whether we have already looked into the parent directory
        // for a module map.
        llvm::DenseMap<const DirectoryEntry *, InferredDirectory>::const_iterator
          inferred = InferredDirectories.find(ParentDir);
        if (inferred == InferredDirectories.end()) {
          // We haven't looked here before. Load a module map, if there is
          // one.
          SmallString<128> ModMapPath = Parent;
          llvm::sys::path::append(ModMapPath, "module.map");
          if (const FileEntry *ModMapFile = FileMgr.getFile(ModMapPath)) {
            parseModuleMapFile(ModMapFile, IsSystem);
            inferred = InferredDirectories.find(ParentDir);
          }

          if (inferred == InferredDirectories.end())
            inferred = InferredDirectories.insert(
                         std::make_pair(ParentDir, InferredDirectory())).first;
        }

        if (inferred->second.InferModules) {
          // We're allowed to infer for this directory, but make sure it's okay
          // to infer this particular module.
          StringRef Name = llvm::sys::path::stem(FrameworkDirName);
          canInfer = std::find(inferred->second.ExcludedModules.begin(),
                               inferred->second.ExcludedModules.end(),
                               Name) == inferred->second.ExcludedModules.end();

          if (inferred->second.InferSystemModules)
            IsSystem = true;
        }
      }
    }

    // If we're not allowed to infer a framework module, don't.
    if (!canInfer)
      return 0;
  }


  // Look for an umbrella header.
  SmallString<128> UmbrellaName = StringRef(FrameworkDir->getName());
  llvm::sys::path::append(UmbrellaName, "Headers", ModuleName + ".h");
  const FileEntry *UmbrellaHeader = FileMgr.getFile(UmbrellaName);
  
  // FIXME: If there's no umbrella header, we could probably scan the
  // framework to load *everything*. But, it's not clear that this is a good
  // idea.
  if (!UmbrellaHeader)
    return 0;
  
  Module *Result = new Module(ModuleName, SourceLocation(), Parent,
                              /*IsFramework=*/true, /*IsExplicit=*/false);
  if (LangOpts.CurrentModule == ModuleName) {
    SourceModule = Result;
    SourceModuleName = ModuleName;
  }
  if (IsSystem)
    Result->IsSystem = IsSystem;
  
  if (!Parent)
    Modules[ModuleName] = Result;
  
  // umbrella header "umbrella-header-name"
  Result->Umbrella = UmbrellaHeader;
  Headers[UmbrellaHeader].push_back(KnownHeader(Result, NormalHeader));
  UmbrellaDirs[UmbrellaHeader->getDir()] = Result;
  
  // export *
  Result->Exports.push_back(Module::ExportDecl(0, true));
  
  // module * { export * }
  Result->InferSubmodules = true;
  Result->InferExportWildcard = true;
  
  // Look for subframeworks.
  llvm::error_code EC;
  SmallString<128> SubframeworksDirName
    = StringRef(FrameworkDir->getName());
  llvm::sys::path::append(SubframeworksDirName, "Frameworks");
  llvm::sys::path::native(SubframeworksDirName);
  for (llvm::sys::fs::directory_iterator 
         Dir(SubframeworksDirName.str(), EC), DirEnd;
       Dir != DirEnd && !EC; Dir.increment(EC)) {
    if (!StringRef(Dir->path()).endswith(".framework"))
      continue;

    if (const DirectoryEntry *SubframeworkDir
          = FileMgr.getDirectory(Dir->path())) {
      // Note: as an egregious but useful hack, we use the real path here and
      // check whether it is actually a subdirectory of the parent directory.
      // This will not be the case if the 'subframework' is actually a symlink
      // out to a top-level framework.
      StringRef SubframeworkDirName = FileMgr.getCanonicalName(SubframeworkDir);
      bool FoundParent = false;
      do {
        // Get the parent directory name.
        SubframeworkDirName
          = llvm::sys::path::parent_path(SubframeworkDirName);
        if (SubframeworkDirName.empty())
          break;

        if (FileMgr.getDirectory(SubframeworkDirName) == FrameworkDir) {
          FoundParent = true;
          break;
        }
      } while (true);

      if (!FoundParent)
        continue;

      // FIXME: Do we want to warn about subframeworks without umbrella headers?
      SmallString<32> NameBuf;
      inferFrameworkModule(sanitizeFilenameAsIdentifier(
                             llvm::sys::path::stem(Dir->path()), NameBuf),
                           SubframeworkDir, IsSystem, Result);
    }
  }

  // If the module is a top-level framework, automatically link against the
  // framework.
  if (!Result->isSubFramework()) {
    inferFrameworkLink(Result, FrameworkDir, FileMgr);
  }

  return Result;
}

void ModuleMap::setUmbrellaHeader(Module *Mod, const FileEntry *UmbrellaHeader){
  Headers[UmbrellaHeader].push_back(KnownHeader(Mod, NormalHeader));
  Mod->Umbrella = UmbrellaHeader;
  UmbrellaDirs[UmbrellaHeader->getDir()] = Mod;
}

void ModuleMap::setUmbrellaDir(Module *Mod, const DirectoryEntry *UmbrellaDir) {
  Mod->Umbrella = UmbrellaDir;
  UmbrellaDirs[UmbrellaDir] = Mod;
}

void ModuleMap::addHeader(Module *Mod, const FileEntry *Header,
                          ModuleHeaderRole Role) {
  if (Role == ExcludedHeader) {
    Mod->ExcludedHeaders.push_back(Header);
  } else {
    if (Role == PrivateHeader)
      Mod->PrivateHeaders.push_back(Header);
    else
      Mod->NormalHeaders.push_back(Header);
    bool isCompilingModuleHeader = Mod->getTopLevelModule() == CompilingModule;
    HeaderInfo.MarkFileModuleHeader(Header, Role, isCompilingModuleHeader);
  }
  Headers[Header].push_back(KnownHeader(Mod, Role));
}

const FileEntry *
ModuleMap::getContainingModuleMapFile(Module *Module) const {
  if (Module->DefinitionLoc.isInvalid())
    return 0;

  return SourceMgr.getFileEntryForID(
           SourceMgr.getFileID(Module->DefinitionLoc));
}

void ModuleMap::dump() {
  llvm::errs() << "Modules:";
  for (llvm::StringMap<Module *>::iterator M = Modules.begin(), 
                                        MEnd = Modules.end(); 
       M != MEnd; ++M)
    M->getValue()->print(llvm::errs(), 2);
  
  llvm::errs() << "Headers:";
  for (HeadersMap::iterator H = Headers.begin(), HEnd = Headers.end();
       H != HEnd; ++H) {
    llvm::errs() << "  \"" << H->first->getName() << "\" -> ";
    for (SmallVectorImpl<KnownHeader>::const_iterator I = H->second.begin(),
                                                      E = H->second.end();
         I != E; ++I) {
      if (I != H->second.begin())
        llvm::errs() << ",";
      llvm::errs() << I->getModule()->getFullModuleName();
    }
    llvm::errs() << "\n";
  }
}

bool ModuleMap::resolveExports(Module *Mod, bool Complain) {
  bool HadError = false;
  for (unsigned I = 0, N = Mod->UnresolvedExports.size(); I != N; ++I) {
    Module::ExportDecl Export = resolveExport(Mod, Mod->UnresolvedExports[I], 
                                              Complain);
    if (Export.getPointer() || Export.getInt())
      Mod->Exports.push_back(Export);
    else
      HadError = true;
  }
  Mod->UnresolvedExports.clear();
  return HadError;
}

bool ModuleMap::resolveUses(Module *Mod, bool Complain) {
  bool HadError = false;
  for (unsigned I = 0, N = Mod->UnresolvedDirectUses.size(); I != N; ++I) {
    Module *DirectUse =
        resolveModuleId(Mod->UnresolvedDirectUses[I], Mod, Complain);
    if (DirectUse)
      Mod->DirectUses.push_back(DirectUse);
    else
      HadError = true;
  }
  Mod->UnresolvedDirectUses.clear();
  return HadError;
}

bool ModuleMap::resolveConflicts(Module *Mod, bool Complain) {
  bool HadError = false;
  for (unsigned I = 0, N = Mod->UnresolvedConflicts.size(); I != N; ++I) {
    Module *OtherMod = resolveModuleId(Mod->UnresolvedConflicts[I].Id,
                                       Mod, Complain);
    if (!OtherMod) {
      HadError = true;
      continue;
    }

    Module::Conflict Conflict;
    Conflict.Other = OtherMod;
    Conflict.Message = Mod->UnresolvedConflicts[I].Message;
    Mod->Conflicts.push_back(Conflict);
  }
  Mod->UnresolvedConflicts.clear();
  return HadError;
}

Module *ModuleMap::inferModuleFromLocation(FullSourceLoc Loc) {
  if (Loc.isInvalid())
    return 0;
  
  // Use the expansion location to determine which module we're in.
  FullSourceLoc ExpansionLoc = Loc.getExpansionLoc();
  if (!ExpansionLoc.isFileID())
    return 0;  
  
  
  const SourceManager &SrcMgr = Loc.getManager();
  FileID ExpansionFileID = ExpansionLoc.getFileID();
  
  while (const FileEntry *ExpansionFile
           = SrcMgr.getFileEntryForID(ExpansionFileID)) {
    // Find the module that owns this header (if any).
    if (Module *Mod = findModuleForHeader(ExpansionFile).getModule())
      return Mod;
    
    // No module owns this header, so look up the inclusion chain to see if
    // any included header has an associated module.
    SourceLocation IncludeLoc = SrcMgr.getIncludeLoc(ExpansionFileID);
    if (IncludeLoc.isInvalid())
      return 0;
    
    ExpansionFileID = SrcMgr.getFileID(IncludeLoc);
  }
  
  return 0;
}

//----------------------------------------------------------------------------//
// Module map file parser
//----------------------------------------------------------------------------//

namespace clang {
  /// \brief A token in a module map file.
  struct MMToken {
    enum TokenKind {
      Comma,
      ConfigMacros,
      Conflict,
      EndOfFile,
      HeaderKeyword,
      Identifier,
      Exclaim,
      ExcludeKeyword,
      ExplicitKeyword,
      ExportKeyword,
      ExternKeyword,
      FrameworkKeyword,
      LinkKeyword,
      ModuleKeyword,
      Period,
      PrivateKeyword,
      UmbrellaKeyword,
      UseKeyword,
      RequiresKeyword,
      Star,
      StringLiteral,
      LBrace,
      RBrace,
      LSquare,
      RSquare
    } Kind;
    
    unsigned Location;
    unsigned StringLength;
    const char *StringData;
    
    void clear() {
      Kind = EndOfFile;
      Location = 0;
      StringLength = 0;
      StringData = 0;
    }
    
    bool is(TokenKind K) const { return Kind == K; }
    
    SourceLocation getLocation() const {
      return SourceLocation::getFromRawEncoding(Location);
    }
    
    StringRef getString() const {
      return StringRef(StringData, StringLength);
    }
  };

  /// \brief The set of attributes that can be attached to a module.
  struct Attributes {
    Attributes() : IsSystem(), IsExhaustive() { }

    /// \brief Whether this is a system module.
    unsigned IsSystem : 1;

    /// \brief Whether this is an exhaustive set of configuration macros.
    unsigned IsExhaustive : 1;
  };
  

  class ModuleMapParser {
    Lexer &L;
    SourceManager &SourceMgr;

    /// \brief Default target information, used only for string literal
    /// parsing.
    const TargetInfo *Target;

    DiagnosticsEngine &Diags;
    ModuleMap &Map;
    
    /// \brief The directory that this module map resides in.
    const DirectoryEntry *Directory;

    /// \brief The directory containing Clang-supplied headers.
    const DirectoryEntry *BuiltinIncludeDir;

    /// \brief Whether this module map is in a system header directory.
    bool IsSystem;
    
    /// \brief Whether an error occurred.
    bool HadError;
        
    /// \brief Stores string data for the various string literals referenced
    /// during parsing.
    llvm::BumpPtrAllocator StringData;
    
    /// \brief The current token.
    MMToken Tok;
    
    /// \brief The active module.
    Module *ActiveModule;
    
    /// \brief Consume the current token and return its location.
    SourceLocation consumeToken();
    
    /// \brief Skip tokens until we reach the a token with the given kind
    /// (or the end of the file).
    void skipUntil(MMToken::TokenKind K);

    typedef SmallVector<std::pair<std::string, SourceLocation>, 2> ModuleId;
    bool parseModuleId(ModuleId &Id);
    void parseModuleDecl();
    void parseExternModuleDecl();
    void parseRequiresDecl();
    void parseHeaderDecl(clang::MMToken::TokenKind,
                         SourceLocation LeadingLoc);
    void parseUmbrellaDirDecl(SourceLocation UmbrellaLoc);
    void parseExportDecl();
    void parseUseDecl();
    void parseLinkDecl();
    void parseConfigMacros();
    void parseConflict();
    void parseInferredModuleDecl(bool Framework, bool Explicit);
    bool parseOptionalAttributes(Attributes &Attrs);

    const DirectoryEntry *getOverriddenHeaderSearchDir();
    
  public:
    explicit ModuleMapParser(Lexer &L, SourceManager &SourceMgr, 
                             const TargetInfo *Target,
                             DiagnosticsEngine &Diags,
                             ModuleMap &Map,
                             const DirectoryEntry *Directory,
                             const DirectoryEntry *BuiltinIncludeDir,
                             bool IsSystem)
      : L(L), SourceMgr(SourceMgr), Target(Target), Diags(Diags), Map(Map), 
        Directory(Directory), BuiltinIncludeDir(BuiltinIncludeDir),
        IsSystem(IsSystem), HadError(false), ActiveModule(0)
    {
      Tok.clear();
      consumeToken();
    }
    
    bool parseModuleMapFile();
  };
}

SourceLocation ModuleMapParser::consumeToken() {
retry:
  SourceLocation Result = Tok.getLocation();
  Tok.clear();
  
  Token LToken;
  L.LexFromRawLexer(LToken);
  Tok.Location = LToken.getLocation().getRawEncoding();
  switch (LToken.getKind()) {
  case tok::raw_identifier:
    Tok.StringData = LToken.getRawIdentifierData();
    Tok.StringLength = LToken.getLength();
    Tok.Kind = llvm::StringSwitch<MMToken::TokenKind>(Tok.getString())
                 .Case("config_macros", MMToken::ConfigMacros)
                 .Case("conflict", MMToken::Conflict)
                 .Case("exclude", MMToken::ExcludeKeyword)
                 .Case("explicit", MMToken::ExplicitKeyword)
                 .Case("export", MMToken::ExportKeyword)
                 .Case("extern", MMToken::ExternKeyword)
                 .Case("framework", MMToken::FrameworkKeyword)
                 .Case("header", MMToken::HeaderKeyword)
                 .Case("link", MMToken::LinkKeyword)
                 .Case("module", MMToken::ModuleKeyword)
                 .Case("private", MMToken::PrivateKeyword)
                 .Case("requires", MMToken::RequiresKeyword)
                 .Case("umbrella", MMToken::UmbrellaKeyword)
                 .Case("use", MMToken::UseKeyword)
                 .Default(MMToken::Identifier);
    break;

  case tok::comma:
    Tok.Kind = MMToken::Comma;
    break;

  case tok::eof:
    Tok.Kind = MMToken::EndOfFile;
    break;
      
  case tok::l_brace:
    Tok.Kind = MMToken::LBrace;
    break;

  case tok::l_square:
    Tok.Kind = MMToken::LSquare;
    break;
      
  case tok::period:
    Tok.Kind = MMToken::Period;
    break;
      
  case tok::r_brace:
    Tok.Kind = MMToken::RBrace;
    break;
      
  case tok::r_square:
    Tok.Kind = MMToken::RSquare;
    break;
      
  case tok::star:
    Tok.Kind = MMToken::Star;
    break;
      
  case tok::exclaim:
    Tok.Kind = MMToken::Exclaim;
    break;
      
  case tok::string_literal: {
    if (LToken.hasUDSuffix()) {
      Diags.Report(LToken.getLocation(), diag::err_invalid_string_udl);
      HadError = true;
      goto retry;
    }

    // Parse the string literal.
    LangOptions LangOpts;
    StringLiteralParser StringLiteral(&LToken, 1, SourceMgr, LangOpts, *Target);
    if (StringLiteral.hadError)
      goto retry;
    
    // Copy the string literal into our string data allocator.
    unsigned Length = StringLiteral.GetStringLength();
    char *Saved = StringData.Allocate<char>(Length + 1);
    memcpy(Saved, StringLiteral.GetString().data(), Length);
    Saved[Length] = 0;
    
    // Form the token.
    Tok.Kind = MMToken::StringLiteral;
    Tok.StringData = Saved;
    Tok.StringLength = Length;
    break;
  }
      
  case tok::comment:
    goto retry;
      
  default:
    Diags.Report(LToken.getLocation(), diag::err_mmap_unknown_token);
    HadError = true;
    goto retry;
  }
  
  return Result;
}

void ModuleMapParser::skipUntil(MMToken::TokenKind K) {
  unsigned braceDepth = 0;
  unsigned squareDepth = 0;
  do {
    switch (Tok.Kind) {
    case MMToken::EndOfFile:
      return;

    case MMToken::LBrace:
      if (Tok.is(K) && braceDepth == 0 && squareDepth == 0)
        return;
        
      ++braceDepth;
      break;

    case MMToken::LSquare:
      if (Tok.is(K) && braceDepth == 0 && squareDepth == 0)
        return;
      
      ++squareDepth;
      break;

    case MMToken::RBrace:
      if (braceDepth > 0)
        --braceDepth;
      else if (Tok.is(K))
        return;
      break;

    case MMToken::RSquare:
      if (squareDepth > 0)
        --squareDepth;
      else if (Tok.is(K))
        return;
      break;

    default:
      if (braceDepth == 0 && squareDepth == 0 && Tok.is(K))
        return;
      break;
    }
    
   consumeToken();
  } while (true);
}

/// \brief Parse a module-id.
///
///   module-id:
///     identifier
///     identifier '.' module-id
///
/// \returns true if an error occurred, false otherwise.
bool ModuleMapParser::parseModuleId(ModuleId &Id) {
  Id.clear();
  do {
    if (Tok.is(MMToken::Identifier) || Tok.is(MMToken::StringLiteral)) {
      Id.push_back(std::make_pair(Tok.getString(), Tok.getLocation()));
      consumeToken();
    } else {
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_module_name);
      return true;
    }
    
    if (!Tok.is(MMToken::Period))
      break;
    
    consumeToken();
  } while (true);
  
  return false;
}

namespace {
  /// \brief Enumerates the known attributes.
  enum AttributeKind {
    /// \brief An unknown attribute.
    AT_unknown,
    /// \brief The 'system' attribute.
    AT_system,
    /// \brief The 'exhaustive' attribute.
    AT_exhaustive
  };
}

/// \brief Parse a module declaration.
///
///   module-declaration:
///     'extern' 'module' module-id string-literal
///     'explicit'[opt] 'framework'[opt] 'module' module-id attributes[opt] 
///       { module-member* }
///
///   module-member:
///     requires-declaration
///     header-declaration
///     submodule-declaration
///     export-declaration
///     link-declaration
///
///   submodule-declaration:
///     module-declaration
///     inferred-submodule-declaration
void ModuleMapParser::parseModuleDecl() {
  assert(Tok.is(MMToken::ExplicitKeyword) || Tok.is(MMToken::ModuleKeyword) ||
         Tok.is(MMToken::FrameworkKeyword) || Tok.is(MMToken::ExternKeyword));
  if (Tok.is(MMToken::ExternKeyword)) {
    parseExternModuleDecl();
    return;
  }

  // Parse 'explicit' or 'framework' keyword, if present.
  SourceLocation ExplicitLoc;
  bool Explicit = false;
  bool Framework = false;

  // Parse 'explicit' keyword, if present.
  if (Tok.is(MMToken::ExplicitKeyword)) {
    ExplicitLoc = consumeToken();
    Explicit = true;
  }

  // Parse 'framework' keyword, if present.
  if (Tok.is(MMToken::FrameworkKeyword)) {
    consumeToken();
    Framework = true;
  } 
  
  // Parse 'module' keyword.
  if (!Tok.is(MMToken::ModuleKeyword)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_module);
    consumeToken();
    HadError = true;
    return;
  }
  consumeToken(); // 'module' keyword

  // If we have a wildcard for the module name, this is an inferred submodule.
  // Parse it. 
  if (Tok.is(MMToken::Star))
    return parseInferredModuleDecl(Framework, Explicit);
  
  // Parse the module name.
  ModuleId Id;
  if (parseModuleId(Id)) {
    HadError = true;
    return;
  }

  if (ActiveModule) {
    if (Id.size() > 1) {
      Diags.Report(Id.front().second, diag::err_mmap_nested_submodule_id)
        << SourceRange(Id.front().second, Id.back().second);
      
      HadError = true;
      return;
    }
  } else if (Id.size() == 1 && Explicit) {
    // Top-level modules can't be explicit.
    Diags.Report(ExplicitLoc, diag::err_mmap_explicit_top_level);
    Explicit = false;
    ExplicitLoc = SourceLocation();
    HadError = true;
  }
  
  Module *PreviousActiveModule = ActiveModule;  
  if (Id.size() > 1) {
    // This module map defines a submodule. Go find the module of which it
    // is a submodule.
    ActiveModule = 0;
    for (unsigned I = 0, N = Id.size() - 1; I != N; ++I) {
      if (Module *Next = Map.lookupModuleQualified(Id[I].first, ActiveModule)) {
        ActiveModule = Next;
        continue;
      }
      
      if (ActiveModule) {
        Diags.Report(Id[I].second, diag::err_mmap_missing_module_qualified)
          << Id[I].first << ActiveModule->getTopLevelModule();
      } else {
        Diags.Report(Id[I].second, diag::err_mmap_expected_module_name);
      }
      HadError = true;
      return;
    }
  } 
  
  StringRef ModuleName = Id.back().first;
  SourceLocation ModuleNameLoc = Id.back().second;
  
  // Parse the optional attribute list.
  Attributes Attrs;
  parseOptionalAttributes(Attrs);
  
  // Parse the opening brace.
  if (!Tok.is(MMToken::LBrace)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_lbrace)
      << ModuleName;
    HadError = true;
    return;
  }  
  SourceLocation LBraceLoc = consumeToken();
  
  // Determine whether this (sub)module has already been defined.
  if (Module *Existing = Map.lookupModuleQualified(ModuleName, ActiveModule)) {
    if (Existing->DefinitionLoc.isInvalid() && !ActiveModule) {
      // Skip the module definition.
      skipUntil(MMToken::RBrace);
      if (Tok.is(MMToken::RBrace))
        consumeToken();
      else {
        Diags.Report(Tok.getLocation(), diag::err_mmap_expected_rbrace);
        Diags.Report(LBraceLoc, diag::note_mmap_lbrace_match);
        HadError = true;        
      }
      return;
    }
    
    Diags.Report(ModuleNameLoc, diag::err_mmap_module_redefinition)
      << ModuleName;
    Diags.Report(Existing->DefinitionLoc, diag::note_mmap_prev_definition);
    
    // Skip the module definition.
    skipUntil(MMToken::RBrace);
    if (Tok.is(MMToken::RBrace))
      consumeToken();
    
    HadError = true;
    return;
  }

  // Start defining this module.
  ActiveModule = Map.findOrCreateModule(ModuleName, ActiveModule, Framework,
                                        Explicit).first;
  ActiveModule->DefinitionLoc = ModuleNameLoc;
  if (Attrs.IsSystem || IsSystem)
    ActiveModule->IsSystem = true;
  
  bool Done = false;
  do {
    switch (Tok.Kind) {
    case MMToken::EndOfFile:
    case MMToken::RBrace:
      Done = true;
      break;

    case MMToken::ConfigMacros:
      parseConfigMacros();
      break;

    case MMToken::Conflict:
      parseConflict();
      break;

    case MMToken::ExplicitKeyword:
    case MMToken::ExternKeyword:
    case MMToken::FrameworkKeyword:
    case MMToken::ModuleKeyword:
      parseModuleDecl();
      break;

    case MMToken::ExportKeyword:
      parseExportDecl();
      break;

    case MMToken::UseKeyword:
      parseUseDecl();
      break;
        
    case MMToken::RequiresKeyword:
      parseRequiresDecl();
      break;

    case MMToken::UmbrellaKeyword: {
      SourceLocation UmbrellaLoc = consumeToken();
      if (Tok.is(MMToken::HeaderKeyword))
        parseHeaderDecl(MMToken::UmbrellaKeyword, UmbrellaLoc);
      else
        parseUmbrellaDirDecl(UmbrellaLoc);
      break;
    }
        
    case MMToken::ExcludeKeyword: {
      SourceLocation ExcludeLoc = consumeToken();
      if (Tok.is(MMToken::HeaderKeyword)) {
        parseHeaderDecl(MMToken::ExcludeKeyword, ExcludeLoc);
      } else {
        Diags.Report(Tok.getLocation(), diag::err_mmap_expected_header)
          << "exclude";
      }
      break;
    }
      
    case MMToken::PrivateKeyword: {
      SourceLocation PrivateLoc = consumeToken();
      if (Tok.is(MMToken::HeaderKeyword)) {
        parseHeaderDecl(MMToken::PrivateKeyword, PrivateLoc);
      } else {
        Diags.Report(Tok.getLocation(), diag::err_mmap_expected_header)
          << "private";
      }
      break;
    }
      
    case MMToken::HeaderKeyword:
      parseHeaderDecl(MMToken::HeaderKeyword, SourceLocation());
      break;

    case MMToken::LinkKeyword:
      parseLinkDecl();
      break;

    default:
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_member);
      consumeToken();
      break;        
    }
  } while (!Done);

  if (Tok.is(MMToken::RBrace))
    consumeToken();
  else {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_rbrace);
    Diags.Report(LBraceLoc, diag::note_mmap_lbrace_match);
    HadError = true;
  }

  // If the active module is a top-level framework, and there are no link
  // libraries, automatically link against the framework.
  if (ActiveModule->IsFramework && !ActiveModule->isSubFramework() &&
      ActiveModule->LinkLibraries.empty()) {
    inferFrameworkLink(ActiveModule, Directory, SourceMgr.getFileManager());
  }

  // We're done parsing this module. Pop back to the previous module.
  ActiveModule = PreviousActiveModule;
}

/// \brief Parse an extern module declaration.
///
///   extern module-declaration:
///     'extern' 'module' module-id string-literal
void ModuleMapParser::parseExternModuleDecl() {
  assert(Tok.is(MMToken::ExternKeyword));
  consumeToken(); // 'extern' keyword

  // Parse 'module' keyword.
  if (!Tok.is(MMToken::ModuleKeyword)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_module);
    consumeToken();
    HadError = true;
    return;
  }
  consumeToken(); // 'module' keyword

  // Parse the module name.
  ModuleId Id;
  if (parseModuleId(Id)) {
    HadError = true;
    return;
  }

  // Parse the referenced module map file name.
  if (!Tok.is(MMToken::StringLiteral)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_mmap_file);
    HadError = true;
    return;
  }
  std::string FileName = Tok.getString();
  consumeToken(); // filename

  StringRef FileNameRef = FileName;
  SmallString<128> ModuleMapFileName;
  if (llvm::sys::path::is_relative(FileNameRef)) {
    ModuleMapFileName += Directory->getName();
    llvm::sys::path::append(ModuleMapFileName, FileName);
    FileNameRef = ModuleMapFileName.str();
  }
  if (const FileEntry *File = SourceMgr.getFileManager().getFile(FileNameRef))
    Map.parseModuleMapFile(File, /*IsSystem=*/false);
}

/// \brief Parse a requires declaration.
///
///   requires-declaration:
///     'requires' feature-list
///
///   feature-list:
///     feature ',' feature-list
///     feature
///
///   feature:
///     '!'[opt] identifier
void ModuleMapParser::parseRequiresDecl() {
  assert(Tok.is(MMToken::RequiresKeyword));

  // Parse 'requires' keyword.
  consumeToken();

  // Parse the feature-list.
  do {
    bool RequiredState = true;
    if (Tok.is(MMToken::Exclaim)) {
      RequiredState = false;
      consumeToken();
    }

    if (!Tok.is(MMToken::Identifier)) {
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_feature);
      HadError = true;
      return;
    }

    // Consume the feature name.
    std::string Feature = Tok.getString();
    consumeToken();

    // Add this feature.
    ActiveModule->addRequirement(Feature, RequiredState,
                                 Map.LangOpts, *Map.Target);

    if (!Tok.is(MMToken::Comma))
      break;

    // Consume the comma.
    consumeToken();
  } while (true);
}

/// \brief Append to \p Paths the set of paths needed to get to the 
/// subframework in which the given module lives.
static void appendSubframeworkPaths(Module *Mod,
                                    SmallVectorImpl<char> &Path) {
  // Collect the framework names from the given module to the top-level module.
  SmallVector<StringRef, 2> Paths;
  for (; Mod; Mod = Mod->Parent) {
    if (Mod->IsFramework)
      Paths.push_back(Mod->Name);
  }
  
  if (Paths.empty())
    return;
  
  // Add Frameworks/Name.framework for each subframework.
  for (unsigned I = Paths.size() - 1; I != 0; --I)
    llvm::sys::path::append(Path, "Frameworks", Paths[I-1] + ".framework");
}

/// \brief Parse a header declaration.
///
///   header-declaration:
///     'umbrella'[opt] 'header' string-literal
///     'exclude'[opt] 'header' string-literal
void ModuleMapParser::parseHeaderDecl(MMToken::TokenKind LeadingToken,
                                      SourceLocation LeadingLoc) {
  assert(Tok.is(MMToken::HeaderKeyword));
  consumeToken();

  // Parse the header name.
  if (!Tok.is(MMToken::StringLiteral)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_header) 
      << "header";
    HadError = true;
    return;
  }
  Module::HeaderDirective Header;
  Header.FileName = Tok.getString();
  Header.FileNameLoc = consumeToken();
  
  // Check whether we already have an umbrella.
  if (LeadingToken == MMToken::UmbrellaKeyword && ActiveModule->Umbrella) {
    Diags.Report(Header.FileNameLoc, diag::err_mmap_umbrella_clash)
      << ActiveModule->getFullModuleName();
    HadError = true;
    return;
  }

  // Look for this file.
  const FileEntry *File = 0;
  const FileEntry *BuiltinFile = 0;
  SmallString<128> PathName;
  if (llvm::sys::path::is_absolute(Header.FileName)) {
    PathName = Header.FileName;
    File = SourceMgr.getFileManager().getFile(PathName);
  } else if (const DirectoryEntry *Dir = getOverriddenHeaderSearchDir()) {
    PathName = Dir->getName();
    llvm::sys::path::append(PathName, Header.FileName);
    File = SourceMgr.getFileManager().getFile(PathName);
  } else {
    // Search for the header file within the search directory.
    PathName = Directory->getName();
    unsigned PathLength = PathName.size();
    
    if (ActiveModule->isPartOfFramework()) {
      appendSubframeworkPaths(ActiveModule, PathName);
      
      // Check whether this file is in the public headers.
      llvm::sys::path::append(PathName, "Headers", Header.FileName);
      File = SourceMgr.getFileManager().getFile(PathName);
      
      if (!File) {
        // Check whether this file is in the private headers.
        PathName.resize(PathLength);
        llvm::sys::path::append(PathName, "PrivateHeaders", Header.FileName);
        File = SourceMgr.getFileManager().getFile(PathName);
      }
    } else {
      // Lookup for normal headers.
      llvm::sys::path::append(PathName, Header.FileName);
      File = SourceMgr.getFileManager().getFile(PathName);
      
      // If this is a system module with a top-level header, this header
      // may have a counterpart (or replacement) in the set of headers
      // supplied by Clang. Find that builtin header.
      if (ActiveModule->IsSystem && LeadingToken != MMToken::UmbrellaKeyword &&
          BuiltinIncludeDir && BuiltinIncludeDir != Directory &&
          isBuiltinHeader(Header.FileName)) {
        SmallString<128> BuiltinPathName(BuiltinIncludeDir->getName());
        llvm::sys::path::append(BuiltinPathName, Header.FileName);
        BuiltinFile = SourceMgr.getFileManager().getFile(BuiltinPathName);
        
        // If Clang supplies this header but the underlying system does not,
        // just silently swap in our builtin version. Otherwise, we'll end
        // up adding both (later).
        if (!File && BuiltinFile) {
          File = BuiltinFile;
          BuiltinFile = 0;
        }
      }
    }
  }
  
  // FIXME: We shouldn't be eagerly stat'ing every file named in a module map.
  // Come up with a lazy way to do this.
  if (File) {
    if (LeadingToken == MMToken::UmbrellaKeyword) {
      const DirectoryEntry *UmbrellaDir = File->getDir();
      if (Module *UmbrellaModule = Map.UmbrellaDirs[UmbrellaDir]) {
        Diags.Report(LeadingLoc, diag::err_mmap_umbrella_clash)
          << UmbrellaModule->getFullModuleName();
        HadError = true;
      } else {
        // Record this umbrella header.
        Map.setUmbrellaHeader(ActiveModule, File);
      }
    } else {
      // Record this header.
      ModuleMap::ModuleHeaderRole Role = ModuleMap::NormalHeader;
      if (LeadingToken == MMToken::ExcludeKeyword)
        Role = ModuleMap::ExcludedHeader;
      else if (LeadingToken == MMToken::PrivateKeyword)
        Role = ModuleMap::PrivateHeader;
      else
        assert(LeadingToken == MMToken::HeaderKeyword);
        
      Map.addHeader(ActiveModule, File, Role);
      
      // If there is a builtin counterpart to this file, add it now.
      if (BuiltinFile)
        Map.addHeader(ActiveModule, BuiltinFile, Role);
    }
  } else if (LeadingToken != MMToken::ExcludeKeyword) {
    // Ignore excluded header files. They're optional anyway.

    // If we find a module that has a missing header, we mark this module as
    // unavailable and store the header directive for displaying diagnostics.
    // Other submodules in the same module can still be used.
    Header.IsUmbrella = LeadingToken == MMToken::UmbrellaKeyword;
    ActiveModule->IsAvailable = false;
    ActiveModule->MissingHeaders.push_back(Header);
  }
}

/// \brief Parse an umbrella directory declaration.
///
///   umbrella-dir-declaration:
///     umbrella string-literal
void ModuleMapParser::parseUmbrellaDirDecl(SourceLocation UmbrellaLoc) {
  // Parse the directory name.
  if (!Tok.is(MMToken::StringLiteral)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_header) 
      << "umbrella";
    HadError = true;
    return;
  }

  std::string DirName = Tok.getString();
  SourceLocation DirNameLoc = consumeToken();
  
  // Check whether we already have an umbrella.
  if (ActiveModule->Umbrella) {
    Diags.Report(DirNameLoc, diag::err_mmap_umbrella_clash)
      << ActiveModule->getFullModuleName();
    HadError = true;
    return;
  }

  // Look for this file.
  const DirectoryEntry *Dir = 0;
  if (llvm::sys::path::is_absolute(DirName))
    Dir = SourceMgr.getFileManager().getDirectory(DirName);
  else {
    SmallString<128> PathName;
    PathName = Directory->getName();
    llvm::sys::path::append(PathName, DirName);
    Dir = SourceMgr.getFileManager().getDirectory(PathName);
  }
  
  if (!Dir) {
    Diags.Report(DirNameLoc, diag::err_mmap_umbrella_dir_not_found)
      << DirName;
    HadError = true;
    return;
  }
  
  if (Module *OwningModule = Map.UmbrellaDirs[Dir]) {
    Diags.Report(UmbrellaLoc, diag::err_mmap_umbrella_clash)
      << OwningModule->getFullModuleName();
    HadError = true;
    return;
  } 
  
  // Record this umbrella directory.
  Map.setUmbrellaDir(ActiveModule, Dir);
}

/// \brief Parse a module export declaration.
///
///   export-declaration:
///     'export' wildcard-module-id
///
///   wildcard-module-id:
///     identifier
///     '*'
///     identifier '.' wildcard-module-id
void ModuleMapParser::parseExportDecl() {
  assert(Tok.is(MMToken::ExportKeyword));
  SourceLocation ExportLoc = consumeToken();
  
  // Parse the module-id with an optional wildcard at the end.
  ModuleId ParsedModuleId;
  bool Wildcard = false;
  do {
    if (Tok.is(MMToken::Identifier)) {
      ParsedModuleId.push_back(std::make_pair(Tok.getString(), 
                                              Tok.getLocation()));
      consumeToken();
      
      if (Tok.is(MMToken::Period)) {
        consumeToken();
        continue;
      } 
      
      break;
    }
    
    if(Tok.is(MMToken::Star)) {
      Wildcard = true;
      consumeToken();
      break;
    }
    
    Diags.Report(Tok.getLocation(), diag::err_mmap_module_id);
    HadError = true;
    return;
  } while (true);
  
  Module::UnresolvedExportDecl Unresolved = { 
    ExportLoc, ParsedModuleId, Wildcard 
  };
  ActiveModule->UnresolvedExports.push_back(Unresolved);
}

/// \brief Parse a module uses declaration.
///
///   uses-declaration:
///     'uses' wildcard-module-id
void ModuleMapParser::parseUseDecl() {
  assert(Tok.is(MMToken::UseKeyword));
  consumeToken();
  // Parse the module-id.
  ModuleId ParsedModuleId;
  parseModuleId(ParsedModuleId);

  ActiveModule->UnresolvedDirectUses.push_back(ParsedModuleId);
}

/// \brief Parse a link declaration.
///
///   module-declaration:
///     'link' 'framework'[opt] string-literal
void ModuleMapParser::parseLinkDecl() {
  assert(Tok.is(MMToken::LinkKeyword));
  SourceLocation LinkLoc = consumeToken();

  // Parse the optional 'framework' keyword.
  bool IsFramework = false;
  if (Tok.is(MMToken::FrameworkKeyword)) {
    consumeToken();
    IsFramework = true;
  }

  // Parse the library name
  if (!Tok.is(MMToken::StringLiteral)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_library_name)
      << IsFramework << SourceRange(LinkLoc);
    HadError = true;
    return;
  }

  std::string LibraryName = Tok.getString();
  consumeToken();
  ActiveModule->LinkLibraries.push_back(Module::LinkLibrary(LibraryName,
                                                            IsFramework));
}

/// \brief Parse a configuration macro declaration.
///
///   module-declaration:
///     'config_macros' attributes[opt] config-macro-list?
///
///   config-macro-list:
///     identifier (',' identifier)?
void ModuleMapParser::parseConfigMacros() {
  assert(Tok.is(MMToken::ConfigMacros));
  SourceLocation ConfigMacrosLoc = consumeToken();

  // Only top-level modules can have configuration macros.
  if (ActiveModule->Parent) {
    Diags.Report(ConfigMacrosLoc, diag::err_mmap_config_macro_submodule);
  }

  // Parse the optional attributes.
  Attributes Attrs;
  parseOptionalAttributes(Attrs);
  if (Attrs.IsExhaustive && !ActiveModule->Parent) {
    ActiveModule->ConfigMacrosExhaustive = true;
  }

  // If we don't have an identifier, we're done.
  if (!Tok.is(MMToken::Identifier))
    return;

  // Consume the first identifier.
  if (!ActiveModule->Parent) {
    ActiveModule->ConfigMacros.push_back(Tok.getString().str());
  }
  consumeToken();

  do {
    // If there's a comma, consume it.
    if (!Tok.is(MMToken::Comma))
      break;
    consumeToken();

    // We expect to see a macro name here.
    if (!Tok.is(MMToken::Identifier)) {
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_config_macro);
      break;
    }

    // Consume the macro name.
    if (!ActiveModule->Parent) {
      ActiveModule->ConfigMacros.push_back(Tok.getString().str());
    }
    consumeToken();
  } while (true);
}

/// \brief Format a module-id into a string.
static std::string formatModuleId(const ModuleId &Id) {
  std::string result;
  {
    llvm::raw_string_ostream OS(result);

    for (unsigned I = 0, N = Id.size(); I != N; ++I) {
      if (I)
        OS << ".";
      OS << Id[I].first;
    }
  }

  return result;
}

/// \brief Parse a conflict declaration.
///
///   module-declaration:
///     'conflict' module-id ',' string-literal
void ModuleMapParser::parseConflict() {
  assert(Tok.is(MMToken::Conflict));
  SourceLocation ConflictLoc = consumeToken();
  Module::UnresolvedConflict Conflict;

  // Parse the module-id.
  if (parseModuleId(Conflict.Id))
    return;

  // Parse the ','.
  if (!Tok.is(MMToken::Comma)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_conflicts_comma)
      << SourceRange(ConflictLoc);
    return;
  }
  consumeToken();

  // Parse the message.
  if (!Tok.is(MMToken::StringLiteral)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_conflicts_message)
      << formatModuleId(Conflict.Id);
    return;
  }
  Conflict.Message = Tok.getString().str();
  consumeToken();

  // Add this unresolved conflict.
  ActiveModule->UnresolvedConflicts.push_back(Conflict);
}

/// \brief Parse an inferred module declaration (wildcard modules).
///
///   module-declaration:
///     'explicit'[opt] 'framework'[opt] 'module' * attributes[opt]
///       { inferred-module-member* }
///
///   inferred-module-member:
///     'export' '*'
///     'exclude' identifier
void ModuleMapParser::parseInferredModuleDecl(bool Framework, bool Explicit) {
  assert(Tok.is(MMToken::Star));
  SourceLocation StarLoc = consumeToken();
  bool Failed = false;

  // Inferred modules must be submodules.
  if (!ActiveModule && !Framework) {
    Diags.Report(StarLoc, diag::err_mmap_top_level_inferred_submodule);
    Failed = true;
  }

  if (ActiveModule) {
    // Inferred modules must have umbrella directories.
    if (!Failed && !ActiveModule->getUmbrellaDir()) {
      Diags.Report(StarLoc, diag::err_mmap_inferred_no_umbrella);
      Failed = true;
    }
    
    // Check for redefinition of an inferred module.
    if (!Failed && ActiveModule->InferSubmodules) {
      Diags.Report(StarLoc, diag::err_mmap_inferred_redef);
      if (ActiveModule->InferredSubmoduleLoc.isValid())
        Diags.Report(ActiveModule->InferredSubmoduleLoc,
                     diag::note_mmap_prev_definition);
      Failed = true;
    }

    // Check for the 'framework' keyword, which is not permitted here.
    if (Framework) {
      Diags.Report(StarLoc, diag::err_mmap_inferred_framework_submodule);
      Framework = false;
    }
  } else if (Explicit) {
    Diags.Report(StarLoc, diag::err_mmap_explicit_inferred_framework);
    Explicit = false;
  }

  // If there were any problems with this inferred submodule, skip its body.
  if (Failed) {
    if (Tok.is(MMToken::LBrace)) {
      consumeToken();
      skipUntil(MMToken::RBrace);
      if (Tok.is(MMToken::RBrace))
        consumeToken();
    }
    HadError = true;
    return;
  }

  // Parse optional attributes.
  Attributes Attrs;
  parseOptionalAttributes(Attrs);

  if (ActiveModule) {
    // Note that we have an inferred submodule.
    ActiveModule->InferSubmodules = true;
    ActiveModule->InferredSubmoduleLoc = StarLoc;
    ActiveModule->InferExplicitSubmodules = Explicit;
  } else {
    // We'll be inferring framework modules for this directory.
    Map.InferredDirectories[Directory].InferModules = true;
    Map.InferredDirectories[Directory].InferSystemModules = Attrs.IsSystem;
  }

  // Parse the opening brace.
  if (!Tok.is(MMToken::LBrace)) {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_lbrace_wildcard);
    HadError = true;
    return;
  }  
  SourceLocation LBraceLoc = consumeToken();

  // Parse the body of the inferred submodule.
  bool Done = false;
  do {
    switch (Tok.Kind) {
    case MMToken::EndOfFile:
    case MMToken::RBrace:
      Done = true;
      break;

    case MMToken::ExcludeKeyword: {
      if (ActiveModule) {
        Diags.Report(Tok.getLocation(), diag::err_mmap_expected_inferred_member)
          << (ActiveModule != 0);
        consumeToken();
        break;
      }

      consumeToken();
      if (!Tok.is(MMToken::Identifier)) {
        Diags.Report(Tok.getLocation(), diag::err_mmap_missing_exclude_name);
        break;
      }

      Map.InferredDirectories[Directory].ExcludedModules
        .push_back(Tok.getString());
      consumeToken();
      break;
    }

    case MMToken::ExportKeyword:
      if (!ActiveModule) {
        Diags.Report(Tok.getLocation(), diag::err_mmap_expected_inferred_member)
          << (ActiveModule != 0);
        consumeToken();
        break;
      }

      consumeToken();
      if (Tok.is(MMToken::Star)) 
        ActiveModule->InferExportWildcard = true;
      else
        Diags.Report(Tok.getLocation(), 
                     diag::err_mmap_expected_export_wildcard);
      consumeToken();
      break;

    case MMToken::ExplicitKeyword:
    case MMToken::ModuleKeyword:
    case MMToken::HeaderKeyword:
    case MMToken::PrivateKeyword:
    case MMToken::UmbrellaKeyword:
    default:
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_inferred_member)
          << (ActiveModule != 0);
      consumeToken();
      break;        
    }
  } while (!Done);
  
  if (Tok.is(MMToken::RBrace))
    consumeToken();
  else {
    Diags.Report(Tok.getLocation(), diag::err_mmap_expected_rbrace);
    Diags.Report(LBraceLoc, diag::note_mmap_lbrace_match);
    HadError = true;
  }
}

/// \brief Parse optional attributes.
///
///   attributes:
///     attribute attributes
///     attribute
///
///   attribute:
///     [ identifier ]
///
/// \param Attrs Will be filled in with the parsed attributes.
///
/// \returns true if an error occurred, false otherwise.
bool ModuleMapParser::parseOptionalAttributes(Attributes &Attrs) {
  bool HadError = false;
  
  while (Tok.is(MMToken::LSquare)) {
    // Consume the '['.
    SourceLocation LSquareLoc = consumeToken();

    // Check whether we have an attribute name here.
    if (!Tok.is(MMToken::Identifier)) {
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_attribute);
      skipUntil(MMToken::RSquare);
      if (Tok.is(MMToken::RSquare))
        consumeToken();
      HadError = true;
    }

    // Decode the attribute name.
    AttributeKind Attribute
      = llvm::StringSwitch<AttributeKind>(Tok.getString())
          .Case("exhaustive", AT_exhaustive)
          .Case("system", AT_system)
          .Default(AT_unknown);
    switch (Attribute) {
    case AT_unknown:
      Diags.Report(Tok.getLocation(), diag::warn_mmap_unknown_attribute)
        << Tok.getString();
      break;

    case AT_system:
      Attrs.IsSystem = true;
      break;

    case AT_exhaustive:
      Attrs.IsExhaustive = true;
      break;
    }
    consumeToken();

    // Consume the ']'.
    if (!Tok.is(MMToken::RSquare)) {
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_rsquare);
      Diags.Report(LSquareLoc, diag::note_mmap_lsquare_match);
      skipUntil(MMToken::RSquare);
      HadError = true;
    }

    if (Tok.is(MMToken::RSquare))
      consumeToken();
  }

  return HadError;
}

/// \brief If there is a specific header search directory due the presence
/// of an umbrella directory, retrieve that directory. Otherwise, returns null.
const DirectoryEntry *ModuleMapParser::getOverriddenHeaderSearchDir() {
  for (Module *Mod = ActiveModule; Mod; Mod = Mod->Parent) {
    // If we have an umbrella directory, use that.
    if (Mod->hasUmbrellaDir())
      return Mod->getUmbrellaDir();
    
    // If we have a framework directory, stop looking.
    if (Mod->IsFramework)
      return 0;
  }
  
  return 0;
}

/// \brief Parse a module map file.
///
///   module-map-file:
///     module-declaration*
bool ModuleMapParser::parseModuleMapFile() {
  do {
    switch (Tok.Kind) {
    case MMToken::EndOfFile:
      return HadError;
      
    case MMToken::ExplicitKeyword:
    case MMToken::ExternKeyword:
    case MMToken::ModuleKeyword:
    case MMToken::FrameworkKeyword:
      parseModuleDecl();
      break;

    case MMToken::Comma:
    case MMToken::ConfigMacros:
    case MMToken::Conflict:
    case MMToken::Exclaim:
    case MMToken::ExcludeKeyword:
    case MMToken::ExportKeyword:
    case MMToken::HeaderKeyword:
    case MMToken::Identifier:
    case MMToken::LBrace:
    case MMToken::LinkKeyword:
    case MMToken::LSquare:
    case MMToken::Period:
    case MMToken::PrivateKeyword:
    case MMToken::RBrace:
    case MMToken::RSquare:
    case MMToken::RequiresKeyword:
    case MMToken::Star:
    case MMToken::StringLiteral:
    case MMToken::UmbrellaKeyword:
    case MMToken::UseKeyword:
      Diags.Report(Tok.getLocation(), diag::err_mmap_expected_module);
      HadError = true;
      consumeToken();
      break;
    }
  } while (true);
}

bool ModuleMap::parseModuleMapFile(const FileEntry *File, bool IsSystem) {
  llvm::DenseMap<const FileEntry *, bool>::iterator Known
    = ParsedModuleMap.find(File);
  if (Known != ParsedModuleMap.end())
    return Known->second;

  assert(Target != 0 && "Missing target information");
  FileID ID = SourceMgr.createFileID(File, SourceLocation(), SrcMgr::C_User);
  const llvm::MemoryBuffer *Buffer = SourceMgr.getBuffer(ID);
  if (!Buffer)
    return ParsedModuleMap[File] = true;
  
  // Parse this module map file.
  Lexer L(ID, SourceMgr.getBuffer(ID), SourceMgr, MMapLangOpts);
  ModuleMapParser Parser(L, SourceMgr, Target, Diags, *this, File->getDir(),
                         BuiltinIncludeDir, IsSystem);
  bool Result = Parser.parseModuleMapFile();
  ParsedModuleMap[File] = Result;
  return Result;
}
