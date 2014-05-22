//===--- CC1AsOptions.h - Clang Assembler Options Table ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef CLANG_DRIVER_CC1ASOPTIONS_H
#define CLANG_DRIVER_CC1ASOPTIONS_H

namespace llvm {
namespace opt {
  class OptTable;
}
}

namespace clang {
namespace driver {

namespace cc1asoptions {
  enum ID {
    OPT_INVALID = 0, // This is not an option ID.
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM, \
               HELPTEXT, METAVAR) OPT_##ID,
#include "clang/Driver/CC1AsOptions.inc"
    LastOption
#undef OPTION
  };
}

llvm::opt::OptTable *createCC1AsOptTable();
}
}

#endif
