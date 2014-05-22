//===--- CC1AsOptions.cpp - Clang Assembler Options Table -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Driver/CC1AsOptions.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"
using namespace clang;
using namespace clang::driver;
using namespace llvm::opt;
using namespace clang::driver::cc1asoptions;

#define PREFIX(NAME, VALUE) static const char *const NAME[] = VALUE;
#include "clang/Driver/CC1AsOptions.inc"
#undef PREFIX

static const OptTable::Info CC1AsInfoTable[] = {
#define OPTION(PREFIX, NAME, ID, KIND, GROUP, ALIAS, ALIASARGS, FLAGS, PARAM, \
               HELPTEXT, METAVAR)   \
  { PREFIX, NAME, HELPTEXT, METAVAR, OPT_##ID, Option::KIND##Class, PARAM, \
    FLAGS, OPT_##GROUP, OPT_##ALIAS, ALIASARGS },
#include "clang/Driver/CC1AsOptions.inc"
#undef OPTION
};

namespace {

class CC1AsOptTable : public OptTable {
public:
  CC1AsOptTable()
    : OptTable(CC1AsInfoTable, llvm::array_lengthof(CC1AsInfoTable)) {}
};

}

OptTable *clang::driver::createCC1AsOptTable() {
  return new CC1AsOptTable();
}
