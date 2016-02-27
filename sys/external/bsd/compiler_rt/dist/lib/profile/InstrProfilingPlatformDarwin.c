/*===- InstrProfilingPlatformDarwin.c - Profile data on Darwin ------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|*
\*===----------------------------------------------------------------------===*/

#include "InstrProfiling.h"

#if defined(__APPLE__)
/* Use linker magic to find the bounds of the Data section. */
COMPILER_RT_VISIBILITY
extern __llvm_profile_data
    DataStart __asm("section$start$__DATA$" INSTR_PROF_DATA_SECT_NAME_STR);
COMPILER_RT_VISIBILITY
extern __llvm_profile_data
    DataEnd __asm("section$end$__DATA$" INSTR_PROF_DATA_SECT_NAME_STR);
COMPILER_RT_VISIBILITY
extern char
    NamesStart __asm("section$start$__DATA$" INSTR_PROF_NAME_SECT_NAME_STR);
COMPILER_RT_VISIBILITY
extern char NamesEnd __asm("section$end$__DATA$" INSTR_PROF_NAME_SECT_NAME_STR);
COMPILER_RT_VISIBILITY
extern uint64_t
    CountersStart __asm("section$start$__DATA$" INSTR_PROF_CNTS_SECT_NAME_STR);
COMPILER_RT_VISIBILITY
extern uint64_t
    CountersEnd __asm("section$end$__DATA$" INSTR_PROF_CNTS_SECT_NAME_STR);

COMPILER_RT_VISIBILITY
const __llvm_profile_data *__llvm_profile_begin_data(void) {
  return &DataStart;
}
COMPILER_RT_VISIBILITY
const __llvm_profile_data *__llvm_profile_end_data(void) { return &DataEnd; }
COMPILER_RT_VISIBILITY
const char *__llvm_profile_begin_names(void) { return &NamesStart; }
COMPILER_RT_VISIBILITY
const char *__llvm_profile_end_names(void) { return &NamesEnd; }
COMPILER_RT_VISIBILITY
uint64_t *__llvm_profile_begin_counters(void) { return &CountersStart; }
COMPILER_RT_VISIBILITY
uint64_t *__llvm_profile_end_counters(void) { return &CountersEnd; }
#endif
