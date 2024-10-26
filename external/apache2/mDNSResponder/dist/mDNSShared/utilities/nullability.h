/*
 * Copyright (c) 2021-2023 Apple Inc. All rights reserved.
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

#ifndef NULLABILITY_H
#define NULLABILITY_H

#ifdef __APPLE__
#include <os/base.h>
#endif

//======================================================================================================================
// MARK: - Macros

#ifndef NULLABLE
	#ifdef __clang__
		#define NULLABLE			_Nullable
		#define NONNULL				_Nonnull
		#define NULL_UNSPECIFIED	_Null_unspecified
		#define UNUSED				__unused
	#else
		#define NULLABLE
		#define NONNULL
		#define NULL_UNSPECIFIED
		#define UNUSED				__attribute__((unused))
	#endif
#endif

#ifndef NULLABILITY_ASSUME_NONNULL_BEGIN
	#ifdef __APPLE__
		#define NULLABILITY_ASSUME_NONNULL_BEGIN	OS_ASSUME_NONNULL_BEGIN
		#define NULLABILITY_ASSUME_NONNULL_END		OS_ASSUME_NONNULL_END
	#else
		#define NULLABILITY_ASSUME_NONNULL_BEGIN
		#define NULLABILITY_ASSUME_NONNULL_END
	#endif
#endif

#endif // NULLABILITY_H
