//===- llvm/unittest/ADT/ArrayRefTest.cpp - ArrayRef unit tests -----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"
using namespace llvm;

namespace llvm {

TEST(ArrayRefTest, AllocatorCopy) {
  BumpPtrAllocator Alloc;
  static const uint16_t Words1[] = { 1, 4, 200, 37 };
  ArrayRef<uint16_t> Array1 = makeArrayRef(Words1, 4);
  static const uint16_t Words2[] = { 11, 4003, 67, 64000, 13 };
  ArrayRef<uint16_t> Array2 = makeArrayRef(Words2, 5);
  ArrayRef<uint16_t> Array1c = Array1.copy(Alloc);
  ArrayRef<uint16_t> Array2c = Array2.copy(Alloc);;
  EXPECT_TRUE(Array1.equals(Array1c));
  EXPECT_NE(Array1.data(), Array1c.data());
  EXPECT_TRUE(Array2.equals(Array2c));
  EXPECT_NE(Array2.data(), Array2c.data());
}


} // end anonymous namespace
