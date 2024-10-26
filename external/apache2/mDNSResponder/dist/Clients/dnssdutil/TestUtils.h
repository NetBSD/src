//
//  TestUtils.h
//  mDNSResponder
//
//  Copyright (c) 2019 Apple Inc. All rights reserved.
//

#ifndef    __TestUtils_h
#define    __TestUtils_h

#include <TargetConditionals.h>
#include <MacTypes.h>
#include <mach/mach.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DNSSDUTIL_XCTEST "DNSSDUTIL_XCTEST"

bool run_xctest_named(const char *classname);
bool audit_token_for_pid(pid_t pid, const audit_token_t *token);

#ifdef __cplusplus
}
#endif

#endif // __TestUtils_h
