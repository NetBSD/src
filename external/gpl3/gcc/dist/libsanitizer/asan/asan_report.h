//===-- asan_report.h -------------------------------------------*- C++ -*-===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file is a part of AddressSanitizer, an address sanity checker.
//
// ASan-private header for error reporting functions.
//===----------------------------------------------------------------------===//

#include "asan_allocator.h"
#include "asan_internal.h"
#include "asan_thread.h"

namespace __asan {

// The following functions prints address description depending
// on the memory type (shadow/heap/stack/global).
void DescribeHeapAddress(uptr addr, uptr access_size);
bool DescribeAddressIfGlobal(uptr addr, uptr access_size);
bool DescribeAddressRelativeToGlobal(uptr addr, uptr access_size,
                                     const __asan_global &g);
bool DescribeAddressIfShadow(uptr addr);
bool DescribeAddressIfStack(uptr addr, uptr access_size);
// Determines memory type on its own.
void DescribeAddress(uptr addr, uptr access_size);

void DescribeThread(AsanThreadSummary *summary);

// Different kinds of error reports.
void NORETURN ReportSIGSEGV(uptr pc, uptr sp, uptr bp, uptr addr);
void NORETURN ReportDoubleFree(uptr addr, StackTrace *stack);
void NORETURN ReportFreeNotMalloced(uptr addr, StackTrace *stack);
void NORETURN ReportAllocTypeMismatch(uptr addr, StackTrace *stack,
                                      AllocType alloc_type,
                                      AllocType dealloc_type);
void NORETURN ReportMallocUsableSizeNotOwned(uptr addr,
                                             StackTrace *stack);
void NORETURN ReportAsanGetAllocatedSizeNotOwned(uptr addr,
                                                 StackTrace *stack);
void NORETURN ReportStringFunctionMemoryRangesOverlap(
    const char *function, const char *offset1, uptr length1,
    const char *offset2, uptr length2, StackTrace *stack);

// Mac-specific errors and warnings.
void WarnMacFreeUnallocated(
    uptr addr, uptr zone_ptr, const char *zone_name, StackTrace *stack);
void NORETURN ReportMacMzReallocUnknown(
    uptr addr, uptr zone_ptr, const char *zone_name, StackTrace *stack);
void NORETURN ReportMacCfReallocUnknown(
    uptr addr, uptr zone_ptr, const char *zone_name, StackTrace *stack);

}  // namespace __asan
