//===--- Utils.h - Misc utilities for the front-end -------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This header contains miscellaneous utilities for various front-end actions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_FRONTEND_UTILS_H
#define LLVM_CLANG_FRONTEND_UTILS_H

#include "clang/Basic/Diagnostic.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Option/OptSpecifier.h"

namespace llvm {
class raw_fd_ostream;
class Triple;

namespace opt {
class ArgList;
}
}

namespace clang {
class ASTConsumer;
class CompilerInstance;
class CompilerInvocation;
class Decl;
class DependencyOutputOptions;
class DiagnosticsEngine;
class DiagnosticOptions;
class FileManager;
class HeaderSearch;
class HeaderSearchOptions;
class IdentifierTable;
class LangOptions;
class Preprocessor;
class PreprocessorOptions;
class PreprocessorOutputOptions;
class SourceManager;
class Stmt;
class TargetInfo;
class FrontendOptions;

/// Apply the header search options to get given HeaderSearch object.
void ApplyHeaderSearchOptions(HeaderSearch &HS,
                              const HeaderSearchOptions &HSOpts,
                              const LangOptions &Lang,
                              const llvm::Triple &triple);

/// InitializePreprocessor - Initialize the preprocessor getting it and the
/// environment ready to process a single file.
void InitializePreprocessor(Preprocessor &PP,
                            const PreprocessorOptions &PPOpts,
                            const HeaderSearchOptions &HSOpts,
                            const FrontendOptions &FEOpts);

/// ProcessWarningOptions - Initialize the diagnostic client and process the
/// warning options specified on the command line.
void ProcessWarningOptions(DiagnosticsEngine &Diags,
                           const DiagnosticOptions &Opts,
                           bool ReportDiags = true);

/// DoPrintPreprocessedInput - Implement -E mode.
void DoPrintPreprocessedInput(Preprocessor &PP, raw_ostream* OS,
                              const PreprocessorOutputOptions &Opts);

/// AttachDependencyFileGen - Create a dependency file generator, and attach
/// it to the given preprocessor.  This takes ownership of the output stream.
void AttachDependencyFileGen(Preprocessor &PP,
                             const DependencyOutputOptions &Opts);

/// AttachDependencyGraphGen - Create a dependency graph generator, and attach
/// it to the given preprocessor.
  void AttachDependencyGraphGen(Preprocessor &PP, StringRef OutputFile,
                                StringRef SysRoot);

/// AttachHeaderIncludeGen - Create a header include list generator, and attach
/// it to the given preprocessor.
///
/// \param ShowAllHeaders - If true, show all header information instead of just
/// headers following the predefines buffer. This is useful for making sure
/// includes mentioned on the command line are also reported, but differs from
/// the default behavior used by -H.
/// \param OutputPath - If non-empty, a path to write the header include
/// information to, instead of writing to stderr.
/// \param ShowDepth - Whether to indent to show the nesting of the includes.
/// \param MSStyle - Whether to print in cl.exe /showIncludes style.
void AttachHeaderIncludeGen(Preprocessor &PP, bool ShowAllHeaders = false,
                            StringRef OutputPath = "",
                            bool ShowDepth = true, bool MSStyle = false);

/// CacheTokens - Cache tokens for use with PCH. Note that this requires
/// a seekable stream.
void CacheTokens(Preprocessor &PP, llvm::raw_fd_ostream* OS);

/// createInvocationFromCommandLine - Construct a compiler invocation object for
/// a command line argument vector.
///
/// \return A CompilerInvocation, or 0 if none was built for the given
/// argument vector.
CompilerInvocation *
createInvocationFromCommandLine(ArrayRef<const char *> Args,
                            IntrusiveRefCntPtr<DiagnosticsEngine> Diags =
                                IntrusiveRefCntPtr<DiagnosticsEngine>());

/// Return the value of the last argument as an integer, or a default. If Diags
/// is non-null, emits an error if the argument is given, but non-integral.
int getLastArgIntValue(const llvm::opt::ArgList &Args,
                       llvm::opt::OptSpecifier Id, int Default,
                       DiagnosticsEngine *Diags = 0);

inline int getLastArgIntValue(const llvm::opt::ArgList &Args,
                              llvm::opt::OptSpecifier Id, int Default,
                              DiagnosticsEngine &Diags) {
  return getLastArgIntValue(Args, Id, Default, &Diags);
}

// When Clang->getFrontendOpts().DisableFree is set we don't delete some of the
// global objects, but we don't want LeakDetectors to complain, so we bury them
// in a globally visible array.
void BuryPointer(const void *Ptr);

} // end namespace clang

#endif
