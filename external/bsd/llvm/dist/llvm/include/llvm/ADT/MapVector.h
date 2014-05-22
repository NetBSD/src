//===- llvm/ADT/MapVector.h - Map w/ deterministic value order --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a map that provides insertion order iteration. The
// interface is purposefully minimal. The key is assumed to be cheap to copy
// and 2 copies are kept, one for indexing in a DenseMap, one for iteration in
// a std::vector.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_MAPVECTOR_H
#define LLVM_ADT_MAPVECTOR_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include <vector>

namespace llvm {

/// This class implements a map that also provides access to all stored values
/// in a deterministic order. The values are kept in a std::vector and the
/// mapping is done with DenseMap from Keys to indexes in that vector.
template<typename KeyT, typename ValueT,
         typename MapType = llvm::DenseMap<KeyT, unsigned>,
         typename VectorType = std::vector<std::pair<KeyT, ValueT> > >
class MapVector {
  typedef typename VectorType::size_type SizeType;

  MapType Map;
  VectorType Vector;

public:
  typedef typename VectorType::iterator iterator;
  typedef typename VectorType::const_iterator const_iterator;

  SizeType size() const {
    return Vector.size();
  }

  iterator begin() {
    return Vector.begin();
  }

  const_iterator begin() const {
    return Vector.begin();
  }

  iterator end() {
    return Vector.end();
  }

  const_iterator end() const {
    return Vector.end();
  }

  bool empty() const {
    return Vector.empty();
  }

  std::pair<KeyT, ValueT>       &front()       { return Vector.front(); }
  const std::pair<KeyT, ValueT> &front() const { return Vector.front(); }
  std::pair<KeyT, ValueT>       &back()        { return Vector.back(); }
  const std::pair<KeyT, ValueT> &back()  const { return Vector.back(); }

  void clear() {
    Map.clear();
    Vector.clear();
  }

  ValueT &operator[](const KeyT &Key) {
    std::pair<KeyT, unsigned> Pair = std::make_pair(Key, 0);
    std::pair<typename MapType::iterator, bool> Result = Map.insert(Pair);
    unsigned &I = Result.first->second;
    if (Result.second) {
      Vector.push_back(std::make_pair(Key, ValueT()));
      I = Vector.size() - 1;
    }
    return Vector[I].second;
  }

  ValueT lookup(const KeyT &Key) const {
    typename MapType::const_iterator Pos = Map.find(Key);
    return Pos == Map.end()? ValueT() : Vector[Pos->second].second;
  }

  std::pair<iterator, bool> insert(const std::pair<KeyT, ValueT> &KV) {
    std::pair<KeyT, unsigned> Pair = std::make_pair(KV.first, 0);
    std::pair<typename MapType::iterator, bool> Result = Map.insert(Pair);
    unsigned &I = Result.first->second;
    if (Result.second) {
      Vector.push_back(std::make_pair(KV.first, KV.second));
      I = Vector.size() - 1;
      return std::make_pair(llvm::prior(end()), true);
    }
    return std::make_pair(begin() + I, false);
  }

  unsigned count(const KeyT &Key) const {
    typename MapType::const_iterator Pos = Map.find(Key);
    return Pos == Map.end()? 0 : 1;
  }

  iterator find(const KeyT &Key) {
    typename MapType::const_iterator Pos = Map.find(Key);
    return Pos == Map.end()? Vector.end() :
                            (Vector.begin() + Pos->second);
  }

  const_iterator find(const KeyT &Key) const {
    typename MapType::const_iterator Pos = Map.find(Key);
    return Pos == Map.end()? Vector.end() :
                            (Vector.begin() + Pos->second);
  }

  /// \brief Remove the last element from the vector.
  void pop_back() {
    typename MapType::iterator Pos = Map.find(Vector.back().first);
    Map.erase(Pos);
    Vector.pop_back();
  }
};

}

#endif
