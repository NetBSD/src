/*
 * Copyright (c) 2022 Apple Inc. All rights reserved.
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

#ifndef DNS_ASSERT_MACROS_H
#define DNS_ASSERT_MACROS_H

//======================================================================================================================
// MARK: - Platform Dependent Assert Macros

#ifdef __APPLE__

// MARK: - Apple

#include <AssertMacros.h>
#else

// MARK: - Other

#include <stdbool.h>

#ifndef DEBUG_ASSERT_MESSAGE
	#define DEBUG_ASSERT_MESSAGE
#endif

#ifndef require
	#define require(assertion, exception_label)			\
		do {											\
			if (__builtin_expect(!(assertion), 0)) {	\
				goto exception_label;					\
			}											\
		} while (false)
#endif

#ifndef require_quiet
	#define require_quiet require
#endif

#ifndef require_action
	#define require_action(assertion, exception_label, action)	\
		do {													\
			if (__builtin_expect(!(assertion), 0)) {			\
				{												\
					action;										\
				}												\
				goto exception_label;							\
			}													\
		} while (false)
#endif

#ifndef require_action_quiet
	#define require_action_quiet require_action
#endif

#ifndef require_noerr
	#define require_noerr(error_code, exceptional_label)		\
		do {													\
			long error_code_long = (error_code);				\
			if (__builtin_expect(0 != error_code_long, 0)) {	\
				goto exceptional_label;							\
			}													\
		} while (false)
#endif

#ifndef require_noerr_action
	#define require_noerr_action(error_code, exceptional_label, action)	\
		do {															\
			long error_code_long = (error_code);						\
			if (__builtin_expect(0 != error_code_long, 0)) {			\
				{														\
					action;												\
				}														\
				goto exceptional_label;									\
			}															\
		} while (false)
#endif

#ifndef verify_action
	#define verify_action(assertion, action)		\
		if (__builtin_expect(!(assertion), 0)) {	\
			action;									\
		} else do {} while (0)
#endif

#endif // __APPLE__

//======================================================================================================================
// MARK: - Common Assert Macros

#ifndef check_compile_time
	#define check_compile_time(expr)	extern int compile_time_assert_failed[(expr) ? 1 : -1]
#endif

#ifndef check_compile_time_code
	#define check_compile_time_code(X)	do {switch(0) {case 0: case X:;}} while( 0 )
#endif

#ifndef require_return
	#define require_return(assertion)											\
	do {																		\
		if(__builtin_expect(!(assertion), 0)) {									\
			DEBUG_ASSERT_MESSAGE("", #assertion, 0, 0, __FILE__, __LINE__, 0);	\
			return;																\
		}																		\
	} while(false)
#endif

#ifndef require_return_quiet
	#define require_return_quiet(assertion)										\
	do {																		\
		if(__builtin_expect(!(assertion), 0)) {									\
			return;																\
		}																		\
	} while(false)
#endif

#ifndef require_return_value
	#define require_return_value(assertion, value)								\
	do {																		\
		if(__builtin_expect(!(assertion), 0)) {									\
			DEBUG_ASSERT_MESSAGE("", #assertion, 0, 0, __FILE__, __LINE__, 0);	\
			return (value);														\
		}																		\
	} while(false)
#endif

#ifndef require_return_value_quiet
	#define require_return_value_quiet(assertion, value)						\
	do {																		\
		if(__builtin_expect(!(assertion), 0)) {									\
			return (value);														\
		}																		\
	} while(false)
#endif

#ifndef require_noerr_return
	#define require_noerr_return(error_code)																\
		do {																								\
			long error_code_long = (error_code);															\
			if (__builtin_expect(0 != error_code_long, 0)) {												\
				DEBUG_ASSERT_MESSAGE("", #error_code " == 0", 0, 0, __FILE__, __LINE__, error_code_long);	\
				return;																						\
			}																								\
		} while (false)
#endif

#ifndef require_noerr_return_quiet
	#define require_noerr_return_quiet(error_code)															\
		do {																								\
			long error_code_long = (error_code);															\
			if (__builtin_expect(0 != error_code_long, 0)) {												\
				return;																						\
			}																								\
		} while (false)
#endif

#ifndef require_noerr_return_value
	#define require_noerr_return_value(error_code, value)													\
		do {																								\
			long error_code_long = (error_code);															\
			if (__builtin_expect(0 != error_code_long, 0)) {												\
				DEBUG_ASSERT_MESSAGE("", #error_code " == 0", 0, 0, __FILE__, __LINE__, error_code_long);	\
				return (value);																				\
			}																								\
		} while (false)
#endif

#ifndef require_noerr_return_value_quiet
	#define require_noerr_return_value_quiet(error_code, value)												\
		do {																								\
			long error_code_long = (error_code);															\
			if (__builtin_expect(0 != error_code_long, 0)) {												\
				return (value);																				\
			}																								\
		} while (false)
#endif

#endif // DNS_ASSERT_MACROS_H
