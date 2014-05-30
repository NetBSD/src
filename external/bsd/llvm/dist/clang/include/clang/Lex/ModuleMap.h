//===--- ModuleMap.h - Describe the layout of modules -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the ModuleMap interface, which describes the layout of a
// module as it relates to headers.
//
//===----------------------------------------------------------------------===//


#ifndef LLVM_CLANG_LEX_MODULEMAP_H
#define LLVM_CLANG_LEX_MODULEMAP_H

#include "clang/Basic/LangOptions.h"
#include "clang/Basic/Module.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringRef.h"
#include <string>

namespace clang {
  
class DirectoryEntry;
class FileEntry;
class FileManager;
class DiagnosticConsumer;
class DiagnosticsEngine;
class HeaderSearch;
class ModuleMapParser;
  
class ModuleMap {
  SourceManager &SourceMgr;
  DiagnosticsEngine &Diags;
  const LangOptions &LangOpts;
  const TargetInfo *Target;
  HeaderSearch &HeaderInfo;
  
  /// \brief The directory used for Clang-supplied, builtin include headers,
  /// such as "stdint.h".
  const DirectoryEntry *BuiltinIncludeDir;
  
  /// \brief Language options used to parse the module map itself.
  ///
  /// These are always simple C language options.
  LangOptions MMapLangOpts;

  // The module that we are building; related to \c LangOptions::CurrentModule.
  Module *CompilingModule;

public:
  // The module that the .cc source file is associated with.
  Module *SourceModule;
  std::string SourceModuleName;

private:
  /// \brief The top-level modules that are known.
  llvm::StringMap<Module *> Modules;

public:
  /// \brief Describes the role of a module header.
  enum ModuleHeaderRole {
    /// \brief This header is normally included in the module.
    NormalHeader,
    /// \brief This header is included but private.
    PrivateHeader,
    /// \brief This header is explicitly excluded from the module.
    ExcludedHeader
    // Caution: Adding an enumerator needs other changes.
    // Adjust the number of bits for KnownHeader::Storage.
    // Adjust the bitfield HeaderFileInfo::HeaderRole size.
    // Adjust the HeaderFileInfoTrait::ReadData streaming.
    // Adjust the HeaderFileInfoTrait::EmitData streaming.
  };

  /// \brief A header that is known to reside within a given module,
  /// whether it was included or excluded.
  class KnownHeader {
    llvm::PointerIntPair<Module *, 2, ModuleHeaderRole> Storage;

  public:
    KnownHeader() : Storage(nullptr, NormalHeader) { }
    KnownHeader(Module *M, ModuleHeaderRole Role) : Storage(M, Role) { }

    /// \brief Retrieve the module the header is stored in.
    Module *getModule() const { return Storage.getPointer(); }

    /// \brief The role of this header within the module.
    ModuleHeaderRole getRole() const { return Storage.getInt(); }

    /// \brief Whether this header is available in the module.
    bool isAvailable() const { 
      return getRole() != ExcludedHeader && getModule()->isAvailable(); 
    }

    // \brief Whether this known header is valid (i.e., it has an
    // associated module).
    LLVM_EXPLICIT operator bool() const {
      return Storage.getPointer() != nullptr;
    }
  };

private:
  typedef llvm::DenseMap<const FileEntry *, SmallVector<KnownHeader, 1> >
  HeadersMap;

  /// \brief Mapping from each header to the module that owns the contents of
  /// that header.
  HeadersMap Headers;
  
  /// \brief Mapping from directories with umbrella headers to the module
  /// that is generated from the umbrella header.
  ///
  /// This mapping is used to map headers that haven't explicitly been named
  /// in the module map over to the module that includes them via its umbrella
  /// header.
  llvm::DenseMap<const DirectoryEntry *, Module *> UmbrellaDirs;

  /// \brief A directory for which framework modules can be inferred.
  struct InferredDirectory {
    InferredDirectory() : InferModules(), InferSystemModules() { }

    /// \brief Whether to infer modules from this directory.
    unsigned InferModules : 1;

    /// \brief Whether the modules we infer are [system] modules.
    unsigned InferSystemModules : 1;

    /// \brief If \c InferModules is non-zero, the module map file that allowed
    /// inferred modules.  Otherwise, nullptr.
    const FileEntry *ModuleMapFile;

    /// \brief The names of modules that cannot be inferred within this
    /// directory.
    SmallVector<std::string, 2> ExcludedModules;
  };

  /// \brief A mapping from directories to information about inferring
  /// framework modules from within those directories.
  llvm::DenseMap<const DirectoryEntry *, InferredDirectory> InferredDirectories;

  /// \brief Describes whether we haved parsed a particular file as a module
  /// map.
  llvm::DenseMap<const FileEntry *, bool> ParsedModuleMap;

  friend class ModuleMapParser;
  
  /// \brief Resolve the given export declaration into an actual export
  /// declaration.
  ///
  /// \param Mod The module in which we're resolving the export declaration.
  ///
  /// \param Unresolved The export declaration to resolve.
  ///
  /// \param Complain Whether this routine should complain about unresolvable
  /// exports.
  ///
  /// \returns The resolved export declaration, which will have a NULL pointer
  /// if the export could not be resolved.
  Module::ExportDecl 
  resolveExport(Module *Mod, const Module::UnresolvedExportDecl &Unresolved,
                bool Complain) const;

  /// \brief Resolve the given module id to an actual module.
  ///
  /// \param Id The module-id to resolve.
  ///
  /// \param Mod The module in which we're resolving the module-id.
  ///
  /// \param Complain Whether this routine should complain about unresolvable
  /// module-ids.
  ///
  /// \returns The resolved module, or null if the module-id could not be
  /// resolved.
  Module *resolveModuleId(const ModuleId &Id, Module *Mod, bool Complain) const;

  /// \brief Looks up the modules that \p File corresponds to.
  ///
  /// If \p File represents a builtin header within Clang's builtin include
  /// directory, this also loads all of the module maps to see if it will get
  /// associated with a specific module (e.g. in /usr/include).
  HeadersMap::iterator findKnownHeader(const FileEntry *File);

  /// \brief Searches for a module whose umbrella directory contains \p File.
  ///
  /// \param File The header to search for.
  ///
  /// \param IntermediateDirs On success, contains the set of directories
  /// searched before finding \p File.
  KnownHeader findHeaderInUmbrellaDirs(const FileEntry *File,
                    SmallVectorImpl<const DirectoryEntry *> &IntermediateDirs);

  /// \brief A convenience method to determine if \p File is (possibly nested)
  /// in an umbrella directory.
  bool isHeaderInUmbrellaDirs(const FileEntry *File) {
    SmallVector<const DirectoryEntry *, 2> IntermediateDirs;
    return static_cast<bool>(findHeaderInUmbrellaDirs(File, IntermediateDirs));
  }

public:
  /// \brief Construct a new module map.
  ///
  /// \param SourceMgr The source manager used to find module files and headers.
  /// This source manager should be shared with the header-search mechanism,
  /// since they will refer to the same headers.
  ///
  /// \param Diags A diagnostic engine used for diagnostics.
  ///
  /// \param LangOpts Language options for this translation unit.
  ///
  /// \param Target The target for this translation unit.
  ModuleMap(SourceManager &SourceMgr, DiagnosticsEngine &Diags,
            const LangOptions &LangOpts, const TargetInfo *Target,
            HeaderSearch &HeaderInfo);

  /// \brief Destroy the module map.
  ///
  ~ModuleMap();

  /// \brief Set the target information.
  void setTarget(const TargetInfo &Target);

  /// \brief Set the directory that contains Clang-supplied include
  /// files, such as our stdarg.h or tgmath.h.
  void setBuiltinIncludeDir(const DirectoryEntry *Dir) {
    BuiltinIncludeDir = Dir;
  }

  /// \brief Retrieve the module that owns the given header file, if any.
  ///
  /// \param File The header file that is likely to be included.
  ///
  /// \param RequestingModule Specifies the module the header is intended to be
  /// used from.  Used to disambiguate if a header is present in multiple
  /// modules.
  ///
  /// \returns The module KnownHeader, which provides the module that owns the
  /// given header file.  The KnownHeader is default constructed to indicate
  /// that no module owns this header file.
  KnownHeader findModuleForHeader(const FileEntry *File,
                                  Module *RequestingModule = nullptr);

  /// \brief Reports errors if a module must not include a specific file.
  ///
  /// \param RequestingModule The module including a file.
  ///
  /// \param FilenameLoc The location of the inclusion's filename.
  ///
  /// \param Filename The included filename as written.
  ///
  /// \param File The included file.
  void diagnoseHeaderInclusion(Module *RequestingModule,
                               SourceLocation FilenameLoc, StringRef Filename,
                               const FileEntry *File);

  /// \brief Determine whether the given header is part of a module
  /// marked 'unavailable'.
  bool isHeaderInUnavailableModule(const FileEntry *Header) const;

  /// \brief Determine whether the given header is unavailable as part
  /// of the specified module.
  bool isHeaderUnavailableInModule(const FileEntry *Header,
                                   const Module *RequestingModule) const;

  /// \brief Retrieve a module with the given name.
  ///
  /// \param Name The name of the module to look up.
  ///
  /// \returns The named module, if known; otherwise, returns null.
  Module *findModule(StringRef Name) const;

  /// \brief Retrieve a module with the given name using lexical name lookup,
  /// starting at the given context.
  ///
  /// \param Name The name of the module to look up.
  ///
  /// \param Context The module context, from which we will perform lexical
  /// name lookup.
  ///
  /// \returns The named module, if known; otherwise, returns null.
  Module *lookupModuleUnqualified(StringRef Name, Module *Context) const;

  /// \brief Retrieve a module with the given name within the given context,
  /// using direct (qualified) name lookup.
  ///
  /// \param Name The name of the module to look up.
  /// 
  /// \param Context The module for which we will look for a submodule. If
  /// null, we will look for a top-level module.
  ///
  /// \returns The named submodule, if known; otherwose, returns null.
  Module *lookupModuleQualified(StringRef Name, Module *Context) const;
  
  /// \brief Find a new module or submodule, or create it if it does not already
  /// exist.
  ///
  /// \param Name The name of the module to find or create.
  ///
  /// \param Parent The module that will act as the parent of this submodule,
  /// or NULL to indicate that this is a top-level module.
  ///
  /// \param ModuleMap The module map that defines or allows the inference of
  /// this module.
  ///
  /// \param IsFramework Whether this is a framework module.
  ///
  /// \param IsExplicit Whether this is an explicit submodule.
  ///
  /// \returns The found or newly-created module, along with a boolean value
  /// that will be true if the module is newly-created.
  std::pair<Module *, bool> findOrCreateModule(StringRef Name, Module *Parent,
                                               const FileEntry *ModuleMap,
                                               bool IsFramework,
                                               bool IsExplicit);

  /// \brief Determine whether we can infer a framework module a framework
  /// with the given name in the given
  ///
  /// \param ParentDir The directory that is the parent of the framework
  /// directory.
  ///
  /// \param Name The name of the module.
  ///
  /// \param IsSystem Will be set to 'true' if the inferred module must be a
  /// system module.
  ///
  /// \returns true if we are allowed to infer a framework module, and false
  /// otherwise.
  bool canInferFrameworkModule(const DirectoryEntry *ParentDir,
                               StringRef Name, bool &IsSystem) const;

  /// \brief Infer the contents of a framework module map from the given
  /// framework directory.
  Module *inferFrameworkModule(StringRef ModuleName, 
                               const DirectoryEntry *FrameworkDir,
                               bool IsSystem, Module *Parent);
  
  /// \brief Retrieve the module map file containing the definition of the given
  /// module.
  ///
  /// \param Module The module whose module map file will be returned, if known.
  ///
  /// \returns The file entry for the module map file containing the given
  /// module, or NULL if the module definition was inferred.
  const FileEntry *getContainingModuleMapFile(Module *Module) const;

  /// \brief Resolve all of the unresolved exports in the given module.
  ///
  /// \param Mod The module whose exports should be resolved.
  ///
  /// \param Complain Whether to emit diagnostics for failures.
  ///
  /// \returns true if any errors were encountered while resolving exports,
  /// false otherwise.
  bool resolveExports(Module *Mod, bool Complain);

  /// \brief Resolve all of the unresolved uses in the given module.
  ///
  /// \param Mod The module whose uses should be resolved.
  ///
  /// \param Complain Whether to emit diagnostics for failures.
  ///
  /// \returns true if any errors were encountered while resolving uses,
  /// false otherwise.
  bool resolveUses(Module *Mod, bool Complain);

  /// \brief Resolve all of the unresolved conflicts in the given module.
  ///
  /// \param Mod The module whose conflicts should be resolved.
  ///
  /// \param Complain Whether to emit diagnostics for failures.
  ///
  /// \returns true if any errors were encountered while resolving conflicts,
  /// false otherwise.
  bool resolveConflicts(Module *Mod, bool Complain);

  /// \brief Infers the (sub)module based on the given source location and
  /// source manager.
  ///
  /// \param Loc The location within the source that we are querying, along
  /// with its source manager.
  ///
  /// \returns The module that owns this source location, or null if no
  /// module owns this source location.
  Module *inferModuleFromLocation(FullSourceLoc Loc);
  
  /// \brief Sets the umbrella header of the given module to the given
  /// header.
  void setUmbrellaHeader(Module *Mod, const FileEntry *UmbrellaHeader);

  /// \brief Sets the umbrella directory of the given module to the given
  /// directory.
  void setUmbrellaDir(Module *Mod, const DirectoryEntry *UmbrellaDir);

  /// \brief Adds this header to the given module.
  /// \param Role The role of the header wrt the module.
  void addHeader(Module *Mod, const FileEntry *Header,
                 ModuleHeaderRole Role);

  /// \brief Parse the given module map file, and record any modules we 
  /// encounter.
  ///
  /// \param File The file to be parsed.
  ///
  /// \param IsSystem Whether this module map file is in a system header
  /// directory, and therefore should be considered a system module.
  ///
  /// \returns true if an error occurred, false otherwise.
  bool parseModuleMapFile(const FileEntry *File, bool IsSystem);
    
  /// \brief Dump the contents of the module map, for debugging purposes.
  void dump();
  
  typedef llvm::StringMap<Module *>::const_iterator module_iterator;
  module_iterator module_begin() const { return Modules.begin(); }
  module_iterator module_end()   const { return Modules.end(); }
};
  
}
#endif
