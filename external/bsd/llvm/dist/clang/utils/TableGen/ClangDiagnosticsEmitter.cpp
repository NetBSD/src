//=- ClangDiagnosticsEmitter.cpp - Generate Clang diagnostics tables -*- C++ -*-
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// These tablegen backends emit Clang diagnostics tables.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/TableGen/Error.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/StringToOffsetTable.h"
#include "llvm/TableGen/TableGenBackend.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <set>
using namespace llvm;

//===----------------------------------------------------------------------===//
// Diagnostic category computation code.
//===----------------------------------------------------------------------===//

namespace {
class DiagGroupParentMap {
  RecordKeeper &Records;
  std::map<const Record*, std::vector<Record*> > Mapping;
public:
  DiagGroupParentMap(RecordKeeper &records) : Records(records) {
    std::vector<Record*> DiagGroups
      = Records.getAllDerivedDefinitions("DiagGroup");
    for (unsigned i = 0, e = DiagGroups.size(); i != e; ++i) {
      std::vector<Record*> SubGroups =
        DiagGroups[i]->getValueAsListOfDefs("SubGroups");
      for (unsigned j = 0, e = SubGroups.size(); j != e; ++j)
        Mapping[SubGroups[j]].push_back(DiagGroups[i]);
    }
  }

  const std::vector<Record*> &getParents(const Record *Group) {
    return Mapping[Group];
  }
};
} // end anonymous namespace.

static std::string
getCategoryFromDiagGroup(const Record *Group,
                         DiagGroupParentMap &DiagGroupParents) {
  // If the DiagGroup has a category, return it.
  std::string CatName = Group->getValueAsString("CategoryName");
  if (!CatName.empty()) return CatName;

  // The diag group may the subgroup of one or more other diagnostic groups,
  // check these for a category as well.
  const std::vector<Record*> &Parents = DiagGroupParents.getParents(Group);
  for (unsigned i = 0, e = Parents.size(); i != e; ++i) {
    CatName = getCategoryFromDiagGroup(Parents[i], DiagGroupParents);
    if (!CatName.empty()) return CatName;
  }
  return "";
}

/// getDiagnosticCategory - Return the category that the specified diagnostic
/// lives in.
static std::string getDiagnosticCategory(const Record *R,
                                         DiagGroupParentMap &DiagGroupParents) {
  // If the diagnostic is in a group, and that group has a category, use it.
  if (DefInit *Group = dyn_cast<DefInit>(R->getValueInit("Group"))) {
    // Check the diagnostic's diag group for a category.
    std::string CatName = getCategoryFromDiagGroup(Group->getDef(),
                                                   DiagGroupParents);
    if (!CatName.empty()) return CatName;
  }

  // If the diagnostic itself has a category, get it.
  return R->getValueAsString("CategoryName");
}

namespace {
  class DiagCategoryIDMap {
    RecordKeeper &Records;
    StringMap<unsigned> CategoryIDs;
    std::vector<std::string> CategoryStrings;
  public:
    DiagCategoryIDMap(RecordKeeper &records) : Records(records) {
      DiagGroupParentMap ParentInfo(Records);

      // The zero'th category is "".
      CategoryStrings.push_back("");
      CategoryIDs[""] = 0;

      std::vector<Record*> Diags =
      Records.getAllDerivedDefinitions("Diagnostic");
      for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
        std::string Category = getDiagnosticCategory(Diags[i], ParentInfo);
        if (Category.empty()) continue;  // Skip diags with no category.

        unsigned &ID = CategoryIDs[Category];
        if (ID != 0) continue;  // Already seen.

        ID = CategoryStrings.size();
        CategoryStrings.push_back(Category);
      }
    }

    unsigned getID(StringRef CategoryString) {
      return CategoryIDs[CategoryString];
    }

    typedef std::vector<std::string>::const_iterator const_iterator;
    const_iterator begin() const { return CategoryStrings.begin(); }
    const_iterator end() const { return CategoryStrings.end(); }
  };

  struct GroupInfo {
    std::vector<const Record*> DiagsInGroup;
    std::vector<std::string> SubGroups;
    unsigned IDNo;

    const Record *ExplicitDef;

    GroupInfo() : ExplicitDef(0) {}
  };
} // end anonymous namespace.

static bool beforeThanCompare(const Record *LHS, const Record *RHS) {
  assert(!LHS->getLoc().empty() && !RHS->getLoc().empty());
  return
    LHS->getLoc().front().getPointer() < RHS->getLoc().front().getPointer();
}

static bool beforeThanCompareGroups(const GroupInfo *LHS, const GroupInfo *RHS){
  assert(!LHS->DiagsInGroup.empty() && !RHS->DiagsInGroup.empty());
  return beforeThanCompare(LHS->DiagsInGroup.front(),
                           RHS->DiagsInGroup.front());
}

static SMRange findSuperClassRange(const Record *R, StringRef SuperName) {
  ArrayRef<Record *> Supers = R->getSuperClasses();

  for (size_t i = 0, e = Supers.size(); i < e; ++i)
    if (Supers[i]->getName() == SuperName)
      return R->getSuperClassRanges()[i];

  return SMRange();
}

/// \brief Invert the 1-[0/1] mapping of diags to group into a one to many
/// mapping of groups to diags in the group.
static void groupDiagnostics(const std::vector<Record*> &Diags,
                             const std::vector<Record*> &DiagGroups,
                             std::map<std::string, GroupInfo> &DiagsInGroup) {

  for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
    const Record *R = Diags[i];
    DefInit *DI = dyn_cast<DefInit>(R->getValueInit("Group"));
    if (DI == 0) continue;
    assert(R->getValueAsDef("Class")->getName() != "CLASS_NOTE" &&
           "Note can't be in a DiagGroup");
    std::string GroupName = DI->getDef()->getValueAsString("GroupName");
    DiagsInGroup[GroupName].DiagsInGroup.push_back(R);
  }

  typedef SmallPtrSet<GroupInfo *, 16> GroupSetTy;
  GroupSetTy ImplicitGroups;

  // Add all DiagGroup's to the DiagsInGroup list to make sure we pick up empty
  // groups (these are warnings that GCC supports that clang never produces).
  for (unsigned i = 0, e = DiagGroups.size(); i != e; ++i) {
    Record *Group = DiagGroups[i];
    GroupInfo &GI = DiagsInGroup[Group->getValueAsString("GroupName")];
    if (Group->isAnonymous()) {
      if (GI.DiagsInGroup.size() > 1)
        ImplicitGroups.insert(&GI);
    } else {
      if (GI.ExplicitDef)
        assert(GI.ExplicitDef == Group);
      else
        GI.ExplicitDef = Group;
    }

    std::vector<Record*> SubGroups = Group->getValueAsListOfDefs("SubGroups");
    for (unsigned j = 0, e = SubGroups.size(); j != e; ++j)
      GI.SubGroups.push_back(SubGroups[j]->getValueAsString("GroupName"));
  }

  // Assign unique ID numbers to the groups.
  unsigned IDNo = 0;
  for (std::map<std::string, GroupInfo>::iterator
       I = DiagsInGroup.begin(), E = DiagsInGroup.end(); I != E; ++I, ++IDNo)
    I->second.IDNo = IDNo;

  // Sort the implicit groups, so we can warn about them deterministically.
  SmallVector<GroupInfo *, 16> SortedGroups(ImplicitGroups.begin(),
                                            ImplicitGroups.end());
  for (SmallVectorImpl<GroupInfo *>::iterator I = SortedGroups.begin(),
                                              E = SortedGroups.end();
       I != E; ++I) {
    MutableArrayRef<const Record *> GroupDiags = (*I)->DiagsInGroup;
    std::sort(GroupDiags.begin(), GroupDiags.end(), beforeThanCompare);
  }
  std::sort(SortedGroups.begin(), SortedGroups.end(), beforeThanCompareGroups);

  // Warn about the same group being used anonymously in multiple places.
  for (SmallVectorImpl<GroupInfo *>::const_iterator I = SortedGroups.begin(),
                                                    E = SortedGroups.end();
       I != E; ++I) {
    ArrayRef<const Record *> GroupDiags = (*I)->DiagsInGroup;

    if ((*I)->ExplicitDef) {
      std::string Name = (*I)->ExplicitDef->getValueAsString("GroupName");
      for (ArrayRef<const Record *>::const_iterator DI = GroupDiags.begin(),
                                                    DE = GroupDiags.end();
           DI != DE; ++DI) {
        const DefInit *GroupInit = cast<DefInit>((*DI)->getValueInit("Group"));
        const Record *NextDiagGroup = GroupInit->getDef();
        if (NextDiagGroup == (*I)->ExplicitDef)
          continue;

        SMRange InGroupRange = findSuperClassRange(*DI, "InGroup");
        SmallString<64> Replacement;
        if (InGroupRange.isValid()) {
          Replacement += "InGroup<";
          Replacement += (*I)->ExplicitDef->getName();
          Replacement += ">";
        }
        SMFixIt FixIt(InGroupRange, Replacement.str());

        SrcMgr.PrintMessage(NextDiagGroup->getLoc().front(),
                            SourceMgr::DK_Error,
                            Twine("group '") + Name +
                              "' is referred to anonymously",
                            None,
                            InGroupRange.isValid() ? FixIt
                                                   : ArrayRef<SMFixIt>());
        SrcMgr.PrintMessage((*I)->ExplicitDef->getLoc().front(),
                            SourceMgr::DK_Note, "group defined here");
      }
    } else {
      // If there's no existing named group, we should just warn once and use
      // notes to list all the other cases.
      ArrayRef<const Record *>::const_iterator DI = GroupDiags.begin(),
                                               DE = GroupDiags.end();
      assert(DI != DE && "We only care about groups with multiple uses!");

      const DefInit *GroupInit = cast<DefInit>((*DI)->getValueInit("Group"));
      const Record *NextDiagGroup = GroupInit->getDef();
      std::string Name = NextDiagGroup->getValueAsString("GroupName");

      SMRange InGroupRange = findSuperClassRange(*DI, "InGroup");
      SrcMgr.PrintMessage(NextDiagGroup->getLoc().front(),
                          SourceMgr::DK_Error,
                          Twine("group '") + Name +
                            "' is referred to anonymously",
                          InGroupRange);

      for (++DI; DI != DE; ++DI) {
        GroupInit = cast<DefInit>((*DI)->getValueInit("Group"));
        InGroupRange = findSuperClassRange(*DI, "InGroup");
        SrcMgr.PrintMessage(GroupInit->getDef()->getLoc().front(),
                            SourceMgr::DK_Note, "also referenced here",
                            InGroupRange);
      }
    }
  }
}

//===----------------------------------------------------------------------===//
// Infer members of -Wpedantic.
//===----------------------------------------------------------------------===//

typedef std::vector<const Record *> RecordVec;
typedef llvm::DenseSet<const Record *> RecordSet;
typedef llvm::PointerUnion<RecordVec*, RecordSet*> VecOrSet;

namespace {
class InferPedantic {
  typedef llvm::DenseMap<const Record*,
                         std::pair<unsigned, Optional<unsigned> > > GMap;

  DiagGroupParentMap &DiagGroupParents;
  const std::vector<Record*> &Diags;
  const std::vector<Record*> DiagGroups;
  std::map<std::string, GroupInfo> &DiagsInGroup;
  llvm::DenseSet<const Record*> DiagsSet;
  GMap GroupCount;
public:
  InferPedantic(DiagGroupParentMap &DiagGroupParents,
                const std::vector<Record*> &Diags,
                const std::vector<Record*> &DiagGroups,
                std::map<std::string, GroupInfo> &DiagsInGroup)
  : DiagGroupParents(DiagGroupParents),
  Diags(Diags),
  DiagGroups(DiagGroups),
  DiagsInGroup(DiagsInGroup) {}

  /// Compute the set of diagnostics and groups that are immediately
  /// in -Wpedantic.
  void compute(VecOrSet DiagsInPedantic,
               VecOrSet GroupsInPedantic);

private:
  /// Determine whether a group is a subgroup of another group.
  bool isSubGroupOfGroup(const Record *Group,
                         llvm::StringRef RootGroupName);

  /// Determine if the diagnostic is an extension.
  bool isExtension(const Record *Diag);

  /// Determine if the diagnostic is off by default.
  bool isOffByDefault(const Record *Diag);

  /// Increment the count for a group, and transitively marked
  /// parent groups when appropriate.
  void markGroup(const Record *Group);

  /// Return true if the diagnostic is in a pedantic group.
  bool groupInPedantic(const Record *Group, bool increment = false);
};
} // end anonymous namespace

bool InferPedantic::isSubGroupOfGroup(const Record *Group,
                                      llvm::StringRef GName) {

  const std::string &GroupName = Group->getValueAsString("GroupName");
  if (GName == GroupName)
    return true;

  const std::vector<Record*> &Parents = DiagGroupParents.getParents(Group);
  for (unsigned i = 0, e = Parents.size(); i != e; ++i)
    if (isSubGroupOfGroup(Parents[i], GName))
      return true;

  return false;
}

/// Determine if the diagnostic is an extension.
bool InferPedantic::isExtension(const Record *Diag) {
  const std::string &ClsName = Diag->getValueAsDef("Class")->getName();
  return ClsName == "CLASS_EXTENSION";
}

bool InferPedantic::isOffByDefault(const Record *Diag) {
  const std::string &DefMap = Diag->getValueAsDef("DefaultMapping")->getName();
  return DefMap == "MAP_IGNORE";
}

bool InferPedantic::groupInPedantic(const Record *Group, bool increment) {
  GMap::mapped_type &V = GroupCount[Group];
  // Lazily compute the threshold value for the group count.
  if (!V.second.hasValue()) {
    const GroupInfo &GI = DiagsInGroup[Group->getValueAsString("GroupName")];
    V.second = GI.SubGroups.size() + GI.DiagsInGroup.size();
  }

  if (increment)
    ++V.first;

  // Consider a group in -Wpendatic IFF if has at least one diagnostic
  // or subgroup AND all of those diagnostics and subgroups are covered
  // by -Wpedantic via our computation.
  return V.first != 0 && V.first == V.second.getValue();
}

void InferPedantic::markGroup(const Record *Group) {
  // If all the diagnostics and subgroups have been marked as being
  // covered by -Wpedantic, increment the count of parent groups.  Once the
  // group's count is equal to the number of subgroups and diagnostics in
  // that group, we can safely add this group to -Wpedantic.
  if (groupInPedantic(Group, /* increment */ true)) {
    const std::vector<Record*> &Parents = DiagGroupParents.getParents(Group);
    for (unsigned i = 0, e = Parents.size(); i != e; ++i)
      markGroup(Parents[i]);
  }
}

void InferPedantic::compute(VecOrSet DiagsInPedantic,
                            VecOrSet GroupsInPedantic) {
  // All extensions that are not on by default are implicitly in the
  // "pedantic" group.  For those that aren't explicitly included in -Wpedantic,
  // mark them for consideration to be included in -Wpedantic directly.
  for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
    Record *R = Diags[i];
    if (isExtension(R) && isOffByDefault(R)) {
      DiagsSet.insert(R);
      if (DefInit *Group = dyn_cast<DefInit>(R->getValueInit("Group"))) {
        const Record *GroupRec = Group->getDef();
        if (!isSubGroupOfGroup(GroupRec, "pedantic")) {
          markGroup(GroupRec);
        }
      }
    }
  }

  // Compute the set of diagnostics that are directly in -Wpedantic.  We
  // march through Diags a second time to ensure the results are emitted
  // in deterministic order.
  for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
    Record *R = Diags[i];
    if (!DiagsSet.count(R))
      continue;
    // Check if the group is implicitly in -Wpedantic.  If so,
    // the diagnostic should not be directly included in the -Wpedantic
    // diagnostic group.
    if (DefInit *Group = dyn_cast<DefInit>(R->getValueInit("Group")))
      if (groupInPedantic(Group->getDef()))
        continue;

    // The diagnostic is not included in a group that is (transitively) in
    // -Wpedantic.  Include it in -Wpedantic directly.
    if (RecordVec *V = DiagsInPedantic.dyn_cast<RecordVec*>())
      V->push_back(R);
    else {
      DiagsInPedantic.get<RecordSet*>()->insert(R);
    }
  }

  if (!GroupsInPedantic)
    return;

  // Compute the set of groups that are directly in -Wpedantic.  We
  // march through the groups to ensure the results are emitted
  /// in a deterministc order.
  for (unsigned i = 0, ei = DiagGroups.size(); i != ei; ++i) {
    Record *Group = DiagGroups[i];
    if (!groupInPedantic(Group))
      continue;

    unsigned ParentsInPedantic = 0;
    const std::vector<Record*> &Parents = DiagGroupParents.getParents(Group);
    for (unsigned j = 0, ej = Parents.size(); j != ej; ++j) {
      if (groupInPedantic(Parents[j]))
        ++ParentsInPedantic;
    }
    // If all the parents are in -Wpedantic, this means that this diagnostic
    // group will be indirectly included by -Wpedantic already.  In that
    // case, do not add it directly to -Wpedantic.  If the group has no
    // parents, obviously it should go into -Wpedantic.
    if (Parents.size() > 0 && ParentsInPedantic == Parents.size())
      continue;

    if (RecordVec *V = GroupsInPedantic.dyn_cast<RecordVec*>())
      V->push_back(Group);
    else {
      GroupsInPedantic.get<RecordSet*>()->insert(Group);
    }
  }
}

//===----------------------------------------------------------------------===//
// Warning Tables (.inc file) generation.
//===----------------------------------------------------------------------===//

static bool isError(const Record &Diag) {
  const std::string &ClsName = Diag.getValueAsDef("Class")->getName();
  return ClsName == "CLASS_ERROR";
}

static bool isRemark(const Record &Diag) {
  const std::string &ClsName = Diag.getValueAsDef("Class")->getName();
  return ClsName == "CLASS_REMARK";
}

/// ClangDiagsDefsEmitter - The top-level class emits .def files containing
/// declarations of Clang diagnostics.
namespace clang {
void EmitClangDiagsDefs(RecordKeeper &Records, raw_ostream &OS,
                        const std::string &Component) {
  // Write the #if guard
  if (!Component.empty()) {
    std::string ComponentName = StringRef(Component).upper();
    OS << "#ifdef " << ComponentName << "START\n";
    OS << "__" << ComponentName << "START = DIAG_START_" << ComponentName
       << ",\n";
    OS << "#undef " << ComponentName << "START\n";
    OS << "#endif\n\n";
  }

  const std::vector<Record*> &Diags =
    Records.getAllDerivedDefinitions("Diagnostic");

  std::vector<Record*> DiagGroups
    = Records.getAllDerivedDefinitions("DiagGroup");

  std::map<std::string, GroupInfo> DiagsInGroup;
  groupDiagnostics(Diags, DiagGroups, DiagsInGroup);

  DiagCategoryIDMap CategoryIDs(Records);
  DiagGroupParentMap DGParentMap(Records);

  // Compute the set of diagnostics that are in -Wpedantic.
  RecordSet DiagsInPedantic;
  InferPedantic inferPedantic(DGParentMap, Diags, DiagGroups, DiagsInGroup);
  inferPedantic.compute(&DiagsInPedantic, (RecordVec*)0);

  for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
    const Record &R = *Diags[i];

    // Check if this is an error that is accidentally in a warning
    // group.
    if (isError(R)) {
      if (DefInit *Group = dyn_cast<DefInit>(R.getValueInit("Group"))) {
        const Record *GroupRec = Group->getDef();
        const std::string &GroupName = GroupRec->getValueAsString("GroupName");
        PrintFatalError(R.getLoc(), "Error " + R.getName() +
                      " cannot be in a warning group [" + GroupName + "]");
      }
    }

    // Check that all remarks have an associated diagnostic group.
    if (isRemark(R)) {
      if (!isa<DefInit>(R.getValueInit("Group"))) {
        PrintFatalError(R.getLoc(), "Error " + R.getName() +
                                        " not in any diagnostic group");
      }
    }

    // Filter by component.
    if (!Component.empty() && Component != R.getValueAsString("Component"))
      continue;

    OS << "DIAG(" << R.getName() << ", ";
    OS << R.getValueAsDef("Class")->getName();
    OS << ", diag::" << R.getValueAsDef("DefaultMapping")->getName();

    // Description string.
    OS << ", \"";
    OS.write_escaped(R.getValueAsString("Text")) << '"';

    // Warning associated with the diagnostic. This is stored as an index into
    // the alphabetically sorted warning table.
    if (DefInit *DI = dyn_cast<DefInit>(R.getValueInit("Group"))) {
      std::map<std::string, GroupInfo>::iterator I =
          DiagsInGroup.find(DI->getDef()->getValueAsString("GroupName"));
      assert(I != DiagsInGroup.end());
      OS << ", " << I->second.IDNo;
    } else if (DiagsInPedantic.count(&R)) {
      std::map<std::string, GroupInfo>::iterator I =
        DiagsInGroup.find("pedantic");
      assert(I != DiagsInGroup.end() && "pedantic group not defined");
      OS << ", " << I->second.IDNo;
    } else {
      OS << ", 0";
    }

    // SFINAE response.
    OS << ", " << R.getValueAsDef("SFINAE")->getName();

    // Default warning has no Werror bit.
    if (R.getValueAsBit("WarningNoWerror"))
      OS << ", true";
    else
      OS << ", false";

    // Default warning show in system header bit.
    if (R.getValueAsBit("WarningShowInSystemHeader"))
      OS << ", true";
    else
      OS << ", false";

    // Category number.
    OS << ", " << CategoryIDs.getID(getDiagnosticCategory(&R, DGParentMap));
    OS << ")\n";
  }
}
} // end namespace clang

//===----------------------------------------------------------------------===//
// Warning Group Tables generation
//===----------------------------------------------------------------------===//

static std::string getDiagCategoryEnum(llvm::StringRef name) {
  if (name.empty())
    return "DiagCat_None";
  SmallString<256> enumName = llvm::StringRef("DiagCat_");
  for (llvm::StringRef::iterator I = name.begin(), E = name.end(); I != E; ++I)
    enumName += isalnum(*I) ? *I : '_';
  return enumName.str();
}

namespace clang {
void EmitClangDiagGroups(RecordKeeper &Records, raw_ostream &OS) {
  // Compute a mapping from a DiagGroup to all of its parents.
  DiagGroupParentMap DGParentMap(Records);

  std::vector<Record*> Diags =
    Records.getAllDerivedDefinitions("Diagnostic");

  std::vector<Record*> DiagGroups
    = Records.getAllDerivedDefinitions("DiagGroup");

  std::map<std::string, GroupInfo> DiagsInGroup;
  groupDiagnostics(Diags, DiagGroups, DiagsInGroup);

  // All extensions are implicitly in the "pedantic" group.  Record the
  // implicit set of groups in the "pedantic" group, and use this information
  // later when emitting the group information for Pedantic.
  RecordVec DiagsInPedantic;
  RecordVec GroupsInPedantic;
  InferPedantic inferPedantic(DGParentMap, Diags, DiagGroups, DiagsInGroup);
  inferPedantic.compute(&DiagsInPedantic, &GroupsInPedantic);

  // Walk through the groups emitting an array for each diagnostic of the diags
  // that are mapped to.
  OS << "\n#ifdef GET_DIAG_ARRAYS\n";
  unsigned MaxLen = 0;
  OS << "static const int16_t DiagArrays[] = {\n"
     << "  /* Empty */ -1,\n";
  for (std::map<std::string, GroupInfo>::const_iterator
       I = DiagsInGroup.begin(), E = DiagsInGroup.end(); I != E; ++I) {
    MaxLen = std::max(MaxLen, (unsigned)I->first.size());
    const bool IsPedantic = I->first == "pedantic";

    const std::vector<const Record*> &V = I->second.DiagsInGroup;
    if (!V.empty() || (IsPedantic && !DiagsInPedantic.empty())) {
      OS << "  /* DiagArray" << I->second.IDNo << " */ ";
      for (unsigned i = 0, e = V.size(); i != e; ++i)
        OS << "diag::" << V[i]->getName() << ", ";
      // Emit the diagnostics implicitly in "pedantic".
      if (IsPedantic) {
        for (unsigned i = 0, e = DiagsInPedantic.size(); i != e; ++i)
          OS << "diag::" << DiagsInPedantic[i]->getName() << ", ";
      }
      OS << "-1,\n";
    }
  }
  OS << "};\n\n";

  OS << "static const int16_t DiagSubGroups[] = {\n"
     << "  /* Empty */ -1,\n";
  for (std::map<std::string, GroupInfo>::const_iterator
       I = DiagsInGroup.begin(), E = DiagsInGroup.end(); I != E; ++I) {
    const bool IsPedantic = I->first == "pedantic";

    const std::vector<std::string> &SubGroups = I->second.SubGroups;
    if (!SubGroups.empty() || (IsPedantic && !GroupsInPedantic.empty())) {
      OS << "  /* DiagSubGroup" << I->second.IDNo << " */ ";
      for (unsigned i = 0, e = SubGroups.size(); i != e; ++i) {
        std::map<std::string, GroupInfo>::const_iterator RI =
          DiagsInGroup.find(SubGroups[i]);
        assert(RI != DiagsInGroup.end() && "Referenced without existing?");
        OS << RI->second.IDNo << ", ";
      }
      // Emit the groups implicitly in "pedantic".
      if (IsPedantic) {
        for (unsigned i = 0, e = GroupsInPedantic.size(); i != e; ++i) {
          const std::string &GroupName =
            GroupsInPedantic[i]->getValueAsString("GroupName");
          std::map<std::string, GroupInfo>::const_iterator RI =
            DiagsInGroup.find(GroupName);
          assert(RI != DiagsInGroup.end() && "Referenced without existing?");
          OS << RI->second.IDNo << ", ";
        }
      }

      OS << "-1,\n";
    }
  }
  OS << "};\n\n";

  StringToOffsetTable GroupNames;
  for (std::map<std::string, GroupInfo>::const_iterator
         I = DiagsInGroup.begin(), E = DiagsInGroup.end(); I != E; ++I) {
    // Store a pascal-style length byte at the beginning of the string.
    std::string Name = char(I->first.size()) + I->first;
    GroupNames.GetOrAddStringOffset(Name, false);
  }

  OS << "static const char DiagGroupNames[] = {\n";
  GroupNames.EmitString(OS);
  OS << "};\n\n";

  OS << "#endif // GET_DIAG_ARRAYS\n\n";

  // Emit the table now.
  OS << "\n#ifdef GET_DIAG_TABLE\n";
  unsigned SubGroupIndex = 1, DiagArrayIndex = 1;
  for (std::map<std::string, GroupInfo>::const_iterator
       I = DiagsInGroup.begin(), E = DiagsInGroup.end(); I != E; ++I) {
    // Group option string.
    OS << "  { /* ";
    if (I->first.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "0123456789!@#$%^*-+=:?")!=std::string::npos)
      PrintFatalError("Invalid character in diagnostic group '" +
                      I->first + "'");
    OS << I->first << " */ " << std::string(MaxLen-I->first.size(), ' ');
    // Store a pascal-style length byte at the beginning of the string.
    std::string Name = char(I->first.size()) + I->first;
    OS << GroupNames.GetOrAddStringOffset(Name, false) << ", ";

    // Special handling for 'pedantic'.
    const bool IsPedantic = I->first == "pedantic";

    // Diagnostics in the group.
    const std::vector<const Record*> &V = I->second.DiagsInGroup;
    const bool hasDiags = !V.empty() ||
                          (IsPedantic && !DiagsInPedantic.empty());
    if (hasDiags) {
      OS << "/* DiagArray" << I->second.IDNo << " */ "
         << DiagArrayIndex << ", ";
      if (IsPedantic)
        DiagArrayIndex += DiagsInPedantic.size();
      DiagArrayIndex += V.size() + 1;
    } else {
      OS << "/* Empty */     0, ";
    }

    // Subgroups.
    const std::vector<std::string> &SubGroups = I->second.SubGroups;
    const bool hasSubGroups = !SubGroups.empty() ||
                              (IsPedantic && !GroupsInPedantic.empty());
    if (hasSubGroups) {
      OS << "/* DiagSubGroup" << I->second.IDNo << " */ " << SubGroupIndex;
      if (IsPedantic)
        SubGroupIndex += GroupsInPedantic.size();
      SubGroupIndex += SubGroups.size() + 1;
    } else {
      OS << "/* Empty */         0";
    }
    OS << " },\n";
  }
  OS << "#endif // GET_DIAG_TABLE\n\n";

  // Emit the category table next.
  DiagCategoryIDMap CategoriesByID(Records);
  OS << "\n#ifdef GET_CATEGORY_TABLE\n";
  for (DiagCategoryIDMap::const_iterator I = CategoriesByID.begin(),
       E = CategoriesByID.end(); I != E; ++I)
    OS << "CATEGORY(\"" << *I << "\", " << getDiagCategoryEnum(*I) << ")\n";
  OS << "#endif // GET_CATEGORY_TABLE\n\n";
}
} // end namespace clang

//===----------------------------------------------------------------------===//
// Diagnostic name index generation
//===----------------------------------------------------------------------===//

namespace {
struct RecordIndexElement
{
  RecordIndexElement() {}
  explicit RecordIndexElement(Record const &R):
    Name(R.getName()) {}

  std::string Name;
};

struct RecordIndexElementSorter :
  public std::binary_function<RecordIndexElement, RecordIndexElement, bool> {

  bool operator()(RecordIndexElement const &Lhs,
                  RecordIndexElement const &Rhs) const {
    return Lhs.Name < Rhs.Name;
  }

};

} // end anonymous namespace.

namespace clang {
void EmitClangDiagsIndexName(RecordKeeper &Records, raw_ostream &OS) {
  const std::vector<Record*> &Diags =
    Records.getAllDerivedDefinitions("Diagnostic");

  std::vector<RecordIndexElement> Index;
  Index.reserve(Diags.size());
  for (unsigned i = 0, e = Diags.size(); i != e; ++i) {
    const Record &R = *(Diags[i]);
    Index.push_back(RecordIndexElement(R));
  }

  std::sort(Index.begin(), Index.end(), RecordIndexElementSorter());

  for (unsigned i = 0, e = Index.size(); i != e; ++i) {
    const RecordIndexElement &R = Index[i];

    OS << "DIAG_NAME_INDEX(" << R.Name << ")\n";
  }
}
} // end namespace clang
