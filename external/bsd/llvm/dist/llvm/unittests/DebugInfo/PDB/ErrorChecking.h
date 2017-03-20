//===- ErrorChecking.h - Helpers for verifying llvm::Errors -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_UNITTESTS_DEBUGINFO_PDB_ERRORCHECKING_H
#define LLVM_UNITTESTS_DEBUGINFO_PDB_ERRORCHECKING_H

#define EXPECT_NO_ERROR(Err)                                                   \
  {                                                                            \
    auto E = Err;                                                              \
    EXPECT_FALSE(static_cast<bool>(E));                                        \
    if (E)                                                                     \
      consumeError(std::move(E));                                              \
  }

#define EXPECT_ERROR(Err)                                                      \
  {                                                                            \
    auto E = Err;                                                              \
    EXPECT_TRUE(static_cast<bool>(E));                                         \
    if (E)                                                                     \
      consumeError(std::move(E));                                              \
  }

#define EXPECT_EXPECTED(Exp)                                                   \
  {                                                                            \
    auto E = Exp.takeError();                                                  \
    EXPECT_FALSE(static_cast<bool>(E));                                        \
    if (E) {                                                                   \
      consumeError(std::move(E));                                              \
      return;                                                                  \
    }                                                                          \
  }

#define EXPECT_UNEXPECTED(Exp)                                                 \
  {                                                                            \
    auto E = Exp.takeError();                                                  \
    EXPECT_TRUE(static_cast<bool>(E));                                         \
    if (E) {                                                                   \
      consumeError(std::move(E));                                              \
      return;                                                                  \
    }                                                                          \
  }

#endif
