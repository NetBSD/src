//===- llvm/ADT/ilist_iterator.h - Intrusive List Iterator -------*- C++ -*-==//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_ILIST_ITERATOR_H
#define LLVM_ADT_ILIST_ITERATOR_H

#include "llvm/ADT/ilist_node.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace llvm {

namespace ilist_detail {

/// Find const-correct node types.
template <class OptionsT, bool IsConst> struct IteratorTraits;
template <class OptionsT> struct IteratorTraits<OptionsT, false> {
  typedef typename OptionsT::value_type value_type;
  typedef typename OptionsT::pointer pointer;
  typedef typename OptionsT::reference reference;
  typedef ilist_node_impl<OptionsT> *node_pointer;
  typedef ilist_node_impl<OptionsT> &node_reference;
};
template <class OptionsT> struct IteratorTraits<OptionsT, true> {
  typedef const typename OptionsT::value_type value_type;
  typedef typename OptionsT::const_pointer pointer;
  typedef typename OptionsT::const_reference reference;
  typedef const ilist_node_impl<OptionsT> *node_pointer;
  typedef const ilist_node_impl<OptionsT> &node_reference;
};

template <bool IsReverse> struct IteratorHelper;
template <> struct IteratorHelper<false> : ilist_detail::NodeAccess {
  typedef ilist_detail::NodeAccess Access;
  template <class T> static void increment(T *&I) { I = Access::getNext(*I); }
  template <class T> static void decrement(T *&I) { I = Access::getPrev(*I); }
};
template <> struct IteratorHelper<true> : ilist_detail::NodeAccess {
  typedef ilist_detail::NodeAccess Access;
  template <class T> static void increment(T *&I) { I = Access::getPrev(*I); }
  template <class T> static void decrement(T *&I) { I = Access::getNext(*I); }
};

} // end namespace ilist_detail

/// Iterator for intrusive lists  based on ilist_node.
template <class OptionsT, bool IsReverse, bool IsConst>
class ilist_iterator : ilist_detail::SpecificNodeAccess<OptionsT> {
  friend ilist_iterator<OptionsT, IsReverse, !IsConst>;
  friend ilist_iterator<OptionsT, !IsReverse, IsConst>;
  friend ilist_iterator<OptionsT, !IsReverse, !IsConst>;

  typedef ilist_detail::IteratorTraits<OptionsT, IsConst> Traits;
  typedef ilist_detail::SpecificNodeAccess<OptionsT> Access;

public:
  typedef typename Traits::value_type value_type;
  typedef typename Traits::pointer pointer;
  typedef typename Traits::reference reference;
  typedef ptrdiff_t difference_type;
  typedef std::bidirectional_iterator_tag iterator_category;

  typedef typename OptionsT::const_pointer const_pointer;
  typedef typename OptionsT::const_reference const_reference;

private:
  typedef typename Traits::node_pointer node_pointer;
  typedef typename Traits::node_reference node_reference;

  node_pointer NodePtr;

public:
  /// Create from an ilist_node.
  explicit ilist_iterator(node_reference N) : NodePtr(&N) {}

  explicit ilist_iterator(pointer NP) : NodePtr(Access::getNodePtr(NP)) {}
  explicit ilist_iterator(reference NR) : NodePtr(Access::getNodePtr(&NR)) {}
  ilist_iterator() : NodePtr(nullptr) {}

  // This is templated so that we can allow constructing a const iterator from
  // a nonconst iterator...
  template <bool RHSIsConst>
  ilist_iterator(
      const ilist_iterator<OptionsT, IsReverse, RHSIsConst> &RHS,
      typename std::enable_if<IsConst || !RHSIsConst, void *>::type = nullptr)
      : NodePtr(RHS.NodePtr) {}

  // This is templated so that we can allow assigning to a const iterator from
  // a nonconst iterator...
  template <bool RHSIsConst>
  typename std::enable_if<IsConst || !RHSIsConst, ilist_iterator &>::type
  operator=(const ilist_iterator<OptionsT, IsReverse, RHSIsConst> &RHS) {
    NodePtr = RHS.NodePtr;
    return *this;
  }

  /// Convert from an iterator to its reverse.
  ///
  /// TODO: Roll this into the implicit constructor once we're sure that no one
  /// is relying on the std::reverse_iterator off-by-one semantics.
  ilist_iterator<OptionsT, !IsReverse, IsConst> getReverse() const {
    if (NodePtr)
      return ilist_iterator<OptionsT, !IsReverse, IsConst>(*NodePtr);
    return ilist_iterator<OptionsT, !IsReverse, IsConst>();
  }

  /// Const-cast.
  ilist_iterator<OptionsT, IsReverse, false> getNonConst() const {
    if (NodePtr)
      return ilist_iterator<OptionsT, IsReverse, false>(
          const_cast<typename ilist_iterator<OptionsT, IsReverse,
                                             false>::node_reference>(*NodePtr));
    return ilist_iterator<OptionsT, IsReverse, false>();
  }

  // Accessors...
  reference operator*() const {
    assert(!NodePtr->isKnownSentinel());
    return *Access::getValuePtr(NodePtr);
  }
  pointer operator->() const { return &operator*(); }

  // Comparison operators
  friend bool operator==(const ilist_iterator &LHS, const ilist_iterator &RHS) {
    return LHS.NodePtr == RHS.NodePtr;
  }
  friend bool operator!=(const ilist_iterator &LHS, const ilist_iterator &RHS) {
    return LHS.NodePtr != RHS.NodePtr;
  }

  // Increment and decrement operators...
  ilist_iterator &operator--() {
    NodePtr = IsReverse ? NodePtr->getNext() : NodePtr->getPrev();
    return *this;
  }
  ilist_iterator &operator++() {
    NodePtr = IsReverse ? NodePtr->getPrev() : NodePtr->getNext();
    return *this;
  }
  ilist_iterator operator--(int) {
    ilist_iterator tmp = *this;
    --*this;
    return tmp;
  }
  ilist_iterator operator++(int) {
    ilist_iterator tmp = *this;
    ++*this;
    return tmp;
  }

  /// Get the underlying ilist_node.
  node_pointer getNodePtr() const { return static_cast<node_pointer>(NodePtr); }

  /// Check for end.  Only valid if ilist_sentinel_tracking<true>.
  bool isEnd() const { return NodePtr ? NodePtr->isSentinel() : false; }
};

template <typename From> struct simplify_type;

/// Allow ilist_iterators to convert into pointers to a node automatically when
/// used by the dyn_cast, cast, isa mechanisms...
///
/// FIXME: remove this, since there is no implicit conversion to NodeTy.
template <class OptionsT, bool IsConst>
struct simplify_type<ilist_iterator<OptionsT, false, IsConst>> {
  typedef ilist_iterator<OptionsT, false, IsConst> iterator;
  typedef typename iterator::pointer SimpleType;

  static SimpleType getSimplifiedValue(const iterator &Node) { return &*Node; }
};
template <class OptionsT, bool IsConst>
struct simplify_type<const ilist_iterator<OptionsT, false, IsConst>>
    : simplify_type<ilist_iterator<OptionsT, false, IsConst>> {};

} // end namespace llvm

#endif // LLVM_ADT_ILIST_ITERATOR_H
