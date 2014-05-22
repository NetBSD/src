//===- LazyCallGraph.h - Analysis of a Module's call graph ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
///
/// Implements a lazy call graph analysis and related passes for the new pass
/// manager.
///
/// NB: This is *not* a traditional call graph! It is a graph which models both
/// the current calls and potential calls. As a consequence there are many
/// edges in this call graph that do not correspond to a 'call' or 'invoke'
/// instruction.
///
/// The primary use cases of this graph analysis is to facilitate iterating
/// across the functions of a module in ways that ensure all callees are
/// visited prior to a caller (given any SCC constraints), or vice versa. As
/// such is it particularly well suited to organizing CGSCC optimizations such
/// as inlining, outlining, argument promotion, etc. That is its primary use
/// case and motivates the design. It may not be appropriate for other
/// purposes. The use graph of functions or some other conservative analysis of
/// call instructions may be interesting for optimizations and subsequent
/// analyses which don't work in the context of an overly specified
/// potential-call-edge graph.
///
/// To understand the specific rules and nature of this call graph analysis,
/// see the documentation of the \c LazyCallGraph below.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_LAZY_CALL_GRAPH
#define LLVM_ANALYSIS_LAZY_CALL_GRAPH

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/Allocator.h"
#include <iterator>

namespace llvm {
class ModuleAnalysisManager;
class PreservedAnalyses;
class raw_ostream;

/// \brief A lazily constructed view of the call graph of a module.
///
/// With the edges of this graph, the motivating constraint that we are
/// attempting to maintain is that function-local optimization, CGSCC-local
/// optimizations, and optimizations transforming a pair of functions connected
/// by an edge in the graph, do not invalidate a bottom-up traversal of the SCC
/// DAG. That is, no optimizations will delete, remove, or add an edge such
/// that functions already visited in a bottom-up order of the SCC DAG are no
/// longer valid to have visited, or such that functions not yet visited in
/// a bottom-up order of the SCC DAG are not required to have already been
/// visited.
///
/// Within this constraint, the desire is to minimize the merge points of the
/// SCC DAG. The greater the fanout of the SCC DAG and the fewer merge points
/// in the SCC DAG, the more independence there is in optimizing within it.
/// There is a strong desire to enable parallelization of optimizations over
/// the call graph, and both limited fanout and merge points will (artificially
/// in some cases) limit the scaling of such an effort.
///
/// To this end, graph represents both direct and any potential resolution to
/// an indirect call edge. Another way to think about it is that it represents
/// both the direct call edges and any direct call edges that might be formed
/// through static optimizations. Specifically, it considers taking the address
/// of a function to be an edge in the call graph because this might be
/// forwarded to become a direct call by some subsequent function-local
/// optimization. The result is that the graph closely follows the use-def
/// edges for functions. Walking "up" the graph can be done by looking at all
/// of the uses of a function.
///
/// The roots of the call graph are the external functions and functions
/// escaped into global variables. Those functions can be called from outside
/// of the module or via unknowable means in the IR -- we may not be able to
/// form even a potential call edge from a function body which may dynamically
/// load the function and call it.
///
/// This analysis still requires updates to remain valid after optimizations
/// which could potentially change the set of potential callees. The
/// constraints it operates under only make the traversal order remain valid.
///
/// The entire analysis must be re-computed if full interprocedural
/// optimizations run at any point. For example, globalopt completely
/// invalidates the information in this analysis.
///
/// FIXME: This class is named LazyCallGraph in a lame attempt to distinguish
/// it from the existing CallGraph. At some point, it is expected that this
/// will be the only call graph and it will be renamed accordingly.
class LazyCallGraph {
public:
  class Node;
  typedef SmallVector<PointerUnion<Function *, Node *>, 4> NodeVectorT;
  typedef SmallVectorImpl<PointerUnion<Function *, Node *> > NodeVectorImplT;

  /// \brief A lazy iterator used for both the entry nodes and child nodes.
  ///
  /// When this iterator is dereferenced, if not yet available, a function will
  /// be scanned for "calls" or uses of functions and its child information
  /// will be constructed. All of these results are accumulated and cached in
  /// the graph.
  class iterator : public std::iterator<std::bidirectional_iterator_tag, Node *,
                                        ptrdiff_t, Node *, Node *> {
    friend class LazyCallGraph;
    friend class LazyCallGraph::Node;
    typedef std::iterator<std::bidirectional_iterator_tag, Node *, ptrdiff_t,
                          Node *, Node *> BaseT;

    /// \brief Nonce type to select the constructor for the end iterator.
    struct IsAtEndT {};

    LazyCallGraph &G;
    NodeVectorImplT::iterator NI;

    // Build the begin iterator for a node.
    explicit iterator(LazyCallGraph &G, NodeVectorImplT &Nodes)
        : G(G), NI(Nodes.begin()) {}

    // Build the end iterator for a node. This is selected purely by overload.
    iterator(LazyCallGraph &G, NodeVectorImplT &Nodes, IsAtEndT /*Nonce*/)
        : G(G), NI(Nodes.end()) {}

  public:
    iterator(const iterator &Arg) : G(Arg.G), NI(Arg.NI) {}

    iterator &operator=(iterator Arg) {
      std::swap(Arg, *this);
      return *this;
    }

    bool operator==(const iterator &Arg) { return NI == Arg.NI; }
    bool operator!=(const iterator &Arg) { return !operator==(Arg); }

    reference operator*() const {
      if (NI->is<Node *>())
        return NI->get<Node *>();

      Function *F = NI->get<Function *>();
      Node *ChildN = G.get(*F);
      *NI = ChildN;
      return ChildN;
    }
    pointer operator->() const { return operator*(); }

    iterator &operator++() {
      ++NI;
      return *this;
    }
    iterator operator++(int) {
      iterator prev = *this;
      ++*this;
      return prev;
    }

    iterator &operator--() {
      --NI;
      return *this;
    }
    iterator operator--(int) {
      iterator next = *this;
      --*this;
      return next;
    }
  };

  /// \brief Construct a graph for the given module.
  ///
  /// This sets up the graph and computes all of the entry points of the graph.
  /// No function definitions are scanned until their nodes in the graph are
  /// requested during traversal.
  LazyCallGraph(Module &M);

  /// \brief Copy constructor.
  ///
  /// This does a deep copy of the graph. It does no verification that the
  /// graph remains valid for the module. It is also relatively expensive.
  LazyCallGraph(const LazyCallGraph &G);

#if LLVM_HAS_RVALUE_REFERENCES
  /// \brief Move constructor.
  ///
  /// This is a deep move. It leaves G in an undefined but destroyable state.
  /// Any other operation on G is likely to fail.
  LazyCallGraph(LazyCallGraph &&G);
#endif

  iterator begin() { return iterator(*this, EntryNodes); }
  iterator end() { return iterator(*this, EntryNodes, iterator::IsAtEndT()); }

  /// \brief Lookup a function in the graph which has already been scanned and
  /// added.
  Node *lookup(const Function &F) const { return NodeMap.lookup(&F); }

  /// \brief Get a graph node for a given function, scanning it to populate the
  /// graph data as necessary.
  Node *get(Function &F) {
    Node *&N = NodeMap[&F];
    if (N)
      return N;

    return insertInto(F, N);
  }

private:
  Module &M;

  /// \brief Allocator that holds all the call graph nodes.
  SpecificBumpPtrAllocator<Node> BPA;

  /// \brief Maps function->node for fast lookup.
  DenseMap<const Function *, Node *> NodeMap;

  /// \brief The entry nodes to the graph.
  ///
  /// These nodes are reachable through "external" means. Put another way, they
  /// escape at the module scope.
  NodeVectorT EntryNodes;

  /// \brief Set of the entry nodes to the graph.
  SmallPtrSet<Function *, 4> EntryNodeSet;

  /// \brief Helper to insert a new function, with an already looked-up entry in
  /// the NodeMap.
  Node *insertInto(Function &F, Node *&MappedN);

  /// \brief Helper to copy a node from another graph into this one.
  Node *copyInto(const Node &OtherN);

#if LLVM_HAS_RVALUE_REFERENCES
  /// \brief Helper to move a node from another graph into this one.
  Node *moveInto(Node &&OtherN);
#endif
};

/// \brief A node in the call graph.
///
/// This represents a single node. It's primary roles are to cache the list of
/// callees, de-duplicate and provide fast testing of whether a function is
/// a callee, and facilitate iteration of child nodes in the graph.
class LazyCallGraph::Node {
  friend class LazyCallGraph;

  LazyCallGraph &G;
  Function &F;
  mutable NodeVectorT Callees;
  SmallPtrSet<Function *, 4> CalleeSet;

  /// \brief Basic constructor implements the scanning of F into Callees and
  /// CalleeSet.
  Node(LazyCallGraph &G, Function &F);

  /// \brief Constructor used when copying a node from one graph to another.
  Node(LazyCallGraph &G, const Node &OtherN);

#if LLVM_HAS_RVALUE_REFERENCES
  /// \brief Constructor used when moving a node from one graph to another.
  Node(LazyCallGraph &G, Node &&OtherN);
#endif

public:
  typedef LazyCallGraph::iterator iterator;

  Function &getFunction() const {
    return F;
  };

  iterator begin() const { return iterator(G, Callees); }
  iterator end() const { return iterator(G, Callees, iterator::IsAtEndT()); }

  /// Equality is defined as address equality.
  bool operator==(const Node &N) const { return this == &N; }
  bool operator!=(const Node &N) const { return !operator==(N); }
};

// Provide GraphTraits specializations for call graphs.
template <> struct GraphTraits<LazyCallGraph::Node *> {
  typedef LazyCallGraph::Node NodeType;
  typedef LazyCallGraph::iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};
template <> struct GraphTraits<LazyCallGraph *> {
  typedef LazyCallGraph::Node NodeType;
  typedef LazyCallGraph::iterator ChildIteratorType;

  static NodeType *getEntryNode(NodeType *N) { return N; }
  static ChildIteratorType child_begin(NodeType *N) { return N->begin(); }
  static ChildIteratorType child_end(NodeType *N) { return N->end(); }
};

/// \brief An analysis pass which computes the call graph for a module.
class LazyCallGraphAnalysis {
public:
  /// \brief Inform generic clients of the result type.
  typedef LazyCallGraph Result;

  static void *ID() { return (void *)&PassID; }

  /// \brief Compute the \c LazyCallGraph for a the module \c M.
  ///
  /// This just builds the set of entry points to the call graph. The rest is
  /// built lazily as it is walked.
  LazyCallGraph run(Module *M) { return LazyCallGraph(*M); }

private:
  static char PassID;
};

/// \brief A pass which prints the call graph to a \c raw_ostream.
///
/// This is primarily useful for testing the analysis.
class LazyCallGraphPrinterPass {
  raw_ostream &OS;

public:
  explicit LazyCallGraphPrinterPass(raw_ostream &OS);

  PreservedAnalyses run(Module *M, ModuleAnalysisManager *AM);

  static StringRef name() { return "LazyCallGraphPrinterPass"; }
};

}

#endif
