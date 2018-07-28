//===------ Core.h -- Core ORC APIs (Layer, JITDylib, etc.) -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Contains core ORC APIs.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_CORE_H
#define LLVM_EXECUTIONENGINE_ORC_CORE_H

#include "llvm/ADT/BitmaskEnum.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/SymbolStringPool.h"
#include "llvm/IR/Module.h"

#include <list>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace llvm {
namespace orc {

// Forward declare some classes.
class AsynchronousSymbolQuery;
class ExecutionSession;
class MaterializationUnit;
class MaterializationResponsibility;
class VSO;

/// VModuleKey provides a unique identifier (allocated and managed by
/// ExecutionSessions) for a module added to the JIT.
using VModuleKey = uint64_t;

/// A set of symbol names (represented by SymbolStringPtrs for
//         efficiency).
using SymbolNameSet = std::set<SymbolStringPtr>;

/// Render a SymbolNameSet to an ostream.
raw_ostream &operator<<(raw_ostream &OS, const SymbolNameSet &Symbols);

/// A map from symbol names (as SymbolStringPtrs) to JITSymbols
///        (address/flags pairs).
using SymbolMap = std::map<SymbolStringPtr, JITEvaluatedSymbol>;

/// Render a SymbolMap to an ostream.
raw_ostream &operator<<(raw_ostream &OS, const SymbolMap &Symbols);

/// A map from symbol names (as SymbolStringPtrs) to JITSymbolFlags.
using SymbolFlagsMap = std::map<SymbolStringPtr, JITSymbolFlags>;

/// Render a SymbolMap to an ostream.
raw_ostream &operator<<(raw_ostream &OS, const SymbolFlagsMap &Symbols);

/// A base class for materialization failures that allows the failing
///        symbols to be obtained for logging.
using SymbolDependenceMap = std::map<VSO *, SymbolNameSet>;

/// Render a SymbolDependendeMap.
raw_ostream &operator<<(raw_ostream &OS, const SymbolDependenceMap &Deps);

/// Used to notify a VSO that the given set of symbols failed to materialize.
class FailedToMaterialize : public ErrorInfo<FailedToMaterialize> {
public:
  static char ID;

  FailedToMaterialize(SymbolNameSet Symbols);
  std::error_code convertToErrorCode() const override;
  void log(raw_ostream &OS) const override;
  const SymbolNameSet &getSymbols() const { return Symbols; }

private:
  SymbolNameSet Symbols;
};

/// Used to notify clients when symbols can not be found during a lookup.
class SymbolsNotFound : public ErrorInfo<SymbolsNotFound> {
public:
  static char ID;

  SymbolsNotFound(SymbolNameSet Symbols);
  std::error_code convertToErrorCode() const override;
  void log(raw_ostream &OS) const override;
  const SymbolNameSet &getSymbols() const { return Symbols; }

private:
  SymbolNameSet Symbols;
};

/// Tracks responsibility for materialization, and mediates interactions between
/// MaterializationUnits and VSOs.
///
/// An instance of this class is passed to MaterializationUnits when their
/// materialize method is called. It allows MaterializationUnits to resolve and
/// finalize symbols, or abandon materialization by notifying any unmaterialized
/// symbols of an error.
class MaterializationResponsibility {
  friend class MaterializationUnit;
public:
  MaterializationResponsibility(MaterializationResponsibility &&) = default;
  MaterializationResponsibility &
  operator=(MaterializationResponsibility &&) = default;

  /// Destruct a MaterializationResponsibility instance. In debug mode
  ///        this asserts that all symbols being tracked have been either
  ///        finalized or notified of an error.
  ~MaterializationResponsibility();

  /// Returns the target VSO that these symbols are being materialized
  ///        into.
  VSO &getTargetVSO() const { return V; }

  /// Returns the names of any symbols covered by this
  /// MaterializationResponsibility object that have queries pending. This
  /// information can be used to return responsibility for unrequested symbols
  /// back to the VSO via the delegate method.
  SymbolNameSet getRequestedSymbols();

  /// Resolves the given symbols. Individual calls to this method may
  ///        resolve a subset of the symbols, but all symbols must have been
  ///        resolved prior to calling finalize.
  void resolve(const SymbolMap &Symbols);

  /// Finalizes all symbols tracked by this instance.
  void finalize();

  /// Adds new symbols to the VSO and this responsibility instance.
  ///        VSO entries start out in the materializing state.
  ///
  ///   This method can be used by materialization units that want to add
  /// additional symbols at materialization time (e.g. stubs, compile
  /// callbacks, metadata).
  Error defineMaterializing(const SymbolFlagsMap &SymbolFlags);

  /// Notify all unfinalized symbols that an error has occurred.
  /// This will remove all symbols covered by this MaterializationResponsibilty
  /// from V, and send an error to any queries waiting on these symbols.
  void failMaterialization();

  /// Transfers responsibility to the given MaterializationUnit for all
  /// symbols defined by that MaterializationUnit. This allows
  /// materializers to break up work based on run-time information (e.g.
  /// by introspecting which symbols have actually been looked up and
  /// materializing only those).
  void replace(std::unique_ptr<MaterializationUnit> MU);

  /// Delegates responsibility for the given symbols to the returned
  /// materialization responsibility. Useful for breaking up work between
  /// threads, or different kinds of materialization processes.
  MaterializationResponsibility delegate(const SymbolNameSet &Symbols);

  /// Add dependencies for the symbols in this dylib.
  void addDependencies(const SymbolDependenceMap &Dependencies);

private:
  /// Create a MaterializationResponsibility for the given VSO and
  ///        initial symbols.
  MaterializationResponsibility(VSO &V, SymbolFlagsMap SymbolFlags);

  VSO &V;
  SymbolFlagsMap SymbolFlags;
};

/// A MaterializationUnit represents a set of symbol definitions that can
///        be materialized as a group, or individually discarded (when
///        overriding definitions are encountered).
///
/// MaterializationUnits are used when providing lazy definitions of symbols to
/// VSOs. The VSO will call materialize when the address of a symbol is
/// requested via the lookup method. The VSO will call discard if a stronger
/// definition is added or already present.
class MaterializationUnit {
public:
  MaterializationUnit(SymbolFlagsMap InitalSymbolFlags)
      : SymbolFlags(std::move(InitalSymbolFlags)) {}

  virtual ~MaterializationUnit() {}

  /// Return the set of symbols that this source provides.
  const SymbolFlagsMap &getSymbols() const { return SymbolFlags; }

  /// Called by materialization dispatchers (see
  /// ExecutionSession::DispatchMaterializationFunction) to trigger
  /// materialization of this MaterializationUnit.
  void doMaterialize(VSO &V) {
    materialize(MaterializationResponsibility(V, std::move(SymbolFlags)));
  }

  /// Called by VSOs to notify MaterializationUnits that the given symbol has
  /// been overridden.
  void doDiscard(const VSO &V, SymbolStringPtr Name) {
    SymbolFlags.erase(Name);
    discard(V, std::move(Name));
  }

protected:
  SymbolFlagsMap SymbolFlags;

private:
  virtual void anchor();

  /// Implementations of this method should materialize all symbols
  ///        in the materialzation unit, except for those that have been
  ///        previously discarded.
  virtual void materialize(MaterializationResponsibility R) = 0;

  /// Implementations of this method should discard the given symbol
  ///        from the source (e.g. if the source is an LLVM IR Module and the
  ///        symbol is a function, delete the function body or mark it available
  ///        externally).
  virtual void discard(const VSO &V, SymbolStringPtr Name) = 0;
};

/// A MaterializationUnit implementation for pre-existing absolute symbols.
///
/// All symbols will be resolved and marked ready as soon as the unit is
/// materialized.
class AbsoluteSymbolsMaterializationUnit : public MaterializationUnit {
public:
  AbsoluteSymbolsMaterializationUnit(SymbolMap Symbols);

private:
  void materialize(MaterializationResponsibility R) override;
  void discard(const VSO &V, SymbolStringPtr Name) override;
  static SymbolFlagsMap extractFlags(const SymbolMap &Symbols);

  SymbolMap Symbols;
};

/// Create an AbsoluteSymbolsMaterializationUnit with the given symbols.
/// Useful for inserting absolute symbols into a VSO. E.g.:
/// \code{.cpp}
///   VSO &V = ...;
///   SymbolStringPtr Foo = ...;
///   JITEvaluatedSymbol FooSym = ...;
///   if (auto Err = V.define(absoluteSymbols({{Foo, FooSym}})))
///     return Err;
/// \endcode
///
inline std::unique_ptr<AbsoluteSymbolsMaterializationUnit>
absoluteSymbols(SymbolMap Symbols) {
  return llvm::make_unique<AbsoluteSymbolsMaterializationUnit>(
      std::move(Symbols));
}

struct SymbolAliasMapEntry {
  SymbolAliasMapEntry() = default;
  SymbolAliasMapEntry(SymbolStringPtr Aliasee, JITSymbolFlags AliasFlags)
      : Aliasee(std::move(Aliasee)), AliasFlags(AliasFlags) {}

  SymbolStringPtr Aliasee;
  JITSymbolFlags AliasFlags;
};

/// A map of Symbols to (Symbol, Flags) pairs.
using SymbolAliasMap = std::map<SymbolStringPtr, SymbolAliasMapEntry>;

/// A materialization unit for symbol aliases. Allows existing symbols to be
/// aliased with alternate flags.
class ReExportsMaterializationUnit : public MaterializationUnit {
public:
  /// SourceVSO is allowed to be nullptr, in which case the source VSO is
  /// taken to be whatever VSO these definitions are materialized in. This
  /// is useful for defining aliases within a VSO.
  ///
  /// Note: Care must be taken that no sets of aliases form a cycle, as such
  ///       a cycle will result in a deadlock when any symbol in the cycle is
  ///       resolved.
  ReExportsMaterializationUnit(VSO *SourceVSO, SymbolAliasMap Aliases);

private:
  void materialize(MaterializationResponsibility R) override;
  void discard(const VSO &V, SymbolStringPtr Name) override;
  static SymbolFlagsMap extractFlags(const SymbolAliasMap &Aliases);

  VSO *SourceVSO = nullptr;
  SymbolAliasMap Aliases;
};

/// Create a ReExportsMaterializationUnit with the given aliases.
/// Useful for defining symbol aliases.: E.g., given a VSO V containing symbols
/// "foo" and "bar", we can define aliases "baz" (for "foo") and "qux" (for
/// "bar") with:
/// \code{.cpp}
///   SymbolStringPtr Baz = ...;
///   SymbolStringPtr Qux = ...;
///   if (auto Err = V.define(symbolAliases({
///       {Baz, { Foo, JITSymbolFlags::Exported }},
///       {Qux, { Bar, JITSymbolFlags::Weak }}}))
///     return Err;
/// \endcode
inline std::unique_ptr<ReExportsMaterializationUnit>
symbolAliases(SymbolAliasMap Aliases) {
  return llvm::make_unique<ReExportsMaterializationUnit>(nullptr,
                                                         std::move(Aliases));
}

/// Create a materialization unit for re-exporting symbols from another VSO
/// with alternative names/flags.
inline std::unique_ptr<ReExportsMaterializationUnit>
reexports(VSO &SourceV, SymbolAliasMap Aliases) {
  return llvm::make_unique<ReExportsMaterializationUnit>(&SourceV,
                                                         std::move(Aliases));
}

/// Build a SymbolAliasMap for the common case where you want to re-export
/// symbols from another VSO with the same linkage/flags.
Expected<SymbolAliasMap>
buildSimpleReexportsAliasMap(VSO &SourceV, const SymbolNameSet &Symbols);

/// Base utilities for ExecutionSession.
class ExecutionSessionBase {
public:
  /// For reporting errors.
  using ErrorReporter = std::function<void(Error)>;

  /// For dispatching MaterializationUnit::materialize calls.
  using DispatchMaterializationFunction =
      std::function<void(VSO &V, std::unique_ptr<MaterializationUnit> MU)>;

  /// Construct an ExecutionSessionBase.
  ///
  /// SymbolStringPools may be shared between ExecutionSessions.
  ExecutionSessionBase(std::shared_ptr<SymbolStringPool> SSP = nullptr)
      : SSP(SSP ? std::move(SSP) : std::make_shared<SymbolStringPool>()) {}

  /// Returns the SymbolStringPool for this ExecutionSession.
  SymbolStringPool &getSymbolStringPool() const { return *SSP; }

  /// Run the given lambda with the session mutex locked.
  template <typename Func> auto runSessionLocked(Func &&F) -> decltype(F()) {
    std::lock_guard<std::recursive_mutex> Lock(SessionMutex);
    return F();
  }

  /// Set the error reporter function.
  ExecutionSessionBase &setErrorReporter(ErrorReporter ReportError) {
    this->ReportError = std::move(ReportError);
    return *this;
  }

  /// Set the materialization dispatch function.
  ExecutionSessionBase &setDispatchMaterialization(
      DispatchMaterializationFunction DispatchMaterialization) {
    this->DispatchMaterialization = std::move(DispatchMaterialization);
    return *this;
  }

  /// Report a error for this execution session.
  ///
  /// Unhandled errors can be sent here to log them.
  void reportError(Error Err) { ReportError(std::move(Err)); }

  /// Allocate a module key for a new module to add to the JIT.
  VModuleKey allocateVModule() { return ++LastKey; }

  /// Return a module key to the ExecutionSession so that it can be
  ///        re-used. This should only be done once all resources associated
  ///        with the original key have been released.
  void releaseVModule(VModuleKey Key) { /* FIXME: Recycle keys */
  }

  /// Cause the given query to fail with the given Error.
  ///
  /// This should only be used by legacy APIs and will be deprecated in the
  /// future.
  void failQuery(AsynchronousSymbolQuery &Q, Error Err);

  /// Materialize the given unit.
  void dispatchMaterialization(VSO &V,
                               std::unique_ptr<MaterializationUnit> MU) {
    DispatchMaterialization(V, std::move(MU));
  }

private:
  static void logErrorsToStdErr(Error Err) {
    logAllUnhandledErrors(std::move(Err), errs(), "JIT session error: ");
  }

  static void
  materializeOnCurrentThread(VSO &V, std::unique_ptr<MaterializationUnit> MU) {
    MU->doMaterialize(V);
  }

  mutable std::recursive_mutex SessionMutex;
  std::shared_ptr<SymbolStringPool> SSP;
  VModuleKey LastKey = 0;
  ErrorReporter ReportError = logErrorsToStdErr;
  DispatchMaterializationFunction DispatchMaterialization =
      materializeOnCurrentThread;
};

/// A symbol query that returns results via a callback when results are
///        ready.
///
/// makes a callback when all symbols are available.
class AsynchronousSymbolQuery {
  friend class ExecutionSessionBase;
  friend class VSO;

public:
  class ResolutionResult {
  public:
    ResolutionResult(SymbolMap Symbols, const SymbolDependenceMap &Dependencies)
        : Symbols(std::move(Symbols)), Dependencies(Dependencies) {}

    SymbolMap Symbols;
    const SymbolDependenceMap &Dependencies;
  };

  /// Callback to notify client that symbols have been resolved.
  using SymbolsResolvedCallback =
      std::function<void(Expected<ResolutionResult>)>;

  /// Callback to notify client that symbols are ready for execution.
  using SymbolsReadyCallback = std::function<void(Error)>;

  /// Create a query for the given symbols, notify-resolved and
  ///        notify-ready callbacks.
  AsynchronousSymbolQuery(const SymbolNameSet &Symbols,
                          SymbolsResolvedCallback NotifySymbolsResolved,
                          SymbolsReadyCallback NotifySymbolsReady);

  /// Set the resolved symbol information for the given symbol name.
  void resolve(const SymbolStringPtr &Name, JITEvaluatedSymbol Sym);

  /// Returns true if all symbols covered by this query have been
  ///        resolved.
  bool isFullyResolved() const { return NotYetResolvedCount == 0; }

  /// Call the NotifySymbolsResolved callback.
  ///
  /// This should only be called if all symbols covered by the query have been
  /// resolved.
  void handleFullyResolved();

  /// Notify the query that a requested symbol is ready for execution.
  void notifySymbolReady();

  /// Returns true if all symbols covered by this query are ready.
  bool isFullyReady() const { return NotYetReadyCount == 0; }

  /// Calls the NotifySymbolsReady callback.
  ///
  /// This should only be called if all symbols covered by this query are ready.
  void handleFullyReady();

private:
  void addQueryDependence(VSO &V, SymbolStringPtr Name);

  void removeQueryDependence(VSO &V, const SymbolStringPtr &Name);

  bool canStillFail();

  void handleFailed(Error Err);

  void detach();

  SymbolsResolvedCallback NotifySymbolsResolved;
  SymbolsReadyCallback NotifySymbolsReady;
  SymbolDependenceMap QueryRegistrations;
  SymbolMap ResolvedSymbols;
  size_t NotYetResolvedCount;
  size_t NotYetReadyCount;
};

/// SymbolResolver is a composable interface for looking up symbol flags
///        and addresses using the AsynchronousSymbolQuery type. It will
///        eventually replace the LegacyJITSymbolResolver interface as the
///        stardard ORC symbol resolver type.
///
/// FIXME: SymbolResolvers should go away and be replaced with VSOs with
///        defenition generators.
class SymbolResolver {
public:
  virtual ~SymbolResolver() = default;

  /// Returns the flags for each symbol in Symbols that can be found,
  ///        along with the set of symbol that could not be found.
  virtual SymbolNameSet lookupFlags(SymbolFlagsMap &Flags,
                                    const SymbolNameSet &Symbols) = 0;

  /// For each symbol in Symbols that can be found, assigns that symbols
  ///        value in Query. Returns the set of symbols that could not be found.
  virtual SymbolNameSet lookup(std::shared_ptr<AsynchronousSymbolQuery> Query,
                               SymbolNameSet Symbols) = 0;

private:
  virtual void anchor();
};

/// Implements SymbolResolver with a pair of supplied function objects
///        for convenience. See createSymbolResolver.
template <typename LookupFlagsFn, typename LookupFn>
class LambdaSymbolResolver final : public SymbolResolver {
public:
  template <typename LookupFlagsFnRef, typename LookupFnRef>
  LambdaSymbolResolver(LookupFlagsFnRef &&LookupFlags, LookupFnRef &&Lookup)
      : LookupFlags(std::forward<LookupFlagsFnRef>(LookupFlags)),
        Lookup(std::forward<LookupFnRef>(Lookup)) {}

  SymbolNameSet lookupFlags(SymbolFlagsMap &Flags,
                            const SymbolNameSet &Symbols) final {
    return LookupFlags(Flags, Symbols);
  }

  SymbolNameSet lookup(std::shared_ptr<AsynchronousSymbolQuery> Query,
                       SymbolNameSet Symbols) final {
    return Lookup(std::move(Query), std::move(Symbols));
  }

private:
  LookupFlagsFn LookupFlags;
  LookupFn Lookup;
};

/// Creates a SymbolResolver implementation from the pair of supplied
///        function objects.
template <typename LookupFlagsFn, typename LookupFn>
std::unique_ptr<LambdaSymbolResolver<
    typename std::remove_cv<
        typename std::remove_reference<LookupFlagsFn>::type>::type,
    typename std::remove_cv<
        typename std::remove_reference<LookupFn>::type>::type>>
createSymbolResolver(LookupFlagsFn &&LookupFlags, LookupFn &&Lookup) {
  using LambdaSymbolResolverImpl = LambdaSymbolResolver<
      typename std::remove_cv<
          typename std::remove_reference<LookupFlagsFn>::type>::type,
      typename std::remove_cv<
          typename std::remove_reference<LookupFn>::type>::type>;
  return llvm::make_unique<LambdaSymbolResolverImpl>(
      std::forward<LookupFlagsFn>(LookupFlags), std::forward<LookupFn>(Lookup));
}

/// A symbol table that supports asynchoronous symbol queries.
///
/// Represents a virtual shared object. Instances can not be copied or moved, so
/// their addresses may be used as keys for resource management.
/// VSO state changes must be made via an ExecutionSession to guarantee that
/// they are synchronized with respect to other VSO operations.
class VSO {
  friend class AsynchronousSymbolQuery;
  friend class ExecutionSession;
  friend class MaterializationResponsibility;
public:
  using FallbackDefinitionGeneratorFunction =
      std::function<SymbolNameSet(VSO &Parent, const SymbolNameSet &Names)>;

  using AsynchronousSymbolQuerySet =
      std::set<std::shared_ptr<AsynchronousSymbolQuery>>;

  using MaterializationUnitList =
      std::vector<std::unique_ptr<MaterializationUnit>>;

  VSO(const VSO &) = delete;
  VSO &operator=(const VSO &) = delete;
  VSO(VSO &&) = delete;
  VSO &operator=(VSO &&) = delete;

  /// Get the name for this VSO.
  const std::string &getName() const { return VSOName; }

  /// Get a reference to the ExecutionSession for this VSO.
  ExecutionSessionBase &getExecutionSession() const { return ES; }

  /// Set a fallback defenition generator. If set, lookup and lookupFlags will
  /// pass the unresolved symbols set to the fallback definition generator,
  /// allowing it to add a new definition to the VSO.
  void setFallbackDefinitionGenerator(
      FallbackDefinitionGeneratorFunction FallbackDefinitionGenerator) {
    this->FallbackDefinitionGenerator = std::move(FallbackDefinitionGenerator);
  }

  /// Define all symbols provided by the materialization unit to be part
  ///        of the given VSO.
  template <typename UniquePtrToMaterializationUnit>
  typename std::enable_if<
      std::is_convertible<
          typename std::decay<UniquePtrToMaterializationUnit>::type,
          std::unique_ptr<MaterializationUnit>>::value,
      Error>::type
  define(UniquePtrToMaterializationUnit &&MU) {
    return ES.runSessionLocked([&, this]() -> Error {
      assert(MU && "Can't define with a null MU");

      if (auto Err = defineImpl(*MU))
        return Err;

      /// defineImpl succeeded.
      auto UMI = std::make_shared<UnmaterializedInfo>(std::move(MU));
      for (auto &KV : UMI->MU->getSymbols())
        UnmaterializedInfos[KV.first] = UMI;

      return Error::success();
    });
  }

  /// Search the given VSO for the symbols in Symbols. If found, store
  ///        the flags for each symbol in Flags. Returns any unresolved symbols.
  SymbolNameSet lookupFlags(SymbolFlagsMap &Flags, const SymbolNameSet &Names);

  /// Search the given VSOs in order for the symbols in Symbols. Results
  ///        (once they become available) will be returned via the given Query.
  ///
  /// If any symbol is not found then the unresolved symbols will be returned,
  /// and the query will not be applied. The Query is not failed and can be
  /// re-used in a subsequent lookup once the symbols have been added, or
  /// manually failed.
  SymbolNameSet lookup(std::shared_ptr<AsynchronousSymbolQuery> Q,
                       SymbolNameSet Names);

  /// Dump current VSO state to OS.
  void dump(raw_ostream &OS);

private:
  using AsynchronousSymbolQueryList =
      std::vector<std::shared_ptr<AsynchronousSymbolQuery>>;

  struct UnmaterializedInfo {
    UnmaterializedInfo(std::unique_ptr<MaterializationUnit> MU)
        : MU(std::move(MU)) {}

    std::unique_ptr<MaterializationUnit> MU;
  };

  using UnmaterializedInfosMap =
      std::map<SymbolStringPtr, std::shared_ptr<UnmaterializedInfo>>;

  struct MaterializingInfo {
    AsynchronousSymbolQueryList PendingQueries;
    SymbolDependenceMap Dependants;
    SymbolDependenceMap UnfinalizedDependencies;
    bool IsFinalized = false;
  };

  using MaterializingInfosMap = std::map<SymbolStringPtr, MaterializingInfo>;

  using LookupImplActionFlags = enum {
    None = 0,
    NotifyFullyResolved = 1 << 0U,
    NotifyFullyReady = 1 << 1U,
    LLVM_MARK_AS_BITMASK_ENUM(NotifyFullyReady)
  };

  VSO(ExecutionSessionBase &ES, std::string Name)
      : ES(ES), VSOName(std::move(Name)) {}

  Error defineImpl(MaterializationUnit &MU);

  SymbolNameSet lookupFlagsImpl(SymbolFlagsMap &Flags,
                                const SymbolNameSet &Names);

  LookupImplActionFlags
  lookupImpl(std::shared_ptr<AsynchronousSymbolQuery> &Q,
             std::vector<std::unique_ptr<MaterializationUnit>> &MUs,
             SymbolNameSet &Unresolved);

  void detachQueryHelper(AsynchronousSymbolQuery &Q,
                         const SymbolNameSet &QuerySymbols);

  void transferFinalizedNodeDependencies(MaterializingInfo &DependantMI,
                                         const SymbolStringPtr &DependantName,
                                         MaterializingInfo &FinalizedMI);

  Error defineMaterializing(const SymbolFlagsMap &SymbolFlags);

  void replace(std::unique_ptr<MaterializationUnit> MU);

  SymbolNameSet getRequestedSymbols(const SymbolFlagsMap &SymbolFlags);

  void addDependencies(const SymbolFlagsMap &Dependents,
                       const SymbolDependenceMap &Dependencies);

  void resolve(const SymbolMap &Resolved);

  void finalize(const SymbolFlagsMap &Finalized);

  void notifyFailed(const SymbolNameSet &FailedSymbols);

  void runOutstandingMUs();

  ExecutionSessionBase &ES;
  std::string VSOName;
  SymbolMap Symbols;
  UnmaterializedInfosMap UnmaterializedInfos;
  MaterializingInfosMap MaterializingInfos;
  FallbackDefinitionGeneratorFunction FallbackDefinitionGenerator;

  // FIXME: Remove this (and runOutstandingMUs) once the linking layer works
  //        with callbacks from asynchronous queries.
  mutable std::recursive_mutex OutstandingMUsMutex;
  std::vector<std::unique_ptr<MaterializationUnit>> OutstandingMUs;
};

/// An ExecutionSession represents a running JIT program.
class ExecutionSession : public ExecutionSessionBase {
public:
  using ErrorReporter = std::function<void(Error)>;

  using DispatchMaterializationFunction =
      std::function<void(VSO &V, std::unique_ptr<MaterializationUnit> MU)>;

  /// Construct an ExecutionEngine.
  ///
  /// SymbolStringPools may be shared between ExecutionSessions.
  ExecutionSession(std::shared_ptr<SymbolStringPool> SSP = nullptr)
      : ExecutionSessionBase(std::move(SSP)) {}

  /// Add a new VSO to this ExecutionSession.
  VSO &createVSO(std::string Name);

private:
  std::vector<std::unique_ptr<VSO>> VSOs;
};

using AsynchronousLookupFunction = std::function<SymbolNameSet(
    std::shared_ptr<AsynchronousSymbolQuery> Q, SymbolNameSet Names)>;

/// Perform a blocking lookup on the given symbols.
Expected<SymbolMap> blockingLookup(ExecutionSessionBase &ES,
                                   AsynchronousLookupFunction AsyncLookup,
                                   SymbolNameSet Names, bool WaiUntilReady,
                                   MaterializationResponsibility *MR = nullptr);

using VSOList = std::vector<VSO *>;

/// Look up the given names in the given VSOs.
/// VSOs will be searched in order and no VSO pointer may be null.
/// All symbols must be found within the given VSOs or an error
/// will be returned.
Expected<SymbolMap> lookup(const VSOList &VSOs, SymbolNameSet Names);

/// Look up a symbol by searching a list of VSOs.
Expected<JITEvaluatedSymbol> lookup(const VSOList &VSOs, SymbolStringPtr Name);

/// Mangles symbol names then uniques them in the context of an
/// ExecutionSession.
class MangleAndInterner {
public:
  MangleAndInterner(ExecutionSessionBase &ES, const DataLayout &DL);
  SymbolStringPtr operator()(StringRef Name);

private:
  ExecutionSessionBase &ES;
  const DataLayout &DL;
};

} // End namespace orc
} // End namespace llvm

#endif // LLVM_EXECUTIONENGINE_ORC_CORE_H
