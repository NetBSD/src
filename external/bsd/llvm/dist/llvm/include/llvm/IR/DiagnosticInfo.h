//===- llvm/IR/DiagnosticInfo.h - Diagnostic Declaration --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the different classes involved in low level diagnostics.
//
// Diagnostics reporting is still done as part of the LLVMContext.
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_DIAGNOSTICINFO_H
#define LLVM_IR_DIAGNOSTICINFO_H

#include "llvm/ADT/None.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/CBindingWrapping.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm-c/Types.h"
#include <functional>
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>

namespace llvm {

// Forward declarations.
class DiagnosticPrinter;
class Function;
class Instruction;
class LLVMContext;
class Module;
class SMDiagnostic;

/// \brief Defines the different supported severity of a diagnostic.
enum DiagnosticSeverity : char {
  DS_Error,
  DS_Warning,
  DS_Remark,
  // A note attaches additional information to one of the previous diagnostic
  // types.
  DS_Note
};

/// \brief Defines the different supported kind of a diagnostic.
/// This enum should be extended with a new ID for each added concrete subclass.
enum DiagnosticKind {
  DK_InlineAsm,
  DK_ResourceLimit,
  DK_StackSize,
  DK_Linker,
  DK_DebugMetadataVersion,
  DK_DebugMetadataInvalid,
  DK_ISelFallback,
  DK_SampleProfile,
  DK_OptimizationRemark,
  DK_OptimizationRemarkMissed,
  DK_OptimizationRemarkAnalysis,
  DK_OptimizationRemarkAnalysisFPCommute,
  DK_OptimizationRemarkAnalysisAliasing,
  DK_OptimizationFailure,
  DK_FirstRemark = DK_OptimizationRemark,
  DK_LastRemark = DK_OptimizationFailure,
  DK_MIRParser,
  DK_PGOProfile,
  DK_Unsupported,
  DK_FirstPluginKind
};

/// \brief Get the next available kind ID for a plugin diagnostic.
/// Each time this function is called, it returns a different number.
/// Therefore, a plugin that wants to "identify" its own classes
/// with a dynamic identifier, just have to use this method to get a new ID
/// and assign it to each of its classes.
/// The returned ID will be greater than or equal to DK_FirstPluginKind.
/// Thus, the plugin identifiers will not conflict with the
/// DiagnosticKind values.
int getNextAvailablePluginDiagnosticKind();

/// \brief This is the base abstract class for diagnostic reporting in
/// the backend.
/// The print method must be overloaded by the subclasses to print a
/// user-friendly message in the client of the backend (let us call it a
/// frontend).
class DiagnosticInfo {
private:
  /// Kind defines the kind of report this is about.
  const /* DiagnosticKind */ int Kind;
  /// Severity gives the severity of the diagnostic.
  const DiagnosticSeverity Severity;

public:
  DiagnosticInfo(/* DiagnosticKind */ int Kind, DiagnosticSeverity Severity)
      : Kind(Kind), Severity(Severity) {}

  virtual ~DiagnosticInfo() = default;

  /* DiagnosticKind */ int getKind() const { return Kind; }
  DiagnosticSeverity getSeverity() const { return Severity; }

  /// Print using the given \p DP a user-friendly message.
  /// This is the default message that will be printed to the user.
  /// It is used when the frontend does not directly take advantage
  /// of the information contained in fields of the subclasses.
  /// The printed message must not end with '.' nor start with a severity
  /// keyword.
  virtual void print(DiagnosticPrinter &DP) const = 0;
};

typedef std::function<void(const DiagnosticInfo &)> DiagnosticHandlerFunction;

/// Diagnostic information for inline asm reporting.
/// This is basically a message and an optional location.
class DiagnosticInfoInlineAsm : public DiagnosticInfo {
private:
  /// Optional line information. 0 if not set.
  unsigned LocCookie;
  /// Message to be reported.
  const Twine &MsgStr;
  /// Optional origin of the problem.
  const Instruction *Instr;

public:
  /// \p MsgStr is the message to be reported to the frontend.
  /// This class does not copy \p MsgStr, therefore the reference must be valid
  /// for the whole life time of the Diagnostic.
  DiagnosticInfoInlineAsm(const Twine &MsgStr,
                          DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_InlineAsm, Severity), LocCookie(0), MsgStr(MsgStr),
        Instr(nullptr) {}

  /// \p LocCookie if non-zero gives the line number for this report.
  /// \p MsgStr gives the message.
  /// This class does not copy \p MsgStr, therefore the reference must be valid
  /// for the whole life time of the Diagnostic.
  DiagnosticInfoInlineAsm(unsigned LocCookie, const Twine &MsgStr,
                          DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_InlineAsm, Severity), LocCookie(LocCookie),
        MsgStr(MsgStr), Instr(nullptr) {}

  /// \p Instr gives the original instruction that triggered the diagnostic.
  /// \p MsgStr gives the message.
  /// This class does not copy \p MsgStr, therefore the reference must be valid
  /// for the whole life time of the Diagnostic.
  /// Same for \p I.
  DiagnosticInfoInlineAsm(const Instruction &I, const Twine &MsgStr,
                          DiagnosticSeverity Severity = DS_Error);

  unsigned getLocCookie() const { return LocCookie; }
  const Twine &getMsgStr() const { return MsgStr; }
  const Instruction *getInstruction() const { return Instr; }

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_InlineAsm;
  }
};

/// Diagnostic information for stack size etc. reporting.
/// This is basically a function and a size.
class DiagnosticInfoResourceLimit : public DiagnosticInfo {
private:
  /// The function that is concerned by this resource limit diagnostic.
  const Function &Fn;

  /// Description of the resource type (e.g. stack size)
  const char *ResourceName;

  /// The computed size usage
  uint64_t ResourceSize;

  // Threshould passed
  uint64_t ResourceLimit;

public:
  /// \p The function that is concerned by this stack size diagnostic.
  /// \p The computed stack size.
  DiagnosticInfoResourceLimit(const Function &Fn,
                              const char *ResourceName,
                              uint64_t ResourceSize,
                              DiagnosticSeverity Severity = DS_Warning,
                              DiagnosticKind Kind = DK_ResourceLimit,
                              uint64_t ResourceLimit = 0)
      : DiagnosticInfo(Kind, Severity),
        Fn(Fn),
        ResourceName(ResourceName),
        ResourceSize(ResourceSize),
        ResourceLimit(ResourceLimit) {}

  const Function &getFunction() const { return Fn; }
  const char *getResourceName() const { return ResourceName; }
  uint64_t getResourceSize() const { return ResourceSize; }
  uint64_t getResourceLimit() const { return ResourceLimit; }

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_ResourceLimit ||
           DI->getKind() == DK_StackSize;
  }
};

class DiagnosticInfoStackSize : public DiagnosticInfoResourceLimit {
public:
  DiagnosticInfoStackSize(const Function &Fn,
                          uint64_t StackSize,
                          DiagnosticSeverity Severity = DS_Warning,
                          uint64_t StackLimit = 0)
    : DiagnosticInfoResourceLimit(Fn, "stack size", StackSize,
                                  Severity, DK_StackSize, StackLimit) {}

  uint64_t getStackSize() const { return getResourceSize(); }
  uint64_t getStackLimit() const { return getResourceLimit(); }

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_StackSize;
  }
};

/// Diagnostic information for debug metadata version reporting.
/// This is basically a module and a version.
class DiagnosticInfoDebugMetadataVersion : public DiagnosticInfo {
private:
  /// The module that is concerned by this debug metadata version diagnostic.
  const Module &M;
  /// The actual metadata version.
  unsigned MetadataVersion;

public:
  /// \p The module that is concerned by this debug metadata version diagnostic.
  /// \p The actual metadata version.
  DiagnosticInfoDebugMetadataVersion(const Module &M, unsigned MetadataVersion,
                          DiagnosticSeverity Severity = DS_Warning)
      : DiagnosticInfo(DK_DebugMetadataVersion, Severity), M(M),
        MetadataVersion(MetadataVersion) {}

  const Module &getModule() const { return M; }
  unsigned getMetadataVersion() const { return MetadataVersion; }

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_DebugMetadataVersion;
  }
};

/// Diagnostic information for stripping invalid debug metadata.
class DiagnosticInfoIgnoringInvalidDebugMetadata : public DiagnosticInfo {
private:
  /// The module that is concerned by this debug metadata version diagnostic.
  const Module &M;

public:
  /// \p The module that is concerned by this debug metadata version diagnostic.
  DiagnosticInfoIgnoringInvalidDebugMetadata(
      const Module &M, DiagnosticSeverity Severity = DS_Warning)
      : DiagnosticInfo(DK_DebugMetadataVersion, Severity), M(M) {}

  const Module &getModule() const { return M; }

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_DebugMetadataInvalid;
  }
};

/// Diagnostic information for the sample profiler.
class DiagnosticInfoSampleProfile : public DiagnosticInfo {
public:
  DiagnosticInfoSampleProfile(StringRef FileName, unsigned LineNum,
                              const Twine &Msg,
                              DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_SampleProfile, Severity), FileName(FileName),
        LineNum(LineNum), Msg(Msg) {}
  DiagnosticInfoSampleProfile(StringRef FileName, const Twine &Msg,
                              DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_SampleProfile, Severity), FileName(FileName),
        LineNum(0), Msg(Msg) {}
  DiagnosticInfoSampleProfile(const Twine &Msg,
                              DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_SampleProfile, Severity), LineNum(0), Msg(Msg) {}

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_SampleProfile;
  }

  StringRef getFileName() const { return FileName; }
  unsigned getLineNum() const { return LineNum; }
  const Twine &getMsg() const { return Msg; }

private:
  /// Name of the input file associated with this diagnostic.
  StringRef FileName;

  /// Line number where the diagnostic occurred. If 0, no line number will
  /// be emitted in the message.
  unsigned LineNum;

  /// Message to report.
  const Twine &Msg;
};

/// Diagnostic information for the PGO profiler.
class DiagnosticInfoPGOProfile : public DiagnosticInfo {
public:
  DiagnosticInfoPGOProfile(const char *FileName, const Twine &Msg,
                           DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(DK_PGOProfile, Severity), FileName(FileName), Msg(Msg) {}

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_PGOProfile;
  }

  const char *getFileName() const { return FileName; }
  const Twine &getMsg() const { return Msg; }

private:
  /// Name of the input file associated with this diagnostic.
  const char *FileName;

  /// Message to report.
  const Twine &Msg;
};

/// Common features for diagnostics with an associated DebugLoc
class DiagnosticInfoWithDebugLocBase : public DiagnosticInfo {
public:
  /// \p Fn is the function where the diagnostic is being emitted. \p DLoc is
  /// the location information to use in the diagnostic.
  DiagnosticInfoWithDebugLocBase(enum DiagnosticKind Kind,
                                 enum DiagnosticSeverity Severity,
                                 const Function &Fn,
                                 const DebugLoc &DLoc)
      : DiagnosticInfo(Kind, Severity), Fn(Fn), DLoc(DLoc) {}

  /// Return true if location information is available for this diagnostic.
  bool isLocationAvailable() const;

  /// Return a string with the location information for this diagnostic
  /// in the format "file:line:col". If location information is not available,
  /// it returns "<unknown>:0:0".
  const std::string getLocationStr() const;

  /// Return location information for this diagnostic in three parts:
  /// the source file name, line number and column.
  void getLocation(StringRef *Filename, unsigned *Line, unsigned *Column) const;

  const Function &getFunction() const { return Fn; }
  const DebugLoc &getDebugLoc() const { return DLoc; }

private:
  /// Function where this diagnostic is triggered.
  const Function &Fn;

  /// Debug location where this diagnostic is triggered.
  DebugLoc DLoc;
};

/// Common features for diagnostics dealing with optimization remarks.
class DiagnosticInfoOptimizationBase : public DiagnosticInfoWithDebugLocBase {
public:
  /// \brief Used to set IsVerbose via the stream interface.
  struct setIsVerbose {};

  /// \brief When an instance of this is inserted into the stream, the arguments
  /// following will not appear in the remark printed in the compiler output
  /// (-Rpass) but only in the optimization record file
  /// (-fsave-optimization-record).
  struct setExtraArgs {};

  /// \brief Used in the streaming interface as the general argument type.  It
  /// internally converts everything into a key-value pair.
  struct Argument {
    StringRef Key;
    std::string Val;
    // If set, the debug location corresponding to the value.
    DebugLoc DLoc;

    explicit Argument(StringRef Str = "") : Key("String"), Val(Str) {}
    Argument(StringRef Key, Value *V);
    Argument(StringRef Key, Type *T);
    Argument(StringRef Key, int N);
    Argument(StringRef Key, unsigned N);
    Argument(StringRef Key, bool B) : Key(Key), Val(B ? "true" : "false") {}
  };

  /// \p PassName is the name of the pass emitting this diagnostic. \p
  /// RemarkName is a textual identifier for the remark.  \p Fn is the function
  /// where the diagnostic is being emitted. \p DLoc is the location information
  /// to use in the diagnostic. If line table information is available, the
  /// diagnostic will include the source code location. \p CodeRegion is IR
  /// value (currently basic block) that the optimization operates on.  This is
  /// currently used to provide run-time hotness information with PGO.
  DiagnosticInfoOptimizationBase(enum DiagnosticKind Kind,
                                 enum DiagnosticSeverity Severity,
                                 const char *PassName, StringRef RemarkName,
                                 const Function &Fn, const DebugLoc &DLoc,
                                 Value *CodeRegion = nullptr)
      : DiagnosticInfoWithDebugLocBase(Kind, Severity, Fn, DLoc),
        PassName(PassName), RemarkName(RemarkName), CodeRegion(CodeRegion) {}

  /// \brief This is ctor variant allows a pass to build an optimization remark
  /// from an existing remark.
  ///
  /// This is useful when a transformation pass (e.g LV) wants to emit a remark
  /// (\p Orig) generated by one of its analyses (e.g. LAA) as its own analysis
  /// remark.  The string \p Prepend will be emitted before the original
  /// message.
  DiagnosticInfoOptimizationBase(const char *PassName, StringRef Prepend,
                                 const DiagnosticInfoOptimizationBase &Orig)
      : DiagnosticInfoWithDebugLocBase((DiagnosticKind)Orig.getKind(),
                                       Orig.getSeverity(), Orig.getFunction(),
                                       Orig.getDebugLoc()),
        PassName(PassName), RemarkName(Orig.RemarkName),
        CodeRegion(Orig.getCodeRegion()) {
    *this << Prepend;
    std::copy(Orig.Args.begin(), Orig.Args.end(), std::back_inserter(Args));
  }

  /// Legacy interface.
  /// \p PassName is the name of the pass emitting this diagnostic.
  /// \p Fn is the function where the diagnostic is being emitted. \p DLoc is
  /// the location information to use in the diagnostic. If line table
  /// information is available, the diagnostic will include the source code
  /// location. \p Msg is the message to show. Note that this class does not
  /// copy this message, so this reference must be valid for the whole life time
  /// of the diagnostic.
  DiagnosticInfoOptimizationBase(enum DiagnosticKind Kind,
                                 enum DiagnosticSeverity Severity,
                                 const char *PassName, const Function &Fn,
                                 const DebugLoc &DLoc, const Twine &Msg,
                                 Optional<uint64_t> Hotness = None)
      : DiagnosticInfoWithDebugLocBase(Kind, Severity, Fn, DLoc),
        PassName(PassName), Hotness(Hotness) {
    Args.push_back(Argument(Msg.str()));
  }

  DiagnosticInfoOptimizationBase &operator<<(StringRef S);
  DiagnosticInfoOptimizationBase &operator<<(Argument A);
  DiagnosticInfoOptimizationBase &operator<<(setIsVerbose V);
  DiagnosticInfoOptimizationBase &operator<<(setExtraArgs EA);

  /// \see DiagnosticInfo::print.
  void print(DiagnosticPrinter &DP) const override;

  /// Return true if this optimization remark is enabled by one of
  /// of the LLVM command line flags (-pass-remarks, -pass-remarks-missed,
  /// or -pass-remarks-analysis). Note that this only handles the LLVM
  /// flags. We cannot access Clang flags from here (they are handled
  /// in BackendConsumer::OptimizationRemarkHandler).
  virtual bool isEnabled() const = 0;

  StringRef getPassName() const { return PassName; }
  std::string getMsg() const;
  Optional<uint64_t> getHotness() const { return Hotness; }
  void setHotness(Optional<uint64_t> H) { Hotness = H; }

  Value *getCodeRegion() const { return CodeRegion; }

  bool isVerbose() const { return IsVerbose; }

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() >= DK_FirstRemark &&
           DI->getKind() <= DK_LastRemark;
  }

private:
  /// Name of the pass that triggers this report. If this matches the
  /// regular expression given in -Rpass=regexp, then the remark will
  /// be emitted.
  const char *PassName;

  /// Textual identifier for the remark.  Can be used by external tools reading
  /// the YAML output file for optimization remarks to identify the remark.
  StringRef RemarkName;

  /// If profile information is available, this is the number of times the
  /// corresponding code was executed in a profile instrumentation run.
  Optional<uint64_t> Hotness;

  /// The IR value (currently basic block) that the optimization operates on.
  /// This is currently used to provide run-time hotness information with PGO.
  Value *CodeRegion;

  /// Arguments collected via the streaming interface.
  SmallVector<Argument, 4> Args;

  /// The remark is expected to be noisy.
  bool IsVerbose = false;

  /// \brief If positive, the index of the first argument that only appear in
  /// the optimization records and not in the remark printed in the compiler
  /// output.
  int FirstExtraArgIndex = -1;

  friend struct yaml::MappingTraits<DiagnosticInfoOptimizationBase *>;
};

/// Diagnostic information for applied optimization remarks.
class OptimizationRemark : public DiagnosticInfoOptimizationBase {
public:
  /// \p PassName is the name of the pass emitting this diagnostic. If
  /// this name matches the regular expression given in -Rpass=, then the
  /// diagnostic will be emitted. \p Fn is the function where the diagnostic
  /// is being emitted. \p DLoc is the location information to use in the
  /// diagnostic. If line table information is available, the diagnostic
  /// will include the source code location. \p Msg is the message to show.
  /// Note that this class does not copy this message, so this reference
  /// must be valid for the whole life time of the diagnostic.
  OptimizationRemark(const char *PassName, const Function &Fn,
                     const DebugLoc &DLoc, const Twine &Msg,
                     Optional<uint64_t> Hotness = None)
      : DiagnosticInfoOptimizationBase(DK_OptimizationRemark, DS_Remark,
                                       PassName, Fn, DLoc, Msg, Hotness) {}

  /// \p PassName is the name of the pass emitting this diagnostic. If this name
  /// matches the regular expression given in -Rpass=, then the diagnostic will
  /// be emitted.  \p RemarkName is a textual identifier for the remark.  \p
  /// DLoc is the debug location and \p CodeRegion is the region that the
  /// optimization operates on (currently on block is supported).
  OptimizationRemark(const char *PassName, StringRef RemarkName,
                     const DebugLoc &DLoc, Value *CodeRegion);

  /// Same as above but the debug location and code region is derived from \p
  /// Instr.
  OptimizationRemark(const char *PassName, StringRef RemarkName,
                     Instruction *Inst);

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationRemark;
  }

  /// \see DiagnosticInfoOptimizationBase::isEnabled.
  bool isEnabled() const override;
};

/// Diagnostic information for missed-optimization remarks.
class OptimizationRemarkMissed : public DiagnosticInfoOptimizationBase {
public:
  /// \p PassName is the name of the pass emitting this diagnostic. If
  /// this name matches the regular expression given in -Rpass-missed=, then the
  /// diagnostic will be emitted. \p Fn is the function where the diagnostic
  /// is being emitted. \p DLoc is the location information to use in the
  /// diagnostic. If line table information is available, the diagnostic
  /// will include the source code location. \p Msg is the message to show.
  /// Note that this class does not copy this message, so this reference
  /// must be valid for the whole life time of the diagnostic.
  OptimizationRemarkMissed(const char *PassName, const Function &Fn,
                           const DebugLoc &DLoc, const Twine &Msg,
                           Optional<uint64_t> Hotness = None)
      : DiagnosticInfoOptimizationBase(DK_OptimizationRemarkMissed, DS_Remark,
                                       PassName, Fn, DLoc, Msg, Hotness) {}

  /// \p PassName is the name of the pass emitting this diagnostic. If this name
  /// matches the regular expression given in -Rpass-missed=, then the
  /// diagnostic will be emitted.  \p RemarkName is a textual identifier for the
  /// remark.  \p DLoc is the debug location and \p CodeRegion is the region
  /// that the optimization operates on (currently on block is supported).
  OptimizationRemarkMissed(const char *PassName, StringRef RemarkName,
                           const DebugLoc &DLoc, Value *CodeRegion);

  /// \brief Same as above but \p Inst is used to derive code region and debug
  /// location.
  OptimizationRemarkMissed(const char *PassName, StringRef RemarkName,
                           Instruction *Inst);

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationRemarkMissed;
  }

  /// \see DiagnosticInfoOptimizationBase::isEnabled.
  bool isEnabled() const override;
};

/// Diagnostic information for optimization analysis remarks.
class OptimizationRemarkAnalysis : public DiagnosticInfoOptimizationBase {
public:
  /// \p PassName is the name of the pass emitting this diagnostic. If
  /// this name matches the regular expression given in -Rpass-analysis=, then
  /// the diagnostic will be emitted. \p Fn is the function where the diagnostic
  /// is being emitted. \p DLoc is the location information to use in the
  /// diagnostic. If line table information is available, the diagnostic will
  /// include the source code location. \p Msg is the message to show. Note that
  /// this class does not copy this message, so this reference must be valid for
  /// the whole life time of the diagnostic.
  OptimizationRemarkAnalysis(const char *PassName, const Function &Fn,
                             const DebugLoc &DLoc, const Twine &Msg,
                             Optional<uint64_t> Hotness = None)
      : DiagnosticInfoOptimizationBase(DK_OptimizationRemarkAnalysis, DS_Remark,
                                       PassName, Fn, DLoc, Msg, Hotness) {}

  /// \p PassName is the name of the pass emitting this diagnostic. If this name
  /// matches the regular expression given in -Rpass-analysis=, then the
  /// diagnostic will be emitted.  \p RemarkName is a textual identifier for the
  /// remark.  \p DLoc is the debug location and \p CodeRegion is the region
  /// that the optimization operates on (currently on block is supported).
  OptimizationRemarkAnalysis(const char *PassName, StringRef RemarkName,
                             const DebugLoc &DLoc, Value *CodeRegion);

  /// \brief This is ctor variant allows a pass to build an optimization remark
  /// from an existing remark.
  ///
  /// This is useful when a transformation pass (e.g LV) wants to emit a remark
  /// (\p Orig) generated by one of its analyses (e.g. LAA) as its own analysis
  /// remark.  The string \p Prepend will be emitted before the original
  /// message.
  OptimizationRemarkAnalysis(const char *PassName, StringRef Prepend,
                             const OptimizationRemarkAnalysis &Orig)
      : DiagnosticInfoOptimizationBase(PassName, Prepend, Orig) {}

  /// \brief Same as above but \p Inst is used to derive code region and debug
  /// location.
  OptimizationRemarkAnalysis(const char *PassName, StringRef RemarkName,
                             Instruction *Inst);

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationRemarkAnalysis;
  }

  /// \see DiagnosticInfoOptimizationBase::isEnabled.
  bool isEnabled() const override;

  static const char *AlwaysPrint;

  bool shouldAlwaysPrint() const { return getPassName() == AlwaysPrint; }

protected:
  OptimizationRemarkAnalysis(enum DiagnosticKind Kind, const char *PassName,
                             const Function &Fn, const DebugLoc &DLoc,
                             const Twine &Msg, Optional<uint64_t> Hotness)
      : DiagnosticInfoOptimizationBase(Kind, DS_Remark, PassName, Fn, DLoc, Msg,
                                       Hotness) {}

  OptimizationRemarkAnalysis(enum DiagnosticKind Kind, const char *PassName,
                             StringRef RemarkName, const DebugLoc &DLoc,
                             Value *CodeRegion);
};

/// Diagnostic information for optimization analysis remarks related to
/// floating-point non-commutativity.
class OptimizationRemarkAnalysisFPCommute : public OptimizationRemarkAnalysis {
public:
  /// \p PassName is the name of the pass emitting this diagnostic. If
  /// this name matches the regular expression given in -Rpass-analysis=, then
  /// the diagnostic will be emitted. \p Fn is the function where the diagnostic
  /// is being emitted. \p DLoc is the location information to use in the
  /// diagnostic. If line table information is available, the diagnostic will
  /// include the source code location. \p Msg is the message to show. The
  /// front-end will append its own message related to options that address
  /// floating-point non-commutativity. Note that this class does not copy this
  /// message, so this reference must be valid for the whole life time of the
  /// diagnostic.
  OptimizationRemarkAnalysisFPCommute(const char *PassName, const Function &Fn,
                                      const DebugLoc &DLoc, const Twine &Msg,
                                      Optional<uint64_t> Hotness = None)
      : OptimizationRemarkAnalysis(DK_OptimizationRemarkAnalysisFPCommute,
                                   PassName, Fn, DLoc, Msg, Hotness) {}

  /// \p PassName is the name of the pass emitting this diagnostic. If this name
  /// matches the regular expression given in -Rpass-analysis=, then the
  /// diagnostic will be emitted.  \p RemarkName is a textual identifier for the
  /// remark.  \p DLoc is the debug location and \p CodeRegion is the region
  /// that the optimization operates on (currently on block is supported). The
  /// front-end will append its own message related to options that address
  /// floating-point non-commutativity.
  OptimizationRemarkAnalysisFPCommute(const char *PassName,
                                      StringRef RemarkName,
                                      const DebugLoc &DLoc, Value *CodeRegion)
      : OptimizationRemarkAnalysis(DK_OptimizationRemarkAnalysisFPCommute,
                                   PassName, RemarkName, DLoc, CodeRegion) {}

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationRemarkAnalysisFPCommute;
  }
};

/// Diagnostic information for optimization analysis remarks related to
/// pointer aliasing.
class OptimizationRemarkAnalysisAliasing : public OptimizationRemarkAnalysis {
public:
  /// \p PassName is the name of the pass emitting this diagnostic. If
  /// this name matches the regular expression given in -Rpass-analysis=, then
  /// the diagnostic will be emitted. \p Fn is the function where the diagnostic
  /// is being emitted. \p DLoc is the location information to use in the
  /// diagnostic. If line table information is available, the diagnostic will
  /// include the source code location. \p Msg is the message to show. The
  /// front-end will append its own message related to options that address
  /// pointer aliasing legality. Note that this class does not copy this
  /// message, so this reference must be valid for the whole life time of the
  /// diagnostic.
  OptimizationRemarkAnalysisAliasing(const char *PassName, const Function &Fn,
                                     const DebugLoc &DLoc, const Twine &Msg,
                                     Optional<uint64_t> Hotness = None)
      : OptimizationRemarkAnalysis(DK_OptimizationRemarkAnalysisAliasing,
                                   PassName, Fn, DLoc, Msg, Hotness) {}

  /// \p PassName is the name of the pass emitting this diagnostic. If this name
  /// matches the regular expression given in -Rpass-analysis=, then the
  /// diagnostic will be emitted.  \p RemarkName is a textual identifier for the
  /// remark.  \p DLoc is the debug location and \p CodeRegion is the region
  /// that the optimization operates on (currently on block is supported). The
  /// front-end will append its own message related to options that address
  /// pointer aliasing legality.
  OptimizationRemarkAnalysisAliasing(const char *PassName, StringRef RemarkName,
                                     const DebugLoc &DLoc, Value *CodeRegion)
      : OptimizationRemarkAnalysis(DK_OptimizationRemarkAnalysisAliasing,
                                   PassName, RemarkName, DLoc, CodeRegion) {}

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationRemarkAnalysisAliasing;
  }
};

/// Diagnostic information for machine IR parser.
class DiagnosticInfoMIRParser : public DiagnosticInfo {
  const SMDiagnostic &Diagnostic;

public:
  DiagnosticInfoMIRParser(DiagnosticSeverity Severity,
                          const SMDiagnostic &Diagnostic)
      : DiagnosticInfo(DK_MIRParser, Severity), Diagnostic(Diagnostic) {}

  const SMDiagnostic &getDiagnostic() const { return Diagnostic; }

  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_MIRParser;
  }
};

/// Diagnostic information for ISel fallback path.
class DiagnosticInfoISelFallback : public DiagnosticInfo {
  /// The function that is concerned by this diagnostic.
  const Function &Fn;

public:
  DiagnosticInfoISelFallback(const Function &Fn,
                             DiagnosticSeverity Severity = DS_Warning)
      : DiagnosticInfo(DK_ISelFallback, Severity), Fn(Fn) {}

  const Function &getFunction() const { return Fn; }

  void print(DiagnosticPrinter &DP) const override;

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_ISelFallback;
  }
};

// Create wrappers for C Binding types (see CBindingWrapping.h).
DEFINE_SIMPLE_CONVERSION_FUNCTIONS(DiagnosticInfo, LLVMDiagnosticInfoRef)

/// Emit an optimization-applied message. \p PassName is the name of the pass
/// emitting the message. If -Rpass= is given and \p PassName matches the
/// regular expression in -Rpass, then the remark will be emitted. \p Fn is
/// the function triggering the remark, \p DLoc is the debug location where
/// the diagnostic is generated. \p Msg is the message string to use.
void emitOptimizationRemark(LLVMContext &Ctx, const char *PassName,
                            const Function &Fn, const DebugLoc &DLoc,
                            const Twine &Msg);

/// Emit an optimization-missed message. \p PassName is the name of the
/// pass emitting the message. If -Rpass-missed= is given and \p PassName
/// matches the regular expression in -Rpass, then the remark will be
/// emitted. \p Fn is the function triggering the remark, \p DLoc is the
/// debug location where the diagnostic is generated. \p Msg is the
/// message string to use.
void emitOptimizationRemarkMissed(LLVMContext &Ctx, const char *PassName,
                                  const Function &Fn, const DebugLoc &DLoc,
                                  const Twine &Msg);

/// Emit an optimization analysis remark message. \p PassName is the name of
/// the pass emitting the message. If -Rpass-analysis= is given and \p
/// PassName matches the regular expression in -Rpass, then the remark will be
/// emitted. \p Fn is the function triggering the remark, \p DLoc is the debug
/// location where the diagnostic is generated. \p Msg is the message string
/// to use.
void emitOptimizationRemarkAnalysis(LLVMContext &Ctx, const char *PassName,
                                    const Function &Fn, const DebugLoc &DLoc,
                                    const Twine &Msg);

/// Emit an optimization analysis remark related to messages about
/// floating-point non-commutativity. \p PassName is the name of the pass
/// emitting the message. If -Rpass-analysis= is given and \p PassName matches
/// the regular expression in -Rpass, then the remark will be emitted. \p Fn is
/// the function triggering the remark, \p DLoc is the debug location where the
/// diagnostic is generated. \p Msg is the message string to use.
void emitOptimizationRemarkAnalysisFPCommute(LLVMContext &Ctx,
                                             const char *PassName,
                                             const Function &Fn,
                                             const DebugLoc &DLoc,
                                             const Twine &Msg);

/// Emit an optimization analysis remark related to messages about
/// pointer aliasing. \p PassName is the name of the pass emitting the message.
/// If -Rpass-analysis= is given and \p PassName matches the regular expression
/// in -Rpass, then the remark will be emitted. \p Fn is the function triggering
/// the remark, \p DLoc is the debug location where the diagnostic is generated.
/// \p Msg is the message string to use.
void emitOptimizationRemarkAnalysisAliasing(LLVMContext &Ctx,
                                            const char *PassName,
                                            const Function &Fn,
                                            const DebugLoc &DLoc,
                                            const Twine &Msg);

/// Diagnostic information for optimization failures.
class DiagnosticInfoOptimizationFailure
    : public DiagnosticInfoOptimizationBase {
public:
  /// \p Fn is the function where the diagnostic is being emitted. \p DLoc is
  /// the location information to use in the diagnostic. If line table
  /// information is available, the diagnostic will include the source code
  /// location. \p Msg is the message to show. Note that this class does not
  /// copy this message, so this reference must be valid for the whole life time
  /// of the diagnostic.
  DiagnosticInfoOptimizationFailure(const Function &Fn, const DebugLoc &DLoc,
                                    const Twine &Msg)
      : DiagnosticInfoOptimizationBase(DK_OptimizationFailure, DS_Warning,
                                       nullptr, Fn, DLoc, Msg) {}

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_OptimizationFailure;
  }

  /// \see DiagnosticInfoOptimizationBase::isEnabled.
  bool isEnabled() const override;
};

/// Diagnostic information for unsupported feature in backend.
class DiagnosticInfoUnsupported
    : public DiagnosticInfoWithDebugLocBase {
private:
  Twine Msg;

public:
  /// \p Fn is the function where the diagnostic is being emitted. \p DLoc is
  /// the location information to use in the diagnostic. If line table
  /// information is available, the diagnostic will include the source code
  /// location. \p Msg is the message to show. Note that this class does not
  /// copy this message, so this reference must be valid for the whole life time
  /// of the diagnostic.
  DiagnosticInfoUnsupported(const Function &Fn, const Twine &Msg,
                            DebugLoc DLoc = DebugLoc(),
                            DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfoWithDebugLocBase(DK_Unsupported, Severity, Fn, DLoc),
        Msg(Msg) {}

  static bool classof(const DiagnosticInfo *DI) {
    return DI->getKind() == DK_Unsupported;
  }

  const Twine &getMessage() const { return Msg; }

  void print(DiagnosticPrinter &DP) const override;
};

/// Emit a warning when loop vectorization is specified but fails. \p Fn is the
/// function triggering the warning, \p DLoc is the debug location where the
/// diagnostic is generated. \p Msg is the message string to use.
void emitLoopVectorizeWarning(LLVMContext &Ctx, const Function &Fn,
                              const DebugLoc &DLoc, const Twine &Msg);

/// Emit a warning when loop interleaving is specified but fails. \p Fn is the
/// function triggering the warning, \p DLoc is the debug location where the
/// diagnostic is generated. \p Msg is the message string to use.
void emitLoopInterleaveWarning(LLVMContext &Ctx, const Function &Fn,
                               const DebugLoc &DLoc, const Twine &Msg);

} // end namespace llvm

#endif // LLVM_IR_DIAGNOSTICINFO_H
