/*
 * Copyright (c) 2019-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#import "TestUtils.h"

#import <dlfcn.h>
#import <XCTest/XCTest.h>

//===========================================================================================================================
//    Main Test Running
//===========================================================================================================================

bool run_xctest_named(const char *classname)
{
    bool result = false;
	NSString *name = [NSString stringWithUTF8String:classname];
	NSBundle *testBundle = [NSBundle bundleWithPath:@"/AppleInternal/XCTests/com.apple.mDNSResponder/Tests.xctest"];
	[testBundle load];

	XCTestSuite *compiledSuite = [NSClassFromString(@"XCTestSuite") testSuiteForTestCaseWithName: name];
	if (compiledSuite.tests.count) {
		[compiledSuite runTest];
		XCTestRun *testRun = compiledSuite.testRun;
		result = (testRun.hasSucceeded != NO);
	} else {
		fprintf(stderr, "%s Test class %s not found\n", __FUNCTION__, classname);
	}
    return result;
}

bool audit_token_for_pid(pid_t pid, const audit_token_t *token)
{
    kern_return_t err;
    task_t task;
    mach_msg_type_number_t info_size = TASK_AUDIT_TOKEN_COUNT;

    err = task_name_for_pid(mach_task_self(), pid, &task);
    if (err != KERN_SUCCESS) {
        return false;
    }

    err = task_info(task, TASK_AUDIT_TOKEN, (integer_t *) token, &info_size);
    if (err != KERN_SUCCESS) {
        return false;
    }

    return true;
}
