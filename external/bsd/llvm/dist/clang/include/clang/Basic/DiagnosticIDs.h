//===--- DiagnosticIDs.h - Diagnostic IDs Handling --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// \brief Defines the Diagnostic IDs-related interfaces.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_DIAGNOSTICIDS_H
#define LLVM_CLANG_DIAGNOSTICIDS_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"

namespace clang {
  class DiagnosticsEngine;
  class SourceLocation;

  // Import the diagnostic enums themselves.
  namespace diag {
    // Start position for diagnostics.
    enum {
      DIAG_START_COMMON        =                                 0,
      DIAG_START_DRIVER        = DIAG_START_COMMON          +  300,
      DIAG_START_FRONTEND      = DIAG_START_DRIVER          +  100,
      DIAG_START_SERIALIZATION = DIAG_START_FRONTEND        +  100,
      DIAG_START_LEX           = DIAG_START_SERIALIZATION   +  120,
      DIAG_START_PARSE         = DIAG_START_LEX             +  300,
      DIAG_START_AST           = DIAG_START_PARSE           +  400,
      DIAG_START_COMMENT       = DIAG_START_AST             +  100,
      DIAG_START_SEMA          = DIAG_START_COMMENT         +  100,
      DIAG_START_ANALYSIS      = DIAG_START_SEMA            + 3000,
      DIAG_UPPER_LIMIT         = DIAG_START_ANALYSIS        +  100
    };

    class CustomDiagInfo;

    /// \brief All of the diagnostics that can be emitted by the frontend.
    typedef unsigned kind;

    // Get typedefs for common diagnostics.
    enum {
#define DIAG(ENUM,FLAGS,DEFAULT_MAPPING,DESC,GROUP,\
             SFINAE,CATEGORY,NOWERROR,SHOWINSYSHEADER) ENUM,
#define COMMONSTART
#include "clang/Basic/DiagnosticCommonKinds.inc"
      NUM_BUILTIN_COMMON_DIAGNOSTICS
#undef DIAG
    };

    /// Enum values that allow the client to map NOTEs, WARNINGs, and EXTENSIONs
    /// to either MAP_IGNORE (nothing), MAP_REMARK (emit a remark), MAP_WARNING
    /// (emit a warning), MAP_ERROR (emit as an error).  It allows clients to
    /// map errors to MAP_ERROR/MAP_DEFAULT or MAP_FATAL (stop emitting
    /// diagnostics after this one).
    enum Mapping {
      // NOTE: 0 means "uncomputed".
      MAP_IGNORE  = 1,     ///< Map this diagnostic to nothing, ignore it.
      MAP_REMARK  = 2,     ///< Map this diagnostic to a remark.
      MAP_WARNING = 3,     ///< Map this diagnostic to a warning.
      MAP_ERROR   = 4,     ///< Map this diagnostic to an error.
      MAP_FATAL   = 5      ///< Map this diagnostic to a fatal error.
    };
  }

class DiagnosticMappingInfo {
  unsigned Mapping : 3;
  unsigned IsUser : 1;
  unsigned IsPragma : 1;
  unsigned HasShowInSystemHeader : 1;
  unsigned HasNoWarningAsError : 1;
  unsigned HasNoErrorAsFatal : 1;

public:
  static DiagnosticMappingInfo Make(diag::Mapping Mapping, bool IsUser,
                                    bool IsPragma) {
    DiagnosticMappingInfo Result;
    Result.Mapping = Mapping;
    Result.IsUser = IsUser;
    Result.IsPragma = IsPragma;
    Result.HasShowInSystemHeader = 0;
    Result.HasNoWarningAsError = 0;
    Result.HasNoErrorAsFatal = 0;
    return Result;
  }

  diag::Mapping getMapping() const { return diag::Mapping(Mapping); }
  void setMapping(diag::Mapping Value) { Mapping = Value; }

  bool isUser() const { return IsUser; }
  bool isPragma() const { return IsPragma; }

  bool hasShowInSystemHeader() const { return HasShowInSystemHeader; }
  void setShowInSystemHeader(bool Value) { HasShowInSystemHeader = Value; }

  bool hasNoWarningAsError() const { return HasNoWarningAsError; }
  void setNoWarningAsError(bool Value) { HasNoWarningAsError = Value; }

  bool hasNoErrorAsFatal() const { return HasNoErrorAsFatal; }
  void setNoErrorAsFatal(bool Value) { HasNoErrorAsFatal = Value; }
};

/// \brief Used for handling and querying diagnostic IDs.
///
/// Can be used and shared by multiple Diagnostics for multiple translation units.
class DiagnosticIDs : public RefCountedBase<DiagnosticIDs> {
public:
  /// \brief The level of the diagnostic, after it has been through mapping.
  enum Level {
    Ignored, Note, Remark, Warning, Error, Fatal
  };

private:
  /// \brief Information for uniquing and looking up custom diags.
  diag::CustomDiagInfo *CustomDiagInfo;

public:
  DiagnosticIDs();
  ~DiagnosticIDs();

  /// \brief Return an ID for a diagnostic with the specified format string and
  /// level.
  ///
  /// If this is the first request for this diagnostic, it is registered and
  /// created, otherwise the existing ID is returned.

  // FIXME: Replace this function with a create-only facilty like
  // createCustomDiagIDFromFormatString() to enforce safe usage. At the time of
  // writing, nearly all callers of this function were invalid.
  unsigned getCustomDiagID(Level L, StringRef FormatString);

  //===--------------------------------------------------------------------===//
  // Diagnostic classification and reporting interfaces.
  //

  /// \brief Given a diagnostic ID, return a description of the issue.
  StringRef getDescription(unsigned DiagID) const;

  /// \brief Return true if the unmapped diagnostic levelof the specified
  /// diagnostic ID is a Warning or Extension.
  ///
  /// This only works on builtin diagnostics, not custom ones, and is not
  /// legal to call on NOTEs.
  static bool isBuiltinWarningOrExtension(unsigned DiagID);

  /// \brief Return true if the specified diagnostic is mapped to errors by
  /// default.
  static bool isDefaultMappingAsError(unsigned DiagID);

  /// \brief Return true if the specified diagnostic is a Remark.
  static bool isRemark(unsigned DiagID);

  /// \brief Determine whether the given built-in diagnostic ID is a Note.
  static bool isBuiltinNote(unsigned DiagID);

  /// \brief Determine whether the given built-in diagnostic ID is for an
  /// extension of some sort.
  static bool isBuiltinExtensionDiag(unsigned DiagID) {
    bool ignored;
    return isBuiltinExtensionDiag(DiagID, ignored);
  }
  
  /// \brief Determine whether the given built-in diagnostic ID is for an
  /// extension of some sort, and whether it is enabled by default.
  ///
  /// This also returns EnabledByDefault, which is set to indicate whether the
  /// diagnostic is ignored by default (in which case -pedantic enables it) or
  /// treated as a warning/error by default.
  ///
  static bool isBuiltinExtensionDiag(unsigned DiagID, bool &EnabledByDefault);
  

  /// \brief Return the lowest-level warning option that enables the specified
  /// diagnostic.
  ///
  /// If there is no -Wfoo flag that controls the diagnostic, this returns null.
  static StringRef getWarningOptionForDiag(unsigned DiagID);
  
  /// \brief Return the category number that a specified \p DiagID belongs to,
  /// or 0 if no category.
  static unsigned getCategoryNumberForDiag(unsigned DiagID);

  /// \brief Return the number of diagnostic categories.
  static unsigned getNumberOfCategories();

  /// \brief Given a category ID, return the name of the category.
  static StringRef getCategoryNameFromID(unsigned CategoryID);
  
  /// \brief Return true if a given diagnostic falls into an ARC diagnostic
  /// category.
  static bool isARCDiagnostic(unsigned DiagID);

  /// \brief Enumeration describing how the emission of a diagnostic should
  /// be treated when it occurs during C++ template argument deduction.
  enum SFINAEResponse {
    /// \brief The diagnostic should not be reported, but it should cause
    /// template argument deduction to fail.
    ///
    /// The vast majority of errors that occur during template argument 
    /// deduction fall into this category.
    SFINAE_SubstitutionFailure,
    
    /// \brief The diagnostic should be suppressed entirely.
    ///
    /// Warnings generally fall into this category.
    SFINAE_Suppress,
    
    /// \brief The diagnostic should be reported.
    ///
    /// The diagnostic should be reported. Various fatal errors (e.g., 
    /// template instantiation depth exceeded) fall into this category.
    SFINAE_Report,
    
    /// \brief The diagnostic is an access-control diagnostic, which will be
    /// substitution failures in some contexts and reported in others.
    SFINAE_AccessControl
  };
  
  /// \brief Determines whether the given built-in diagnostic ID is
  /// for an error that is suppressed if it occurs during C++ template
  /// argument deduction.
  ///
  /// When an error is suppressed due to SFINAE, the template argument
  /// deduction fails but no diagnostic is emitted. Certain classes of
  /// errors, such as those errors that involve C++ access control,
  /// are not SFINAE errors.
  static SFINAEResponse getDiagnosticSFINAEResponse(unsigned DiagID);

  /// \brief Get the set of all diagnostic IDs in the group with the given name.
  ///
  /// \param[out] Diags - On return, the diagnostics in the group.
  /// \returns \c true if the given group is unknown, \c false otherwise.
  bool getDiagnosticsInGroup(StringRef Group,
                             SmallVectorImpl<diag::kind> &Diags) const;

  /// \brief Get the set of all diagnostic IDs.
  void getAllDiagnostics(SmallVectorImpl<diag::kind> &Diags) const;

  /// \brief Get the warning option with the closest edit distance to the given
  /// group name.
  static StringRef getNearestWarningOption(StringRef Group);

private:
  /// \brief Classify the specified diagnostic ID into a Level, consumable by
  /// the DiagnosticClient.
  /// 
  /// The classification is based on the way the client configured the
  /// DiagnosticsEngine object.
  ///
  /// \param Loc The source location for which we are interested in finding out
  /// the diagnostic state. Can be null in order to query the latest state.
  DiagnosticIDs::Level
  getDiagnosticLevel(unsigned DiagID, SourceLocation Loc,
                     const DiagnosticsEngine &Diag) const LLVM_READONLY;

  /// \brief An internal implementation helper used when \p DiagClass is
  /// already known.
  DiagnosticIDs::Level
  getDiagnosticLevel(unsigned DiagID, unsigned DiagClass, SourceLocation Loc,
                     const DiagnosticsEngine &Diag) const LLVM_READONLY;

  /// \brief Used to report a diagnostic that is finally fully formed.
  ///
  /// \returns \c true if the diagnostic was emitted, \c false if it was
  /// suppressed.
  bool ProcessDiag(DiagnosticsEngine &Diag) const;

  /// \brief Used to emit a diagnostic that is finally fully formed,
  /// ignoring suppression.
  void EmitDiag(DiagnosticsEngine &Diag, Level DiagLevel) const;

  /// \brief Whether the diagnostic may leave the AST in a state where some
  /// invariants can break.
  bool isUnrecoverable(unsigned DiagID) const;

  friend class DiagnosticsEngine;
};

}  // end namespace clang

#endif
