//===-- tsan_ignoreset.h ----------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of ThreadSanitizer (TSan), a race detector.
//
// IgnoreSet holds a set of stack traces where ignores were enabled.
//===----------------------------------------------------------------------===//
#ifndef TSAN_IGNORESET_H
#define TSAN_IGNORESET_H

#include "tsan_defs.h"

namespace __tsan {

class IgnoreSet {
 public:
  IgnoreSet();
  void Add(StackID stack_id);
  void Reset() { size_ = 0; }
  uptr Size() const { return size_; }
  StackID At(uptr i) const;

 private:
  static constexpr uptr kMaxSize = 16;
  uptr size_;
  StackID stacks_[kMaxSize];
};

}  // namespace __tsan

#endif  // TSAN_IGNORESET_H
