//===- llvm/unittest/IR/AttributesTest.cpp - Attributes unit tests --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Attributes.h"
#include "llvm/IR/LLVMContext.h"
#include "gtest/gtest.h"
using namespace llvm;

namespace {

TEST(Attributes, Uniquing) {
  LLVMContext C;

  Attribute AttrA = Attribute::get(C, Attribute::AlwaysInline);
  Attribute AttrB = Attribute::get(C, Attribute::AlwaysInline);
  EXPECT_EQ(AttrA, AttrB);

  AttributeSet ASs[] = {
    AttributeSet::get(C, 1, Attribute::ZExt),
    AttributeSet::get(C, 2, Attribute::SExt)
  };

  AttributeSet SetA = AttributeSet::get(C, ASs);
  AttributeSet SetB = AttributeSet::get(C, ASs);
  EXPECT_EQ(SetA, SetB);
}

TEST(Attributes, Ordering) {
  LLVMContext C;

  Attribute Align4 = Attribute::get(C, Attribute::Alignment, 4);
  Attribute Align5 = Attribute::get(C, Attribute::Alignment, 5);
  Attribute Deref4 = Attribute::get(C, Attribute::Dereferenceable, 4);
  Attribute Deref5 = Attribute::get(C, Attribute::Dereferenceable, 5);
  EXPECT_TRUE(Align4 < Align5);
  EXPECT_TRUE(Align4 < Deref4);
  EXPECT_TRUE(Align4 < Deref5);
  EXPECT_TRUE(Align5 < Deref4);

  AttributeSet ASs[] = {
    AttributeSet::get(C, 2, Attribute::ZExt),
    AttributeSet::get(C, 1, Attribute::SExt)
  };

  AttributeSet SetA = AttributeSet::get(C, ASs);
  AttributeSet SetB = SetA.removeAttributes(C, 1, ASs[1]);
  EXPECT_NE(SetA, SetB);
}

} // end anonymous namespace
