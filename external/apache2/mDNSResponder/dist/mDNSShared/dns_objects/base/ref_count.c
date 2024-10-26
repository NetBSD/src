/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
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

#include "ref_count.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Object Kind Definition

const struct ref_count_kind_s ref_count_kind = {
	.superkind		= NULL,
	.name			= "ref_count_obj",
	.init			= NULL,
	.compare		= NULL,
	.finalize		= NULL
};

//======================================================================================================================
// MARK: - Object Public Methods

ref_count_obj_t
ref_count_obj_alloc(const size_t size)
{
	return mdns_calloc(1, size);
}

//======================================================================================================================

void
ref_count_obj_init(const ref_count_obj_t me, const ref_count_kind_t new_kind)
{
	me->kind = new_kind;
	for (ref_count_kind_t kind = me->kind; kind != NULL; kind = kind->superkind) {
		if (kind->init != NULL) {
			kind->init(me);
		}
	}
}

//======================================================================================================================

ref_count_obj_t
ref_count_obj_retain(const ref_count_obj_t me)
{
	me->ref_count++;
	return me;
}

//======================================================================================================================

static void
_ref_count_obj_finalize(const ref_count_obj_t me);

void
ref_count_obj_release(const ref_count_obj_t me)
{
	me->ref_count--;
	if (me->ref_count == 0) {
		_ref_count_obj_finalize(me);
	}
}

//======================================================================================================================

compare_result_t
ref_count_obj_compare(const ref_count_obj_t me, const ref_count_obj_t other, const bool check_equality_only)
{
	if (me == other) {
		return compare_result_equal;
	}

	if (me->kind != other->kind) {
		if (check_equality_only) {
			// Two objects that do not have the same kind is not equal.
			return compare_result_notequal;
		} else {
			// There is no way to know the exact order of two different kinds of objects.
			return compare_result_unknown;
		}
	}

	compare_result_t result = compare_result_unknown;
	for (ref_count_kind_t kind = me->kind; kind != NULL; kind = kind->superkind) {
		if (kind->compare != NULL) {
			result = kind->compare(me, other, check_equality_only);
			// If the specified comparator returns COMPARE_RESULT_UNKNOWN, it means that the current comparator of the
			// kind can not determine the compare result. Therefore, we continue to the super kind trying to compare
			// the two objects.
			if (result != compare_result_unknown) {
				break;
			}
		}
	}

	return result;
}

//======================================================================================================================

static inline bool
_continue_searching(const sort_order_t order, const compare_result_t compare_result)
{
	bool continue_searching;

	if (order == sort_order_ascending) {
		if (compare_result == compare_result_less) {
			continue_searching = true;
		} else {
			continue_searching = false;
		}
	} else {
		if (compare_result == compare_result_greater) {
			continue_searching = true;
		} else {
			continue_searching = false;
		}
	}
	return continue_searching;
}

void
ref_count_objs_sort(ref_count_obj_t * const us, const size_t count, const sort_order_t order)
{
	// Insertion sort.
	// Reason: Usually, the number of the reference counted objects to be sorted will be a small number, and the array
	// will almost be in order. Therefore, insertion sort is a reasonable choice.

	if (count <= 1) {
		return;
	}

	for (size_t i = 0; i < count - 1; i++) {
		size_t j = i + 1;
		const ref_count_obj_t me_to_insert = us[j];

		compare_result_t compare_result = ref_count_obj_compare(me_to_insert, us[j - 1], false);
		bool continue_searching = _continue_searching(order, compare_result);

		while (continue_searching) {
			us[j] = us[j - 1];
			j--;
			if (j == 0) {
				break;
			}
			compare_result = ref_count_obj_compare(me_to_insert, us[j - 1], false);
			continue_searching = _continue_searching(order, compare_result);
		}

		us[j] = me_to_insert;
	}
}

//======================================================================================================================
// MARK: - Object Private Methods

static void
_ref_count_obj_finalize(const ref_count_obj_t me)
{
	// Release the resource allocated for the specific kind.
	for (ref_count_kind_t kind = me->kind; kind != NULL; kind = kind->superkind) {
		if (kind->finalize != NULL) {
			kind->finalize(me);
		}
	}

	// Release the memory associated with the object itself.
	ref_count_obj_t me_to_deallocate = me;
	mdns_free(me_to_deallocate);
}
