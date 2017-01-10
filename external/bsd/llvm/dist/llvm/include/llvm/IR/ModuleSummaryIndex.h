//===-- llvm/ModuleSummaryIndex.h - Module Summary Index --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// @file
/// ModuleSummaryIndex.h This file contains the declarations the classes that
///  hold the module index and summary for function importing.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_IR_MODULESUMMARYINDEX_H
#define LLVM_IR_MODULESUMMARYINDEX_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/IR/Module.h"

#include <array>

namespace llvm {

namespace yaml {
template <typename T> struct MappingTraits;
}

/// \brief Class to accumulate and hold information about a callee.
struct CalleeInfo {
  enum class HotnessType : uint8_t { Unknown = 0, Cold = 1, None = 2, Hot = 3 };
  HotnessType Hotness = HotnessType::Unknown;

  CalleeInfo() = default;
  explicit CalleeInfo(HotnessType Hotness) : Hotness(Hotness) {}

  void updateHotness(const HotnessType OtherHotness) {
    Hotness = std::max(Hotness, OtherHotness);
  }
};

/// Struct to hold value either by GUID or GlobalValue*. Values in combined
/// indexes as well as indirect calls are GUIDs, all others are GlobalValues.
struct ValueInfo {
  /// The value representation used in this instance.
  enum ValueInfoKind {
    VI_GUID,
    VI_Value,
  };

  /// Union of the two possible value types.
  union ValueUnion {
    GlobalValue::GUID Id;
    const GlobalValue *GV;
    ValueUnion(GlobalValue::GUID Id) : Id(Id) {}
    ValueUnion(const GlobalValue *GV) : GV(GV) {}
  };

  /// The value being represented.
  ValueUnion TheValue;
  /// The value representation.
  ValueInfoKind Kind;
  /// Constructor for a GUID value
  ValueInfo(GlobalValue::GUID Id = 0) : TheValue(Id), Kind(VI_GUID) {}
  /// Constructor for a GlobalValue* value
  ValueInfo(const GlobalValue *V) : TheValue(V), Kind(VI_Value) {}
  /// Accessor for GUID value
  GlobalValue::GUID getGUID() const {
    assert(Kind == VI_GUID && "Not a GUID type");
    return TheValue.Id;
  }
  /// Accessor for GlobalValue* value
  const GlobalValue *getValue() const {
    assert(Kind == VI_Value && "Not a Value type");
    return TheValue.GV;
  }
  bool isGUID() const { return Kind == VI_GUID; }
};

template <> struct DenseMapInfo<ValueInfo> {
  static inline ValueInfo getEmptyKey() { return ValueInfo((GlobalValue *)-1); }
  static inline ValueInfo getTombstoneKey() {
    return ValueInfo((GlobalValue *)-2);
  }
  static bool isEqual(ValueInfo L, ValueInfo R) {
    if (L.isGUID() != R.isGUID())
      return false;
    return L.isGUID() ? (L.getGUID() == R.getGUID())
                      : (L.getValue() == R.getValue());
  }
  static unsigned getHashValue(ValueInfo I) {
    return I.isGUID() ? I.getGUID() : (uintptr_t)I.getValue();
  }
};

/// \brief Function and variable summary information to aid decisions and
/// implementation of importing.
class GlobalValueSummary {
public:
  /// \brief Sububclass discriminator (for dyn_cast<> et al.)
  enum SummaryKind : unsigned { AliasKind, FunctionKind, GlobalVarKind };

  /// Group flags (Linkage, NotEligibleToImport, etc.) as a bitfield.
  struct GVFlags {
    /// \brief The linkage type of the associated global value.
    ///
    /// One use is to flag values that have local linkage types and need to
    /// have module identifier appended before placing into the combined
    /// index, to disambiguate from other values with the same name.
    /// In the future this will be used to update and optimize linkage
    /// types based on global summary-based analysis.
    unsigned Linkage : 4;

    /// Indicate if the global value cannot be imported (e.g. it cannot
    /// be renamed or references something that can't be renamed).
    unsigned NotEligibleToImport : 1;

    /// Indicate that the global value must be considered a live root for
    /// index-based liveness analysis. Used for special LLVM values such as
    /// llvm.global_ctors that the linker does not know about.
    unsigned LiveRoot : 1;

    /// Convenience Constructors
    explicit GVFlags(GlobalValue::LinkageTypes Linkage,
                     bool NotEligibleToImport, bool LiveRoot)
        : Linkage(Linkage), NotEligibleToImport(NotEligibleToImport),
          LiveRoot(LiveRoot) {}
  };

private:
  /// Kind of summary for use in dyn_cast<> et al.
  SummaryKind Kind;

  GVFlags Flags;

  /// This is the hash of the name of the symbol in the original file. It is
  /// identical to the GUID for global symbols, but differs for local since the
  /// GUID includes the module level id in the hash.
  GlobalValue::GUID OriginalName;

  /// \brief Path of module IR containing value's definition, used to locate
  /// module during importing.
  ///
  /// This is only used during parsing of the combined index, or when
  /// parsing the per-module index for creation of the combined summary index,
  /// not during writing of the per-module index which doesn't contain a
  /// module path string table.
  StringRef ModulePath;

  /// List of values referenced by this global value's definition
  /// (either by the initializer of a global variable, or referenced
  /// from within a function). This does not include functions called, which
  /// are listed in the derived FunctionSummary object.
  std::vector<ValueInfo> RefEdgeList;

protected:
  /// GlobalValueSummary constructor.
  GlobalValueSummary(SummaryKind K, GVFlags Flags, std::vector<ValueInfo> Refs)
      : Kind(K), Flags(Flags), RefEdgeList(std::move(Refs)) {}

public:
  virtual ~GlobalValueSummary() = default;

  /// Returns the hash of the original name, it is identical to the GUID for
  /// externally visible symbols, but not for local ones.
  GlobalValue::GUID getOriginalName() { return OriginalName; }

  /// Initialize the original name hash in this summary.
  void setOriginalName(GlobalValue::GUID Name) { OriginalName = Name; }

  /// Which kind of summary subclass this is.
  SummaryKind getSummaryKind() const { return Kind; }

  /// Set the path to the module containing this function, for use in
  /// the combined index.
  void setModulePath(StringRef ModPath) { ModulePath = ModPath; }

  /// Get the path to the module containing this function.
  StringRef modulePath() const { return ModulePath; }

  /// Get the flags for this GlobalValue (see \p struct GVFlags).
  GVFlags flags() { return Flags; }

  /// Return linkage type recorded for this global value.
  GlobalValue::LinkageTypes linkage() const {
    return static_cast<GlobalValue::LinkageTypes>(Flags.Linkage);
  }

  /// Sets the linkage to the value determined by global summary-based
  /// optimization. Will be applied in the ThinLTO backends.
  void setLinkage(GlobalValue::LinkageTypes Linkage) {
    Flags.Linkage = Linkage;
  }

  /// Return true if this global value can't be imported.
  bool notEligibleToImport() const { return Flags.NotEligibleToImport; }

  /// Return true if this global value must be considered a root for live
  /// value analysis on the index.
  bool liveRoot() const { return Flags.LiveRoot; }

  /// Flag that this global value must be considered a root for live
  /// value analysis on the index.
  void setLiveRoot() { Flags.LiveRoot = true; }

  /// Flag that this global value cannot be imported.
  void setNotEligibleToImport() { Flags.NotEligibleToImport = true; }

  /// Return the list of values referenced by this global value definition.
  ArrayRef<ValueInfo> refs() const { return RefEdgeList; }
};

/// \brief Alias summary information.
class AliasSummary : public GlobalValueSummary {
  GlobalValueSummary *AliaseeSummary;

public:
  /// Summary constructors.
  AliasSummary(GVFlags Flags, std::vector<ValueInfo> Refs)
      : GlobalValueSummary(AliasKind, Flags, std::move(Refs)) {}

  /// Check if this is an alias summary.
  static bool classof(const GlobalValueSummary *GVS) {
    return GVS->getSummaryKind() == AliasKind;
  }

  void setAliasee(GlobalValueSummary *Aliasee) { AliaseeSummary = Aliasee; }

  const GlobalValueSummary &getAliasee() const {
    return const_cast<AliasSummary *>(this)->getAliasee();
  }

  GlobalValueSummary &getAliasee() {
    assert(AliaseeSummary && "Unexpected missing aliasee summary");
    return *AliaseeSummary;
  }
};

/// \brief Function summary information to aid decisions and implementation of
/// importing.
class FunctionSummary : public GlobalValueSummary {
public:
  /// <CalleeValueInfo, CalleeInfo> call edge pair.
  typedef std::pair<ValueInfo, CalleeInfo> EdgeTy;

private:
  /// Number of instructions (ignoring debug instructions, e.g.) computed
  /// during the initial compile step when the summary index is first built.
  unsigned InstCount;

  /// List of <CalleeValueInfo, CalleeInfo> call edge pairs from this function.
  std::vector<EdgeTy> CallGraphEdgeList;

  /// List of type identifiers used by this function, represented as GUIDs.
  std::vector<GlobalValue::GUID> TypeIdList;

public:
  /// Summary constructors.
  FunctionSummary(GVFlags Flags, unsigned NumInsts, std::vector<ValueInfo> Refs,
                  std::vector<EdgeTy> CGEdges,
                  std::vector<GlobalValue::GUID> TypeIds)
      : GlobalValueSummary(FunctionKind, Flags, std::move(Refs)),
        InstCount(NumInsts), CallGraphEdgeList(std::move(CGEdges)),
        TypeIdList(std::move(TypeIds)) {}

  /// Check if this is a function summary.
  static bool classof(const GlobalValueSummary *GVS) {
    return GVS->getSummaryKind() == FunctionKind;
  }

  /// Get the instruction count recorded for this function.
  unsigned instCount() const { return InstCount; }

  /// Return the list of <CalleeValueInfo, CalleeInfo> pairs.
  ArrayRef<EdgeTy> calls() const { return CallGraphEdgeList; }

  /// Returns the list of type identifiers used by this function.
  ArrayRef<GlobalValue::GUID> type_tests() const { return TypeIdList; }
};

/// \brief Global variable summary information to aid decisions and
/// implementation of importing.
///
/// Currently this doesn't add anything to the base \p GlobalValueSummary,
/// but is a placeholder as additional info may be added to the summary
/// for variables.
class GlobalVarSummary : public GlobalValueSummary {

public:
  /// Summary constructors.
  GlobalVarSummary(GVFlags Flags, std::vector<ValueInfo> Refs)
      : GlobalValueSummary(GlobalVarKind, Flags, std::move(Refs)) {}

  /// Check if this is a global variable summary.
  static bool classof(const GlobalValueSummary *GVS) {
    return GVS->getSummaryKind() == GlobalVarKind;
  }
};

struct TypeTestResolution {
  /// Specifies which kind of type check we should emit for this byte array.
  /// See http://clang.llvm.org/docs/ControlFlowIntegrityDesign.html for full
  /// details on each kind of check; the enumerators are described with
  /// reference to that document.
  enum Kind {
    Unsat,     ///< Unsatisfiable type (i.e. no global has this type metadata)
    ByteArray, ///< Test a byte array (first example)
    Inline,    ///< Inlined bit vector ("Short Inline Bit Vectors")
    Single,    ///< Single element (last example in "Short Inline Bit Vectors")
    AllOnes,   ///< All-ones bit vector ("Eliminating Bit Vector Checks for
               ///  All-Ones Bit Vectors")
  } TheKind = Unsat;

  /// Range of the size expressed as a bit width. For example, if the size is in
  /// range [0,256), this number will be 8. This helps generate the most compact
  /// instruction sequences.
  unsigned SizeBitWidth = 0;
};

struct TypeIdSummary {
  TypeTestResolution TTRes;
};

/// 160 bits SHA1
typedef std::array<uint32_t, 5> ModuleHash;

/// List of global value summary structures for a particular value held
/// in the GlobalValueMap. Requires a vector in the case of multiple
/// COMDAT values of the same name.
typedef std::vector<std::unique_ptr<GlobalValueSummary>> GlobalValueSummaryList;

/// Map from global value GUID to corresponding summary structures.
/// Use a std::map rather than a DenseMap since it will likely incur
/// less overhead, as the value type is not very small and the size
/// of the map is unknown, resulting in inefficiencies due to repeated
/// insertions and resizing.
typedef std::map<GlobalValue::GUID, GlobalValueSummaryList>
    GlobalValueSummaryMapTy;

/// Type used for iterating through the global value summary map.
typedef GlobalValueSummaryMapTy::const_iterator const_gvsummary_iterator;
typedef GlobalValueSummaryMapTy::iterator gvsummary_iterator;

/// String table to hold/own module path strings, which additionally holds the
/// module ID assigned to each module during the plugin step, as well as a hash
/// of the module. The StringMap makes a copy of and owns inserted strings.
typedef StringMap<std::pair<uint64_t, ModuleHash>> ModulePathStringTableTy;

/// Map of global value GUID to its summary, used to identify values defined in
/// a particular module, and provide efficient access to their summary.
typedef std::map<GlobalValue::GUID, GlobalValueSummary *> GVSummaryMapTy;

/// Class to hold module path string table and global value map,
/// and encapsulate methods for operating on them.
class ModuleSummaryIndex {
private:
  /// Map from value name to list of summary instances for values of that
  /// name (may be duplicates in the COMDAT case, e.g.).
  GlobalValueSummaryMapTy GlobalValueMap;

  /// Holds strings for combined index, mapping to the corresponding module ID.
  ModulePathStringTableTy ModulePathStringTable;

  /// Mapping from type identifiers to summary information for that type
  /// identifier.
  // FIXME: Add bitcode read/write support for this field.
  std::map<std::string, TypeIdSummary> TypeIdMap;

  // YAML I/O support.
  friend yaml::MappingTraits<ModuleSummaryIndex>;

public:
  gvsummary_iterator begin() { return GlobalValueMap.begin(); }
  const_gvsummary_iterator begin() const { return GlobalValueMap.begin(); }
  gvsummary_iterator end() { return GlobalValueMap.end(); }
  const_gvsummary_iterator end() const { return GlobalValueMap.end(); }
  size_t size() const { return GlobalValueMap.size(); }

  /// Get the list of global value summary objects for a given value name.
  const GlobalValueSummaryList &getGlobalValueSummaryList(StringRef ValueName) {
    return GlobalValueMap[GlobalValue::getGUID(ValueName)];
  }

  /// Get the list of global value summary objects for a given value name.
  const const_gvsummary_iterator
  findGlobalValueSummaryList(StringRef ValueName) const {
    return GlobalValueMap.find(GlobalValue::getGUID(ValueName));
  }

  /// Get the list of global value summary objects for a given value GUID.
  const const_gvsummary_iterator
  findGlobalValueSummaryList(GlobalValue::GUID ValueGUID) const {
    return GlobalValueMap.find(ValueGUID);
  }

  /// Add a global value summary for a value of the given name.
  void addGlobalValueSummary(StringRef ValueName,
                             std::unique_ptr<GlobalValueSummary> Summary) {
    GlobalValueMap[GlobalValue::getGUID(ValueName)].push_back(
        std::move(Summary));
  }

  /// Add a global value summary for a value of the given GUID.
  void addGlobalValueSummary(GlobalValue::GUID ValueGUID,
                             std::unique_ptr<GlobalValueSummary> Summary) {
    GlobalValueMap[ValueGUID].push_back(std::move(Summary));
  }

  /// Find the summary for global \p GUID in module \p ModuleId, or nullptr if
  /// not found.
  GlobalValueSummary *findSummaryInModule(GlobalValue::GUID ValueGUID,
                                          StringRef ModuleId) const {
    auto CalleeInfoList = findGlobalValueSummaryList(ValueGUID);
    if (CalleeInfoList == end()) {
      return nullptr; // This function does not have a summary
    }
    auto Summary =
        llvm::find_if(CalleeInfoList->second,
                      [&](const std::unique_ptr<GlobalValueSummary> &Summary) {
                        return Summary->modulePath() == ModuleId;
                      });
    if (Summary == CalleeInfoList->second.end())
      return nullptr;
    return Summary->get();
  }

  /// Returns the first GlobalValueSummary for \p GV, asserting that there
  /// is only one if \p PerModuleIndex.
  GlobalValueSummary *getGlobalValueSummary(const GlobalValue &GV,
                                            bool PerModuleIndex = true) const {
    assert(GV.hasName() && "Can't get GlobalValueSummary for GV with no name");
    return getGlobalValueSummary(GlobalValue::getGUID(GV.getName()),
                                 PerModuleIndex);
  }

  /// Returns the first GlobalValueSummary for \p ValueGUID, asserting that
  /// there
  /// is only one if \p PerModuleIndex.
  GlobalValueSummary *getGlobalValueSummary(GlobalValue::GUID ValueGUID,
                                            bool PerModuleIndex = true) const;

  /// Table of modules, containing module hash and id.
  const StringMap<std::pair<uint64_t, ModuleHash>> &modulePaths() const {
    return ModulePathStringTable;
  }

  /// Table of modules, containing hash and id.
  StringMap<std::pair<uint64_t, ModuleHash>> &modulePaths() {
    return ModulePathStringTable;
  }

  /// Get the module ID recorded for the given module path.
  uint64_t getModuleId(const StringRef ModPath) const {
    return ModulePathStringTable.lookup(ModPath).first;
  }

  /// Get the module SHA1 hash recorded for the given module path.
  const ModuleHash &getModuleHash(const StringRef ModPath) const {
    auto It = ModulePathStringTable.find(ModPath);
    assert(It != ModulePathStringTable.end() && "Module not registered");
    return It->second.second;
  }

  /// Add the given per-module index into this module index/summary,
  /// assigning it the given module ID. Each module merged in should have
  /// a unique ID, necessary for consistent renaming of promoted
  /// static (local) variables.
  void mergeFrom(std::unique_ptr<ModuleSummaryIndex> Other,
                 uint64_t NextModuleId);

  /// Convenience method for creating a promoted global name
  /// for the given value name of a local, and its original module's ID.
  static std::string getGlobalNameForLocal(StringRef Name, ModuleHash ModHash) {
    SmallString<256> NewName(Name);
    NewName += ".llvm.";
    NewName += utohexstr(ModHash[0]); // Take the first 32 bits
    return NewName.str();
  }

  /// Helper to obtain the unpromoted name for a global value (or the original
  /// name if not promoted).
  static StringRef getOriginalNameBeforePromote(StringRef Name) {
    std::pair<StringRef, StringRef> Pair = Name.split(".llvm.");
    return Pair.first;
  }

  /// Add a new module path with the given \p Hash, mapped to the given \p
  /// ModID, and return an iterator to the entry in the index.
  ModulePathStringTableTy::iterator
  addModulePath(StringRef ModPath, uint64_t ModId,
                ModuleHash Hash = ModuleHash{{0}}) {
    return ModulePathStringTable.insert(std::make_pair(
                                            ModPath,
                                            std::make_pair(ModId, Hash))).first;
  }

  /// Check if the given Module has any functions available for exporting
  /// in the index. We consider any module present in the ModulePathStringTable
  /// to have exported functions.
  bool hasExportedFunctions(const Module &M) const {
    return ModulePathStringTable.count(M.getModuleIdentifier());
  }

  /// Remove entries in the GlobalValueMap that have empty summaries due to the
  /// eager nature of map entry creation during VST parsing. These would
  /// also be suppressed during combined index generation in mergeFrom(),
  /// but if there was only one module or this was the first module we might
  /// not invoke mergeFrom.
  void removeEmptySummaryEntries();

  /// Collect for the given module the list of function it defines
  /// (GUID -> Summary).
  void collectDefinedFunctionsForModule(StringRef ModulePath,
                                        GVSummaryMapTy &GVSummaryMap) const;

  /// Collect for each module the list of Summaries it defines (GUID ->
  /// Summary).
  void collectDefinedGVSummariesPerModule(
      StringMap<GVSummaryMapTy> &ModuleToDefinedGVSummaries) const;
};

} // End llvm namespace

#endif
