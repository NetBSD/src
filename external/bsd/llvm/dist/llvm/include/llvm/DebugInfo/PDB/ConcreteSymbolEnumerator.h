//===- ConcreteSymbolEnumerator.h -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_DEBUGINFO_PDB_CONCRETESYMBOLENUMERATOR_H
#define LLVM_DEBUGINFO_PDB_CONCRETESYMBOLENUMERATOR_H

#include "llvm/DebugInfo/PDB/IPDBEnumChildren.h"
#include "llvm/DebugInfo/PDB/PDBTypes.h"
#include "llvm/Support/Casting.h"
#include <algorithm>
#include <cstdint>
#include <memory>

namespace llvm {
namespace pdb {

template <typename ChildType>
class ConcreteSymbolEnumerator : public IPDBEnumChildren<ChildType> {
public:
  ConcreteSymbolEnumerator(std::unique_ptr<IPDBEnumSymbols> SymbolEnumerator)
      : Enumerator(std::move(SymbolEnumerator)) {}

  ~ConcreteSymbolEnumerator() override = default;

  uint32_t getChildCount() const override {
    return Enumerator->getChildCount();
  }

  std::unique_ptr<ChildType> getChildAtIndex(uint32_t Index) const override {
    std::unique_ptr<PDBSymbol> Child = Enumerator->getChildAtIndex(Index);
    return make_concrete_child(std::move(Child));
  }

  std::unique_ptr<ChildType> getNext() override {
    std::unique_ptr<PDBSymbol> Child = Enumerator->getNext();
    return make_concrete_child(std::move(Child));
  }

  void reset() override { Enumerator->reset(); }

  ConcreteSymbolEnumerator<ChildType> *clone() const override {
    std::unique_ptr<IPDBEnumSymbols> WrappedClone(Enumerator->clone());
    return new ConcreteSymbolEnumerator<ChildType>(std::move(WrappedClone));
  }

private:
  std::unique_ptr<ChildType>
  make_concrete_child(std::unique_ptr<PDBSymbol> Child) const {
    ChildType *ConcreteChild = dyn_cast_or_null<ChildType>(Child.release());
    return std::unique_ptr<ChildType>(ConcreteChild);
  }

  std::unique_ptr<IPDBEnumSymbols> Enumerator;
};

} // end namespace pdb
} // end namespace llvm

#endif // LLVM_DEBUGINFO_PDB_CONCRETESYMBOLENUMERATOR_H
