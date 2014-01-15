//===--- SanitizerArgs.cpp - Arguments for sanitizer tools  ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#include "clang/Driver/SanitizerArgs.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/ToolChain.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Transforms/Utils/SpecialCaseList.h"

using namespace clang::driver;
using namespace llvm::opt;

void SanitizerArgs::clear() {
  Kind = 0;
  BlacklistFile = "";
  MsanTrackOrigins = false;
  AsanZeroBaseShadow = false;
  UbsanTrapOnError = false;
}

SanitizerArgs::SanitizerArgs() {
  clear();
}

SanitizerArgs::SanitizerArgs(const ToolChain &TC,
                             const llvm::opt::ArgList &Args) {
  clear();
  unsigned AllAdd = 0;  // All kinds of sanitizers that were turned on
                        // at least once (possibly, disabled further).
  unsigned AllRemove = 0;  // During the loop below, the accumulated set of
                           // sanitizers disabled by the current sanitizer
                           // argument or any argument after it.
  unsigned DiagnosedKinds = 0;  // All Kinds we have diagnosed up to now.
                                // Used to deduplicate diagnostics.
  const Driver &D = TC.getDriver();
  for (ArgList::const_reverse_iterator I = Args.rbegin(), E = Args.rend();
       I != E; ++I) {
    unsigned Add, Remove;
    if (!parse(D, Args, *I, Add, Remove, true))
      continue;
    (*I)->claim();

    AllAdd |= expandGroups(Add);
    AllRemove |= expandGroups(Remove);

    // Avoid diagnosing any sanitizer which is disabled later.
    Add &= ~AllRemove;
    // At this point we have not expanded groups, so any unsupported sanitizers
    // in Add are those which have been explicitly enabled. Diagnose them.
    Add = filterUnsupportedKinds(TC, Add, Args, *I, /*DiagnoseErrors=*/true,
                                 DiagnosedKinds);
    Add = expandGroups(Add);
    // Group expansion may have enabled a sanitizer which is disabled later.
    Add &= ~AllRemove;
    // Silently discard any unsupported sanitizers implicitly enabled through
    // group expansion.
    Add = filterUnsupportedKinds(TC, Add, Args, *I, /*DiagnoseErrors=*/false,
                                 DiagnosedKinds);

    Kind |= Add;
  }

  UbsanTrapOnError =
    Args.hasArg(options::OPT_fcatch_undefined_behavior) ||
    Args.hasFlag(options::OPT_fsanitize_undefined_trap_on_error,
                 options::OPT_fno_sanitize_undefined_trap_on_error, false);

  if (Args.hasArg(options::OPT_fcatch_undefined_behavior) &&
      !Args.hasFlag(options::OPT_fsanitize_undefined_trap_on_error,
                    options::OPT_fno_sanitize_undefined_trap_on_error, true)) {
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << "-fcatch-undefined-behavior"
      << "-fno-sanitize-undefined-trap-on-error";
  }

  // Warn about undefined sanitizer options that require runtime support.
  if (UbsanTrapOnError && notAllowedWithTrap()) {
    if (Args.hasArg(options::OPT_fcatch_undefined_behavior))
      D.Diag(diag::err_drv_argument_not_allowed_with)
        << lastArgumentForKind(D, Args, NotAllowedWithTrap)
        << "-fcatch-undefined-behavior";
    else if (Args.hasFlag(options::OPT_fsanitize_undefined_trap_on_error,
                          options::OPT_fno_sanitize_undefined_trap_on_error,
                          false))
      D.Diag(diag::err_drv_argument_not_allowed_with)
        << lastArgumentForKind(D, Args, NotAllowedWithTrap)
        << "-fsanitize-undefined-trap-on-error";
  }

  // Only one runtime library can be used at once.
  bool NeedsAsan = needsAsanRt();
  bool NeedsTsan = needsTsanRt();
  bool NeedsMsan = needsMsanRt();
  bool NeedsLsan = needsLeakDetection();
  if (NeedsAsan && NeedsTsan)
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << lastArgumentForKind(D, Args, NeedsAsanRt)
      << lastArgumentForKind(D, Args, NeedsTsanRt);
  if (NeedsAsan && NeedsMsan)
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << lastArgumentForKind(D, Args, NeedsAsanRt)
      << lastArgumentForKind(D, Args, NeedsMsanRt);
  if (NeedsTsan && NeedsMsan)
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << lastArgumentForKind(D, Args, NeedsTsanRt)
      << lastArgumentForKind(D, Args, NeedsMsanRt);
  if (NeedsLsan && NeedsTsan)
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << lastArgumentForKind(D, Args, NeedsLeakDetection)
      << lastArgumentForKind(D, Args, NeedsTsanRt);
  if (NeedsLsan && NeedsMsan)
    D.Diag(diag::err_drv_argument_not_allowed_with)
      << lastArgumentForKind(D, Args, NeedsLeakDetection)
      << lastArgumentForKind(D, Args, NeedsMsanRt);
  // FIXME: Currently -fsanitize=leak is silently ignored in the presence of
  // -fsanitize=address. Perhaps it should print an error, or perhaps
  // -f(-no)sanitize=leak should change whether leak detection is enabled by
  // default in ASan?

  // If -fsanitize contains extra features of ASan, it should also
  // explicitly contain -fsanitize=address (probably, turned off later in the
  // command line).
  if ((Kind & AddressFull) != 0 && (AllAdd & Address) == 0)
    D.Diag(diag::warn_drv_unused_sanitizer)
     << lastArgumentForKind(D, Args, AddressFull)
     << "-fsanitize=address";

  // Parse -f(no-)sanitize-blacklist options.
  if (Arg *BLArg = Args.getLastArg(options::OPT_fsanitize_blacklist,
                                   options::OPT_fno_sanitize_blacklist)) {
    if (BLArg->getOption().matches(options::OPT_fsanitize_blacklist)) {
      std::string BLPath = BLArg->getValue();
      if (llvm::sys::fs::exists(BLPath)) {
        // Validate the blacklist format.
        std::string BLError;
        llvm::OwningPtr<llvm::SpecialCaseList> SCL(
            llvm::SpecialCaseList::create(BLPath, BLError));
        if (!SCL.get())
          D.Diag(diag::err_drv_malformed_sanitizer_blacklist) << BLError;
        else
          BlacklistFile = BLPath;
      } else {
        D.Diag(diag::err_drv_no_such_file) << BLPath;
      }
    }
  } else {
    // If no -fsanitize-blacklist option is specified, try to look up for
    // blacklist in the resource directory.
    std::string BLPath;
    if (getDefaultBlacklistForKind(D, Kind, BLPath) &&
        llvm::sys::fs::exists(BLPath))
      BlacklistFile = BLPath;
  }

  // Parse -f(no-)sanitize-memory-track-origins options.
  if (NeedsMsan)
    MsanTrackOrigins =
      Args.hasFlag(options::OPT_fsanitize_memory_track_origins,
                   options::OPT_fno_sanitize_memory_track_origins,
                   /* Default */false);

  // Parse -f(no-)sanitize-address-zero-base-shadow options.
  if (NeedsAsan) {
    bool IsAndroid = (TC.getTriple().getEnvironment() == llvm::Triple::Android);
    bool ZeroBaseShadowDefault = IsAndroid;
    AsanZeroBaseShadow =
        Args.hasFlag(options::OPT_fsanitize_address_zero_base_shadow,
                     options::OPT_fno_sanitize_address_zero_base_shadow,
                     ZeroBaseShadowDefault);
    // Zero-base shadow is a requirement on Android.
    if (IsAndroid && !AsanZeroBaseShadow) {
      D.Diag(diag::err_drv_argument_not_allowed_with)
          << "-fno-sanitize-address-zero-base-shadow"
          << lastArgumentForKind(D, Args, Address);
    }
  }
}

void SanitizerArgs::addArgs(const llvm::opt::ArgList &Args,
                            llvm::opt::ArgStringList &CmdArgs) const {
  if (!Kind)
    return;
  SmallString<256> SanitizeOpt("-fsanitize=");
#define SANITIZER(NAME, ID) \
  if (Kind & ID) \
    SanitizeOpt += NAME ",";
#include "clang/Basic/Sanitizers.def"
  SanitizeOpt.pop_back();
  CmdArgs.push_back(Args.MakeArgString(SanitizeOpt));
  if (!BlacklistFile.empty()) {
    SmallString<64> BlacklistOpt("-fsanitize-blacklist=");
    BlacklistOpt += BlacklistFile;
    CmdArgs.push_back(Args.MakeArgString(BlacklistOpt));
  }

  if (MsanTrackOrigins)
    CmdArgs.push_back(Args.MakeArgString("-fsanitize-memory-track-origins"));

  if (AsanZeroBaseShadow)
    CmdArgs.push_back(
        Args.MakeArgString("-fsanitize-address-zero-base-shadow"));

  // Workaround for PR16386.
  if (needsMsanRt())
    CmdArgs.push_back(Args.MakeArgString("-fno-assume-sane-operator-new"));
}

unsigned SanitizerArgs::parse(const char *Value) {
  unsigned ParsedKind = llvm::StringSwitch<SanitizeKind>(Value)
#define SANITIZER(NAME, ID) .Case(NAME, ID)
#define SANITIZER_GROUP(NAME, ID, ALIAS) .Case(NAME, ID##Group)
#include "clang/Basic/Sanitizers.def"
    .Default(SanitizeKind());
  // Assume -fsanitize=address implies -fsanitize=init-order,use-after-return.
  // FIXME: This should be either specified in Sanitizers.def, or go away when
  // we get rid of "-fsanitize=init-order,use-after-return" flags at all.
  if (ParsedKind & Address)
    ParsedKind |= InitOrder | UseAfterReturn;
  return ParsedKind;
}

unsigned SanitizerArgs::expandGroups(unsigned Kinds) {
#define SANITIZER(NAME, ID)
#define SANITIZER_GROUP(NAME, ID, ALIAS) if (Kinds & ID##Group) Kinds |= ID;
#include "clang/Basic/Sanitizers.def"
  return Kinds;
}

void SanitizerArgs::filterUnsupportedMask(const ToolChain &TC, unsigned &Kinds,
                                          unsigned Mask,
                                          const llvm::opt::ArgList &Args,
                                          const llvm::opt::Arg *A,
                                          bool DiagnoseErrors,
                                          unsigned &DiagnosedKinds) {
  unsigned MaskedKinds = Kinds & Mask;
  if (!MaskedKinds)
    return;
  Kinds &= ~Mask;
  // Do we have new kinds to diagnose?
  if (DiagnoseErrors && (DiagnosedKinds & MaskedKinds) != MaskedKinds) {
    // Only diagnose the new kinds.
    std::string Desc =
        describeSanitizeArg(Args, A, MaskedKinds & ~DiagnosedKinds);
    TC.getDriver().Diag(diag::err_drv_unsupported_opt_for_target)
        << Desc << TC.getTriple().str();
    DiagnosedKinds |= MaskedKinds;
  }
}

unsigned SanitizerArgs::filterUnsupportedKinds(const ToolChain &TC,
                                               unsigned Kinds,
                                               const llvm::opt::ArgList &Args,
                                               const llvm::opt::Arg *A,
                                               bool DiagnoseErrors,
                                               unsigned &DiagnosedKinds) {
  bool IsLinux = TC.getTriple().getOS() == llvm::Triple::Linux;
  bool IsX86 = TC.getTriple().getArch() == llvm::Triple::x86;
  bool IsX86_64 = TC.getTriple().getArch() == llvm::Triple::x86_64;
  if (!(IsLinux && IsX86_64)) {
    filterUnsupportedMask(TC, Kinds, Thread | Memory | DataFlow, Args, A,
                          DiagnoseErrors, DiagnosedKinds);
  }
  if (!(IsLinux && (IsX86 || IsX86_64))) {
    filterUnsupportedMask(TC, Kinds, Function, Args, A, DiagnoseErrors,
                          DiagnosedKinds);
  }
  return Kinds;
}

unsigned SanitizerArgs::parse(const Driver &D, const llvm::opt::Arg *A,
                              bool DiagnoseErrors) {
  unsigned Kind = 0;
  for (unsigned I = 0, N = A->getNumValues(); I != N; ++I) {
    if (unsigned K = parse(A->getValue(I)))
      Kind |= K;
    else if (DiagnoseErrors)
      D.Diag(diag::err_drv_unsupported_option_argument)
        << A->getOption().getName() << A->getValue(I);
  }
  return Kind;
}

bool SanitizerArgs::parse(const Driver &D, const llvm::opt::ArgList &Args,
                          const llvm::opt::Arg *A, unsigned &Add,
                          unsigned &Remove, bool DiagnoseErrors) {
  Add = 0;
  Remove = 0;
  const char *DeprecatedReplacement = 0;
  if (A->getOption().matches(options::OPT_faddress_sanitizer)) {
    Add = Address;
    DeprecatedReplacement = "-fsanitize=address";
  } else if (A->getOption().matches(options::OPT_fno_address_sanitizer)) {
    Remove = Address;
    DeprecatedReplacement = "-fno-sanitize=address";
  } else if (A->getOption().matches(options::OPT_fthread_sanitizer)) {
    Add = Thread;
    DeprecatedReplacement = "-fsanitize=thread";
  } else if (A->getOption().matches(options::OPT_fno_thread_sanitizer)) {
    Remove = Thread;
    DeprecatedReplacement = "-fno-sanitize=thread";
  } else if (A->getOption().matches(options::OPT_fcatch_undefined_behavior)) {
    Add = UndefinedTrap;
    DeprecatedReplacement =
      "-fsanitize=undefined-trap -fsanitize-undefined-trap-on-error";
  } else if (A->getOption().matches(options::OPT_fbounds_checking) ||
             A->getOption().matches(options::OPT_fbounds_checking_EQ)) {
    Add = LocalBounds;
    DeprecatedReplacement = "-fsanitize=local-bounds";
  } else if (A->getOption().matches(options::OPT_fsanitize_EQ)) {
    Add = parse(D, A, DiagnoseErrors);
  } else if (A->getOption().matches(options::OPT_fno_sanitize_EQ)) {
    Remove = parse(D, A, DiagnoseErrors);
  } else {
    // Flag is not relevant to sanitizers.
    return false;
  }
  // If this is a deprecated synonym, produce a warning directing users
  // towards the new spelling.
  if (DeprecatedReplacement && DiagnoseErrors)
    D.Diag(diag::warn_drv_deprecated_arg)
      << A->getAsString(Args) << DeprecatedReplacement;
  return true;
}

std::string SanitizerArgs::lastArgumentForKind(const Driver &D,
                                               const llvm::opt::ArgList &Args,
                                               unsigned Kind) {
  for (llvm::opt::ArgList::const_reverse_iterator I = Args.rbegin(),
                                                  E = Args.rend();
       I != E; ++I) {
    unsigned Add, Remove;
    if (parse(D, Args, *I, Add, Remove, false) &&
        (expandGroups(Add) & Kind))
      return describeSanitizeArg(Args, *I, Kind);
    Kind &= ~Remove;
  }
  llvm_unreachable("arg list didn't provide expected value");
}

std::string SanitizerArgs::describeSanitizeArg(const llvm::opt::ArgList &Args,
                                               const llvm::opt::Arg *A,
                                               unsigned Mask) {
  if (!A->getOption().matches(options::OPT_fsanitize_EQ))
    return A->getAsString(Args);

  std::string Sanitizers;
  for (unsigned I = 0, N = A->getNumValues(); I != N; ++I) {
    if (expandGroups(parse(A->getValue(I))) & Mask) {
      if (!Sanitizers.empty())
        Sanitizers += ",";
      Sanitizers += A->getValue(I);
    }
  }

  assert(!Sanitizers.empty() && "arg didn't provide expected value");
  return "-fsanitize=" + Sanitizers;
}

bool SanitizerArgs::getDefaultBlacklistForKind(const Driver &D, unsigned Kind,
                                               std::string &BLPath) {
  const char *BlacklistFile = 0;
  if (Kind & NeedsAsanRt)
    BlacklistFile = "asan_blacklist.txt";
  else if (Kind & NeedsMsanRt)
    BlacklistFile = "msan_blacklist.txt";
  else if (Kind & NeedsTsanRt)
    BlacklistFile = "tsan_blacklist.txt";
  else if (Kind & NeedsDfsanRt)
    BlacklistFile = "dfsan_abilist.txt";

  if (BlacklistFile) {
    SmallString<64> Path(D.ResourceDir);
    llvm::sys::path::append(Path, BlacklistFile);
    BLPath = Path.str();
    return true;
  }
  return false;
}
