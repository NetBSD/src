/*
 * Copyright (c) 2023-2024 Apple Inc. All rights reserved.
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

#ifndef MDNS_GENERAL_H
#define MDNS_GENERAL_H

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for a particular platform.
 *
 *	@param PLATFORM_NAME
 *		The name of the platform, e.g., APPLE.
 */
#define MDNS_PLATFORM(PLATFORM_NAME)	MDNS_PLATFORM_PRIVATE_DEFINITION_ ## PLATFORM_NAME ()

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for Apple OSes.
 *
 *	@discussion
 *		`__APPLE__` is defined when compiling for Apple, see
 *		<https://developer.apple.com/library/archive/documentation/Porting/Conceptual/PortingUnix/compiling/compiling.html>.
 */
#if defined(__APPLE__) && __APPLE__
	#define MDNS_PLATFORM_PRIVATE_DEFINITION_APPLE()	1
#else
	#define MDNS_PLATFORM_PRIVATE_DEFINITION_APPLE()	0
#endif

#if MDNS_PLATFORM(APPLE)
	#include <TargetConditionals.h>
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for a particular OS.
 *
 *	@param OS_NAME
 *		The name of the OS, e.g., macOS, iOS, etc.
 */
#define MDNS_OS(OS_NAME)	MDNS_OS_PRIVATE_DEFINITION_ ## OS_NAME ()

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for macOS.
 *
 *	@discussion
 *		Use `MDNS_OS(macOS)` instead of using this macro directly.
 */
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
	#define MDNS_OS_PRIVATE_DEFINITION_macOS()	1
#else
	#define MDNS_OS_PRIVATE_DEFINITION_macOS()	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for iOS.
 *
 *	@discussion
 *		Use `MDNS_OS(iOS)` instead of using this macro directly.
 */
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
	#define MDNS_OS_PRIVATE_DEFINITION_iOS()	1
#else
	#define MDNS_OS_PRIVATE_DEFINITION_iOS()	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for watchOS.
 *
 *	@discussion
 *		Use `MDNS_OS(watchOS)` instead of using this macro directly.
 */
#if defined(TARGET_OS_WATCH) && TARGET_OS_WATCH
	#define MDNS_OS_PRIVATE_DEFINITION_watchOS()	1
#else
	#define MDNS_OS_PRIVATE_DEFINITION_watchOS()	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if compiling for tvOS.
 *
 *	@discussion
 *		Use `MDNS_OS(tvOS)` instead of using this macro directly.
 */
#if defined(TARGET_OS_TV) && TARGET_OS_TV
	#define MDNS_OS_PRIVATE_DEFINITION_tvOS()	1
#else
	#define MDNS_OS_PRIVATE_DEFINITION_tvOS()	0
#endif

// Time conversion constants

#define MDNS_NANOSECONDS_PER_SECOND		1000000000
#define MDNS_MILLISECONDS_PER_SECOND	1000
#define MDNS_MILLISECONDS_PER_MINUTE	(MDNS_MILLISECONDS_PER_SECOND * MDNS_SECONDS_PER_MINUTE)
#define MDNS_MILLISECONDS_PER_HOUR		(MDNS_MILLISECONDS_PER_SECOND * MDNS_SECONDS_PER_HOUR)
#define MDNS_SECONDS_PER_MINUTE			60
#define MDNS_SECONDS_PER_HOUR			(MDNS_SECONDS_PER_MINUTE * MDNS_MINUTES_PER_HOUR)
#define MDNS_SECONDS_PER_DAY			(MDNS_SECONDS_PER_HOUR * MDNS_HOUR_PER_DAY)
#define MDNS_MINUTES_PER_HOUR			60
#define MDNS_HOUR_PER_DAY				24

// Clang's __has_*() builtin macros are defined as zero if not defined.

#if !defined(__has_attribute)
	#define __has_attribute(X)	0
#endif
#if !defined(__has_extension)
	#define __has_extension(X)	0
#endif
#if !defined(__has_feature)
	#define __has_feature(X)	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler is Clang.
 *
 *	@discussion
 *		__clang__ is defined when compiling with Clang, see
 *		<https://clang.llvm.org/docs/LanguageExtensions.html#builtin-macros>.
 */
#if defined(__clang__)
	#define MDNS_COMPILER_IS_CLANG() 1
#else
	#define MDNS_COMPILER_IS_CLANG() 0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler is Clang and its version is at least a specified version.
 *
 *	@param MAJOR
 *		The specified version's major number.
 *
 *	@param MINOR
 *		The specified version's minor number.
 *
 *	@param PATCH_LEVEL
 *		The specified version's patch level.
 *
 *	@discussion
 *		Clang version numbers are of the form "<major number>.<minor number>.<patch level>". See
 *		<https://clang.llvm.org/docs/LanguageExtensions.html#builtin-macros>
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_CLANG_VERSION_IS_AT_LEAST(MAJOR, MINOR, PATCH_LEVEL) (						\
		(__clang_major__ > (MAJOR)) || (													\
			(__clang_major__ == (MAJOR)) && (												\
				(__clang_minor__ > (MINOR)) || (											\
					(__clang_minor__ == (MINOR)) && (__clang_patchlevel__ >= (PATCH_LEVEL))	\
				)																			\
			)																				\
		)																					\
	)
#else
	#define MDNS_CLANG_VERSION_IS_AT_LEAST(MAJOR, MINOR, PATCH_LEVEL) 0
#endif

/*!
 *	@brief
 *		Stringizes the argument and passes it to the _Pragma() operator, which takes a string literal argument.
 *
 *	@param ARG
 *		The argument.
 *
 *	@discussion
 *		Useful for escaping double quotes. For example,
 *
 *			MDNS_PRAGMA_WITH_STRINGIZED_ARGUMENT(clang diagnostic ignored "-Wpadded")
 *
 *		turns into
 *
 *			_Pragma("clang diagnostic ignored \"-Wpadded\"")
 *
 *		See <https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html>.
 */
#define MDNS_PRAGMA_WITH_STRINGIZED_ARGUMENT(ARG)	_Pragma(#ARG)

/*!
 *	@brief
 *		For Clang, starts ignoring the specified warning diagnostic flag.
 *
 *	@param WARNING
 *		The warning diagnostic flag.
 *
 *	@discussion
 *		Use MDNS_CLANG_IGNORE_WARNING_END() to undo the effect of this macro.
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_CLANG_IGNORE_WARNING_BEGIN(WARNING)	\
		_Pragma("clang diagnostic push")				\
		MDNS_PRAGMA_WITH_STRINGIZED_ARGUMENT(clang diagnostic ignored #WARNING)
#else
	#define MDNS_CLANG_IGNORE_WARNING_BEGIN(WARNING)
#endif

/*!
 *	@brief
 *		Use to undo the effect of a previous MDNS_CLANG_IGNORE_WARNING_BEGIN().
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_CLANG_IGNORE_WARNING_END()	_Pragma("clang diagnostic pop")
#else
	#define MDNS_CLANG_IGNORE_WARNING_END()
#endif

/*!
 *	@brief
 *		An alternative version of MDNS_CLANG_IGNORE_WARNING_BEGIN() that looks nicer when used among statements.
 *
 *	@discussion
 *		This version looks nicer when used among C statements. Here's an example:
 *
 *			mdns_clang_ignore_warning_begin(-Wformat-nonliteral);
 *			const int n = vsnprintf(dst, len, fmt, args);
 *			mdns_clang_ignore_warning_end();
 */
#define mdns_clang_ignore_warning_begin(WARNING)	\
	MDNS_CLANG_IGNORE_WARNING_BEGIN(WARNING)		\
	do {} while (0)

/*!
 *	@brief
 *		An alternative version of MDNS_CLANG_IGNORE_WARNING_END() that looks nicer when used among statements.
 *
 *	@discussion
 *		This version looks nicer when used among C statements. Here's an example:
 *
 *			mdns_clang_ignore_warning_begin(-Wformat-nonliteral);
 *			const int n = vsnprintf(dst, len, fmt, args);
 *			mdns_clang_ignore_warning_end();
 */
#define mdns_clang_ignore_warning_end()	\
	MDNS_CLANG_IGNORE_WARNING_END()		\
	do {} while (0)

/*!
 *	@brief
 *		For Clang, starts ignoring the -Wunaligned-access warning diagnostic flag.
 *
 *	@discussion
 *		The -Wunaligned-access is new in clang version 14.0.3. This macro allow us to conditionally ignore
 *		-Wunaligned-access with Clang 14.0.3 or later. This avoids -Wunknown-warning-option warnings with
 *		earlier Clang versions, which don't recognize -Wunaligned-access.
 */
#if MDNS_CLANG_VERSION_IS_AT_LEAST(14, 0, 3)
	#define MDNS_CLANG_IGNORE_UNALIGNED_ACCESS_WARNING_BEGIN()	MDNS_CLANG_IGNORE_WARNING_BEGIN(-Wunaligned-access)
#else
	#define MDNS_CLANG_IGNORE_UNALIGNED_ACCESS_WARNING_BEGIN()
#endif

/*!
 *	@brief
 *		Undoes the effect of a previous MDNS_CLANG_IGNORE_UNALIGNED_ACCESS_WARNING_BEGIN().
 */
#if MDNS_CLANG_VERSION_IS_AT_LEAST(14, 0, 3)
	#define MDNS_CLANG_IGNORE_UNALIGNED_ACCESS_WARNING_END()	MDNS_CLANG_IGNORE_WARNING_END()
#else
	#define MDNS_CLANG_IGNORE_UNALIGNED_ACCESS_WARNING_END()
#endif

/*!
 *	@brief
 *		For Clang, starts ignoring the -Wincompatible-function-pointer-types-strict warning diagnostic flag.
 *
 *	@discussion
 *		-Wincompatible-function-pointer-types-strict is like -Wincompatible-function-pointer-types, but is more
 *		strict in that it warns about function pointer types that are not identical but are still compatible.
 *
 *		The -Wincompatible-function-pointer-types-strict is new in clang version 16.0.0 (see
 *		https://releases.llvm.org/16.0.0/tools/clang/docs/ReleaseNotes.html). This macro allow us to
 *		conditionally ignore -Wincompatible-function-pointer-types-strict with Clang 16.0.0 or later. This
 *		avoids -Wunknown-warning-option warnings with earlier Clang versions, which don't recognize
 *		-Wincompatible-function-pointer-types-strict.
 */
#if MDNS_CLANG_VERSION_IS_AT_LEAST(16, 0, 0)
	#define MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN() \
		MDNS_CLANG_IGNORE_WARNING_BEGIN(-Wincompatible-function-pointer-types-strict)
#else
	#define MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN()
#endif

/*!
 *	@brief
 *		Undoes the effect of a previous
 *		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN().
 */
#if MDNS_CLANG_VERSION_IS_AT_LEAST(16, 0, 0)
	#define MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()	MDNS_CLANG_IGNORE_WARNING_END()
#else
	#define MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()
#endif

/*!
 *	@brief
 *		For Clang, treats the specified warning diagnostic flag as an error.
 *
 *	@param WARNING
 *		The warning diagnostic flag.
 *
 *	@discussion
 *		Use MDNS_CLANG_TREAT_WARNING_AS_ERROR_END() to undo the effect of this macro.
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_CLANG_TREAT_WARNING_AS_ERROR_BEGIN(WARNING)	\
		_Pragma("clang diagnostic push")						\
		MDNS_PRAGMA_WITH_STRINGIZED_ARGUMENT(clang diagnostic error #WARNING)
#else
	#define MDNS_CLANG_TREAT_WARNING_AS_ERROR_BEGIN(WARNING)
#endif

/*!
 *	@brief
 *		Undoes the effect of a previous MDNS_CLANG_TREAT_WARNING_AS_ERROR_BEGIN().
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_CLANG_TREAT_WARNING_AS_ERROR_END()	_Pragma("clang diagnostic pop")
#else
	#define MDNS_CLANG_TREAT_WARNING_AS_ERROR_END()
#endif

/*!
 *	@brief
 *		For Clang, specifies that pointers without a nullability qualifier are _Nonnull.
 *
 *	@discussion
 *		See <https://clang.llvm.org/docs/AttributeReference.html#nullability-attributes>.
 */
#if (MDNS_COMPILER_IS_CLANG() && __has_feature(assume_nonnull))
	#define MDNS_ASSUME_NONNULL_BEGIN	_Pragma("clang assume_nonnull begin")
#else
	#define MDNS_ASSUME_NONNULL_BEGIN
#endif

/*!
 *	@brief
 *		Undoes the effect of a previous MDNS_ASSUME_NONNULL_BEGIN.
 */
#if (MDNS_COMPILER_IS_CLANG() && __has_feature(assume_nonnull))
	#define MDNS_ASSUME_NONNULL_END	_Pragma("clang assume_nonnull end")
#else
	#define MDNS_ASSUME_NONNULL_END
#endif

/*!
 *	@brief
 *		If supported, an attribute for closed enumeration definitions.
 *
 *	@discussion
 *		See <https://clang.llvm.org/docs/AttributeReference.html#enum-extensibility>.
 */
#if __has_attribute(enum_extensibility)
	#define MDNS_ENUM_ATTR_CLOSED	__attribute__((enum_extensibility(closed)))
#else
	#define MDNS_ENUM_ATTR_CLOSED
#endif

/*!
 *	@brief
 *		If supported, defines a fixed-width closed enumeration.
 *
 *	@param NAME
 *		The name of the enumeration.
 *
 *	@param UNDERLYING_TYPE
 *		The enumeration's underlying type.
 *
 *	@param ...
 *		The enumerator list.
 *
 *	@discussion
 *		See <https://clang.llvm.org/docs/LanguageExtensions.html#enumerations-with-a-fixed-underlying-type> and
 *		<https://clang.llvm.org/docs/AttributeReference.html#enum-extensibility>.
 */
#if (__has_feature(objc_fixed_enum) || __has_extension(cxx_fixed_enum) || __has_extension(cxx_strong_enums))
	#define MDNS_CLOSED_ENUM(NAME, UNDERLYING_TYPE, ...)	\
		typedef enum : UNDERLYING_TYPE {					\
			__VA_ARGS__										\
		} MDNS_ENUM_ATTR_CLOSED NAME
#else
	#define MDNS_CLOSED_ENUM(NAME, UNDERLYING_TYPE, ...)	\
		typedef UNDERLYING_TYPE NAME;						\
		enum NAME ## _enum {								\
			__VA_ARGS__										\
		} MDNS_ENUM_ATTR_CLOSED
#endif

/*!
 *	@brief
 *		If supported, an attribute for flag-like enumeration definitions.
 *
 *	@discussion
 *		See <https://clang.llvm.org/docs/AttributeReference.html#flag-enum>.
 */
#if __has_attribute(flag_enum)
	#define MDNS_ENUM_ATTR_FLAG	__attribute__((flag_enum))
#else
	#define MDNS_ENUM_ATTR_FLAG
#endif

/*!
 *	@brief
 *		If supported, defines a fixed-width closed flag-like enumeration.
 *
 *	@param NAME
 *		The name of the enumeration.
 *
 *	@param UNDERLYING_TYPE
 *		The enumeration's underlying type.
 *
 *	@param ...
 *		The enumeratior list.
 *
 *	@discussion
 *		See <https://clang.llvm.org/docs/LanguageExtensions.html#enumerations-with-a-fixed-underlying-type> and
 *		<https://clang.llvm.org/docs/AttributeReference.html#flag-enum>.
 */
#if (__has_feature(objc_fixed_enum) || __has_extension(cxx_fixed_enum) || __has_extension(cxx_strong_enums))
	#define MDNS_CLOSED_OPTIONS(NAME, UNDERLYING_TYPE, ...)	\
		typedef enum : UNDERLYING_TYPE {					\
			__VA_ARGS__										\
		} MDNS_ENUM_ATTR_CLOSED MDNS_ENUM_ATTR_FLAG NAME
#else
	#define MDNS_CLOSED_OPTIONS(NAME, UNDERLYING_TYPE, ...)	\
		typedef UNDERLYING_TYPE NAME;						\
		enum NAME ## _enum {								\
			__VA_ARGS__										\
		} MDNS_ENUM_ATTR_CLOSED MDNS_ENUM_ATTR_FLAG
#endif

/*!
 *	@brief
 *		For compatibility with C++, marks the beginning of C function declarations.
 *
 *	@discussion
 *		See <https://en.cppreference.com/w/cpp/language/language_linkage>.
 */
#if defined(__cplusplus)
	#define MDNS_C_DECLARATIONS_BEGIN	extern "C" {
#else
	#define MDNS_C_DECLARATIONS_BEGIN
#endif

/*!
 *	@brief
 *		For compatibility with C++, marks the end of C function declarations.
 *
 *	@discussion
 *		This is the counterpart to MDNS_C_DECLARATIONS_BEGIN.
 */
#if defined(__cplusplus)
	#define MDNS_C_DECLARATIONS_END	}
#else
	#define MDNS_C_DECLARATIONS_END
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler conforms to a specific minimum C standard.
 *
 *	@param STANDARD
 *		The C standard.
 */
#define MDNS_C_STANDARD_IS_AT_LEAST(STANDARD)	MDNS_C_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_ ## STANDARD ()

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler confroms to the C99 standard or later.
 *
 *	@discussion
 *		__STDC_VERSION__ is a predefined macro that expands to 199901L for the C99 standard. See
 *		<https://en.cppreference.com/w/c/preprocessor/replace>.
 *
 *		Use `MDNS_C_STANDARD_IS_AT_LEAST(C99)` instead of using this macro directly.
 */
#if defined(__STDC_VERSION__)
	#define MDNS_C_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_C99()	(__STDC_VERSION__ >= 199901L)
#else
	#define MDNS_C_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_C99()	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler confroms to the C11 standard or later.
 *
 *	@discussion
 *		__STDC_VERSION__ is a predefined macro that expands to 201112L for the C11 standard. See
 *		<https://en.cppreference.com/w/c/preprocessor/replace>.
 *
 *		Use `MDNS_C_STANDARD_IS_AT_LEAST(C11)` instead of using this macro directly.
 */
#if defined(__STDC_VERSION__)
	#define MDNS_C_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_C11()	(__STDC_VERSION__ >= 201112L)
#else
	#define MDNS_C_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_C11()	0
#endif

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler conforms to a specific minimum C++ standard.
 *
 *	@param STANDARD
 *		The C standard.
 */
#define MDNS_CPP_STANDARD_IS_AT_LEAST(STANDARD)	MDNS_CPP_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_ ## STANDARD ()

/*!
 *	@brief
 *		Evaluates to non-zero if the compiler confroms to the C++11 standard or later.
 *
 *	@discussion
 *		__cplusplus is a predefined macro that expands to 201103L for the C++11 standard. See
 *		<https://en.cppreference.com/w/cpp/preprocessor/replace>.
 *
 *		Use `MDNS_CPP_STANDARD_IS_AT_LEAST(CPP11)` instead of using this macro directly.
 */
#if defined(__cplusplus)
	#define MDNS_CPP_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_CPP11()	(__cplusplus >= 201103L)
#else
	#define MDNS_CPP_STANDARD_PRIVATE_DEFINITION_IS_AT_LEAST_CPP11()	0
#endif

/*!
 *	@brief
 *		Causes a compile-time error if an expression evaluates to false.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@param MESSAGE
 *		If supported, a sting literal to include as a diagnostic message if the expression evaluates to false.
 */
#if MDNS_C_STANDARD_IS_AT_LEAST(C11)
	#define mdns_compile_time_check(EXPRESSION, MESSAGE)	_Static_assert(EXPRESSION, MESSAGE)
#elif MDNS_CPP_STANDARD_IS_AT_LEAST(CPP11)
	#define mdns_compile_time_check(EXPRESSION, MESSAGE)	static_assert(EXPRESSION, MESSAGE)
#elif defined(__cplusplus)
	#define	mdns_compile_time_check(EXPRESSION, MESSAGE) \
		extern "C" int mdns_compile_time_check_failed[(EXPRESSION) ? 1 : -1]
#else
	#define	mdns_compile_time_check(EXPRESSION, MESSAGE) \
		extern int mdns_compile_time_check_failed[(EXPRESSION) ? 1 : -1]
#endif

/*!
 *	@brief
 *		Causes a compile-time error if an expression evaluates to false.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@discussion
 *		This macro is meant to be used in a local scope, i.e., inside of a function or a block. For the global
 *		scope, use `mdns_compile_time_check()`.
 *
 *		The fallback implementation is based on code from
 *		<https://www.drdobbs.com/compile-time-assertions/184401873>.
 */
#if MDNS_C_STANDARD_IS_AT_LEAST(C11)
	#define mdns_compile_time_check_local(EXPRESSION)	_Static_assert(EXPRESSION, "Compile-time assertion failed.")
#elif MDNS_CPP_STANDARD_IS_AT_LEAST(CPP11)
	#define mdns_compile_time_check_local(EXPRESSION)	static_assert(EXPRESSION, "Compile-time assertion failed.")
#else
	#define mdns_compile_time_check_local(EXPRESSION)								\
		do {																		\
			enum {																	\
				mdns_compile_time_check_local_failed = 1 / ((EXPRESSION) ? 1 : 0)	\
			};																		\
		} while (0)
#endif

/*!
 *	@brief
 *		Determines at compile-time if the size of a type exceeds a specified maximum.
 *
 *	@param TYPE
 *		The type.
 *
 *	@param MAX_SIZE
 *		The maximum size in bytes.
 */
#define mdns_compile_time_max_size_check(TYPE, MAX_SIZE) \
	mdns_compile_time_check(sizeof(TYPE) <= MAX_SIZE, "The size of " # TYPE " exceeds max size of '" # MAX_SIZE "'.")

/*!
 *	@brief
 *		Determines the size of an array's element type.
 *
 *	@param ARRAY
 *		The array.
 */
#define mdns_sizeof_element(ARRAY)	sizeof(ARRAY[0])

/*!
 *	@brief
 *		Determines the size of a member variable from a struct or union.
 *
 *	@param TYPE
 *		The type name of the struct or union.
 *
 *	@param MEMBER
 *		The name of the member variable.
 */
#define mdns_sizeof_member(TYPE, MEMBER)	sizeof(((TYPE *)0)->MEMBER)

/*!
 *	@brief
 *		Determines the number of elements in an array.
 *
 *	@param ARRAY
 *		The array.
 */
#define mdns_countof(ARRAY)	(sizeof(ARRAY) / mdns_sizeof_element(ARRAY))

/*!
 *	@brief
 *		If an expression evaluates to false, transfers control to a goto label.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@param LABEL
 *		The location's goto label.
 *
 *	@discussion
 *		No debugging information is logged.
 */
#define mdns_require_quiet(EXPRESSION, LABEL)	\
	do {										\
		if (!(EXPRESSION)) {					\
			goto LABEL;							\
		}										\
	} while (0)

/*!
 *	@brief
 *		If an expression evaluates to false, executes an action, then transfers control to a goto label.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@param LABEL
 *		The goto label.
 *
 *	@param ACTION
 *		The code to execute.
 *
 *	@discussion
 *		No debugging information is logged.
 */
#define mdns_require_action_quiet(EXPRESSION, LABEL, ACTION)	\
	do {														\
		if (!(EXPRESSION)) {									\
			{													\
				ACTION;											\
			}													\
			goto LABEL;											\
		}														\
	} while (0)

/*!
 *	@brief
 *		If an error code is non-zero, transfers control to a goto label.
 *
 *	@param ERROR
 *		The error code.
 *
 *	@param LABEL
 *		The location's goto label.
 *
 *	@discussion
 *		No debugging information is logged.
 */
#define mdns_require_noerr_quiet(ERROR, LABEL)	mdns_require_quiet(!(ERROR), LABEL)

/*!
 *	@brief
 *		If an error code is non-zero, executes an action, then transfers control to a goto label.
 *
 *	@param ERROR
 *		The error code.
 *
 *	@param LABEL
 *		The location's goto label.
 *
 *	@param ACTION
 *		The code to execute.
 *
 *	@discussion
 *		No debugging information is logged.
 */
#define mdns_require_noerr_action_quiet(ERROR, LABEL, ACTION)	mdns_require_action_quiet(!(ERROR), LABEL, ACTION)

/*!
 *	@brief
 *		Returns from the current function if an expression evaluates to false.
 *
 *	@param EXPRESSION
 *		The expression.
 */
#define mdns_require_return(EXPRESSION)	\
	do {								\
		if (!(EXPRESSION)) {			\
			return;						\
		}								\
	} while (0)

/*!
 *	@brief
 *		If an expression evaluates to false, executes an action, then returns.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@param ACTION
 *		The code to execute.
 *
 *	@discussion
 *		No debugging information is logged.
 */
#define mdns_require_return_action(EXPRESSION, ACTION)	\
	do {												\
		if (!(EXPRESSION)) {							\
			{											\
				ACTION;									\
			}											\
			return;										\
		}												\
	} while (0)

/*!
 *	@brief
 *		Returns from the current function with a specified value if an expression evaluates to false.
 *
 *	@param EXPRESSION
 *		The expression.
 *
 *	@param VALUE
 *		The return value.
 */
#define mdns_require_return_value(EXPRESSION, VALUE)	\
	do {												\
		if (!(EXPRESSION)) {							\
			return (VALUE);								\
		}												\
	} while (0)

/*!
 *	@brief
 *		Returns from the current function with a specified return value if an error code is non-zero.
 *
 *	@param ERROR
 *		The error code.
 *
 *	@param VALUE
 *		The return value.
 */
#define mdns_require_noerr_return_value(ERROR, VALUE)	mdns_require_return_value(!(ERROR), VALUE)

/*!
 *	@brief
 *		Assigns a value to a variable if the variable's address isn't NULL.
 *
 *	@param VARIABLE_ADDR
 *		The variable's address.
 *
 *	@param VALUE
 *		The value.
 */
#define mdns_assign(VARIABLE_ADDR, VALUE)	\
	do {									\
		if (VARIABLE_ADDR) {				\
			*(VARIABLE_ADDR) = (VALUE);		\
		}									\
	} while (0)

/*!
 *	@brief
 *		Declares an array of bytes that is meant to be used as explicit padding at the end of a struct.
 *
 *	@param BYTE_COUNT
 *		The size of the array in number of bytes.
 *
 *	@discussion
 *		This explicit padding is meant to be used as the final member variable of a struct to eliminate the
 *		-Wpadded warning about a struct's overall size being implicitly padded up to an alignment boundary.
 *		In other words, in place of such implicit padding, use this explicit padding instead.
 */
#define MDNS_STRUCT_PAD(BYTE_COUNT)							\
	MDNS_CLANG_IGNORE_WARNING_BEGIN(-Wzero-length-array)	\
	char _mdns_unused_padding[(BYTE_COUNT)]					\
	MDNS_CLANG_IGNORE_WARNING_END()

/*!
 *	@brief
 *		Like MDNS_STRUCT_PAD(), except that the amount of padding is specified for 64-bit and 32-bit platforms.
 *
 *	@param BYTE_COUNT_64
 *		The amount of padding in number of bytes to use on 64-bit platforms.
 *
 *	@param BYTE_COUNT_32
 *		The amount of padding in number of bytes to use on 32-bit platforms.
 *
 *	@discussion
 *		This macro assumes that pointers on 64-bit platforms are eight bytes in size and that pointers on 32-bit
 *		platforms are four bytes in size.
 *
 *		If the size of a pointer is something other than eight or four bytes, then a compiler error will occur.
 */
#define MDNS_STRUCT_PAD_64_32(BYTE_COUNT_64, BYTE_COUNT_32)	\
	MDNS_STRUCT_PAD(										\
		(sizeof(void *) == 8) ? BYTE_COUNT_64 :				\
		(sizeof(void *) == 4) ? BYTE_COUNT_32 : -1			\
	)

/*!
 *	@brief
 *		Compile-time check to ensure that a struct that uses MDNS_STRUCT_PAD() or MDNS_STRUCT_PAD_64_32() hasn't
 *		specified too much padding.
 *
 *	@param STRUCT_TYPE
 *		The struct type.
 *
 *	@discussion
 *		There's too much padding if the padding's size is greater than or equal to the struct's alignment
 *		requirement. This is because the point of MDNS_STRUCT_PAD() and MDNS_STRUCT_PAD_64_32() is to explicitly
 *		pad a struct up to a multiple of the struct's alignment requirement. Violating this check would
 *		unnecessarily increase the size of the struct.
 */
#define MDNS_GENERAL_STRUCT_PAD_CHECK(STRUCT_TYPE)															\
	mdns_compile_time_check(mdns_sizeof_member(STRUCT_TYPE, _mdns_unused_padding) < _Alignof(STRUCT_TYPE),	\
		"Padding exceeds alignment of '" # STRUCT_TYPE "', so the amount of padding is excessive.")

/*!
 *	@brief
 *		Retains a Core Foundation object if the specified object reference is non-NULL.
 *
 *	@param OBJ
 *		A reference to the object to retain.
 *
 *	@discussion
 *		The object reference is explicitly compared against NULL to avoid a warning from the Clang analyzer's
 *		osx.NumberObjectConversion checker. See
 *		<https://clang.llvm.org/docs/analyzer/checkers.html#osx-numberobjectconversion-c-c-objc>.
 */
#define mdns_cf_retain_null_safe(OBJ)	\
	do {								\
		if ((OBJ) != NULL) {			\
			CFRetain((OBJ));			\
		}								\
	} while (0)

/*!
 *	@brief
 *		Releases the Core Foundation object referenced by a pointer.
 *
 *	@param OBJ_PTR
 *		The address of the pointer that either references a Core Foundation object or references NULL.
 *
 *	@discussion
 *		If the pointer contains a non-NULL reference, then the pointer will be set to NULL after releasing the
 *		object.
 *
 *		The object reference is explicitly compared against NULL to avoid a warning from the Clang analyzer's
 *		osx.NumberObjectConversion checker. See
 *		<https://clang.llvm.org/docs/analyzer/checkers.html#osx-numberobjectconversion-c-c-objc>.
 */
#define mdns_cf_forget(OBJ_PTR)		\
	do {							\
		if (*(OBJ_PTR) != NULL) {	\
			CFRelease(*(OBJ_PTR));	\
			*(OBJ_PTR) = NULL;		\
		}							\
	} while (0)

/*!
 *	@brief
 *		Alternative to the `default` label in a switch statement that covers all enumeration values.
 *
 *	@discussion
 *		Use `MDNS_COVERED_SWITCH_DEFAULT` instead of `default` to avoid the `-Wcovered-switch-default` warning
 *		in a switch statement that covers all enumeration values. This macro is useful when strict enforcement
 *		of the `-Wswitch-default` warning compels us to include a default label in such switch statements.
 */
#if MDNS_COMPILER_IS_CLANG()
	#define MDNS_COVERED_SWITCH_DEFAULT								\
		MDNS_CLANG_IGNORE_WARNING_BEGIN(-Wcovered-switch-default)	\
		default														\
		MDNS_CLANG_IGNORE_WARNING_END()
#else
	#define MDNS_COVERED_SWITCH_DEFAULT	default
#endif

/*!
 *	@brief
 *		The static keyword for array parameters for C99 or later.
 *
 *	@discussion
 *		See <https://en.cppreference.com/w/c/language/operator_other#Function_call>.
 */
#if MDNS_C_STANDARD_IS_AT_LEAST(C99)
	#define MDNS_STATIC_ARRAY_PARAM	static
#else
	#define MDNS_STATIC_ARRAY_PARAM
#endif

/*!
 *	@brief
 *		The size of a Universally Unique Identifier (UUID) in bytes.
 *
 *	@discussion
 *		See <https://datatracker.ietf.org/doc/html/rfc4122#section-4.1>.
 */
#define MDNS_UUID_SIZE	16

#endif	// MDNS_GENERAL_H
