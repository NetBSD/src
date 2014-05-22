//===- LineIterator.h - Iterator to read a text buffer's lines --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringRef.h"
#include <iterator>

namespace llvm {

class MemoryBuffer;

/// \brief A forward iterator which reads non-blank text lines from a buffer.
///
/// This class provides a forward iterator interface for reading one line at
/// a time from a buffer. When default constructed the iterator will be the
/// "end" iterator.
///
/// The iterator also is aware of what line number it is currently processing
/// and can strip comment lines given the comment-starting character.
///
/// Note that this iterator requires the buffer to be nul terminated.
class line_iterator
    : public std::iterator<std::forward_iterator_tag, StringRef, ptrdiff_t> {
  const MemoryBuffer *Buffer;
  char CommentMarker;

  unsigned LineNumber;
  StringRef CurrentLine;

public:
  /// \brief Default construct an "end" iterator.
  line_iterator() : Buffer(0) {}

  /// \brief Construct a new iterator around some memory buffer.
  explicit line_iterator(const MemoryBuffer &Buffer, char CommentMarker = '\0');

  /// \brief Return true if we've reached EOF or are an "end" iterator.
  bool is_at_eof() const { return !Buffer; }

  /// \brief Return true if we're an "end" iterator or have reached EOF.
  bool is_at_end() const { return is_at_eof(); }

  /// \brief Return the current line number. May return any number at EOF.
  int64_t line_number() const { return LineNumber; }

  /// \brief Advance to the next (non-empty, non-comment) line.
  line_iterator &operator++() {
    advance();
    return *this;
  }

  /// \brief Get the current line as a \c StringRef.
  StringRef operator*() const { return CurrentLine; }

  friend bool operator==(const line_iterator &LHS, const line_iterator &RHS) {
    return LHS.Buffer == RHS.Buffer &&
           LHS.CurrentLine.begin() == RHS.CurrentLine.begin();
  }

  friend bool operator!=(const line_iterator &LHS, const line_iterator &RHS) {
    return !(LHS == RHS);
  }

private:
  /// \brief Advance the iterator to the next line.
  void advance();
};
}
