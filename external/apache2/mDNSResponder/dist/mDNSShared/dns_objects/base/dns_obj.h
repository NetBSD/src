/*
 * Copyright (c) 2022-2023 Apple Inc. All rights reserved.
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

#ifndef DNS_OBJECT_H
#define DNS_OBJECT_H

//======================================================================================================================
// MARK: - Headers

#include "ref_count.h"

#include "nullability.h"

//======================================================================================================================
// MARK: - DNS Object Kind and Subkind Helper Macros

// Define a base DNS object.
#define DNS_OBJECT_DEFINE_FULL(NAME)							REF_COUNT_OBJECT_DEFINE_FULL(dns_obj, NAME)
// Define a kind type for a DNS object that has a subkind.
#define DNS_OBJECT_DEFINE_KIND_TYPE_FOR_SUBKIND(NAME, ...)		REF_COUNT_OBJECT_DEFINE_KIND_TYPE_FOR_SUBKIND(dns_obj, NAME, __VA_ARGS__)

// Define a subkind of a DNS object.
// Subkind with no comparator nor finalizer.
#define DNS_OBJECT_SUBKIND_DEFINE_ABSTRUCT(SUPER, NAME, ...)			REF_COUNT_OBJECT_SUBKIND_DEFINE_ABSTRUCT(dns_obj, SUPER, NAME, __VA_ARGS__)
// Subkind with no finalizer, but with comparator.
#define DNS_OBJECT_SUBKIND_DEFINE_WITHOUT_FINALIZER(SUPER, NAME, ...)	REF_COUNT_OBJECT_SUBKIND_DEFINE_WITHOUT_FINALIZER(dns_obj, SUPER, NAME, __VA_ARGS__)
// Subkind with finalizer, but with no comparator.
#define DNS_OBJECT_SUBKIND_DEFINE_WITHOUT_COMPARATOR(SUPER, NAME, ...)	REF_COUNT_OBJECT_SUBKIND_DEFINE_WITHOUT_COMPARATOR(dns_obj, SUPER, NAME, __VA_ARGS__)
// Subkind with both finalizer and comparator.
#define DNS_OBJECT_SUBKIND_DEFINE_FULL(SUPER, NAME, ...)				REF_COUNT_OBJECT_SUBKIND_DEFINE_FULL(dns_obj, SUPER, NAME, __VA_ARGS__)

// Declare all kinds and subkinds as the DNS objects.
// Declare an object as a DNS object.
#define DNS_OBJECT_DECLARE_SUPPORTED_OBJECT(NAME)						REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_obj, NAME)
// Declare a subkind of object as a DNS object.
#define DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(SUPER, NAME)		REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, SUPER, NAME)

// Declare an object array as a DNS object array.
#define DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(NAME)					REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(dns_obj, NAME)
// Declare a subkind of object array as a DNS object array.
#define DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(SUPER, NAME)	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, SUPER, NAME)

#define DNS_OBJECT_TYPEDEF_OPAQUE_POINTER(NAME)					OBJECT_TYPEDEF_OPAQUE_POINTER(dns_obj_ ## NAME)
#define DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(SUPER, NAME)	OBJECT_TYPEDEF_OPAQUE_POINTER(dns_obj_ ## SUPER ## _ ## NAME)

//======================================================================================================================
// MARK: - DNS Object Families

OBJECT_TYPEDEF_OPAQUE_POINTER(dns_obj);

typedef union {
	STRUCT_PTR_DECLARE(dns_obj);								// Declare dns_obj_t object family.

	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT(domain_name);			// Declare dns_obj_domain_name_t as a dns_obj_t object.

	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT(rr);					// Declare dns_obj_rr_t as a dns_obj_t object.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, cname);		// Declare dns_obj_rr_cname_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, soa);		// Declare dns_obj_rr_soa_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, srv);		// Declare dns_obj_rr_srv_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec);		// Declare dns_obj_rr_nsec_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, ds);		// Declare dns_obj_rr_ds_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, rrsig);		// Declare dns_obj_rr_rrsig_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, dnskey);	// Declare dns_obj_rr_dnskey_t as a subkind of dns_obj_rr_t.
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec3);		// Declare dns_obj_rr_nsec3_t as a subkind of dns_obj_rr_t.

} dns_obj_any_t __attribute__((__transparent_union__)); // __transparent_union__ makes all object above as valid DNS objects.

typedef union {
	STRUCT_ARRAY_PTR_DECLARE(dns_obj);								// Declare dns_obj_t objects to be sortable.
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(domain_name);				// Declare dns_obj_domain_name_t objects to be sortable.
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(rr);						// Declare dns_obj_rr_t objects to be sortable.
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec);		// Declare dns_obj_rr_nsec_t objects to be sortable.
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, dnskey);		// Declare dns_obj_rr_dnskey_t objects to be sortable.
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec3);		// Declare dns_obj_rr_nsec3_t objects to be sortable.
} dns_objs_any_t __attribute__((__transparent_union__));				// __transparent_union__ makes all arrays above as valid DNS object arrays.

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Retain a supported DNS object by increasing the reference count by one.
 *
 *	@param dns_object
 *		The supported DNS object contained in dns_obj_any_t union.
 *
 *	@result
 *		The retained DNS object.
 */
dns_obj_t NONNULL
dns_obj_retain(dns_obj_any_t dns_object);

/*!
 *	@brief
 *		Release a supported DNS object by decreasing the reference count by one. If the reference count becomes zero after releasing, the object will be
 *		finalized.
 *
 *	@param dns_object
 *		The supported DNS object contained in dns_obj_any_t union.
 *
 *	@discussion
 *		Use <code>MDNS_DISPOSE_DNS_OBJ()</code> provided by <code>mdns_strict.h</code> rather than the <code>dns_obj_release()</code>,
 *		because the macro checks the nullability of the pointer and always set the pointer to NULL after releasing.
 */
void
dns_obj_release(dns_obj_any_t dns_object);

/*!
 *	@brief
 *		Check if two supported DNS objects are equal or not, based on the definition of the comparator of the object.
 *
 *	@param dns_object1
 *		The supported DNS object to check the equality.
 *
 *	@param dns_object2
 *		The supported DNS object to check the equality.
 *
 *	@result
 *		True if two objects have the same kind and the defined comparator indicates that they are equal, or the two objects are pointing to the same object
 *		instance. Otherwise, false.
 *
 *	@discussion
 *		If the kind of the two objects has no comparator defined, the comparator of the super kind will be used to determine their equality. Such process will
 *		continue until one available comparator is found or the root kind (NULL) is reached. If no comparator is available for the current kind, the result will be
 *		false.
 */
bool
dns_obj_equal(dns_obj_any_t dns_object1, dns_obj_any_t dns_object2);

/*!
 *	@brief
 *		Compare two supported DNS objects, based on the definition of the comparator of the object.
 *
 *	@param dns_object1
 *		The supported DNS object to compare.
 *
 *	@param dns_object2
 *		The supported DNS object to compare.
 *
 *	@result
 *		<code>compare_result_less</code> if <code>dns_object1</code> is less than <code>dns_object2</code>.
 *		<code>compare_result_equal</code> if <code>dns_object1</code> is equal to <code>dns_object2</code>.
 *		<code>compare_result_greater</code> if <code>dns_object1</code> is greater than <code>dns_object2</code>.
 *		<code>compare_result_notequal</code> if two objects can be compared and they are not equal, but the specific order of the two objects cannot be determined.
 *		<code>compare_result_unknown</code> if two objects have different kind or no any comparator available to determine the comparison result, or any
 *			unexpected cases defined by the comparator.
 *
 *	@discussion
 *		If the kind of the two objects has no comparator defined, the comparator of the super kind will be used to determine their equality. Such process will
 *		continue until one available comparator is found or the root kind (NULL) is reached. If no comparator is available for the current kind, the result will be
 *		<code>compare_result_unknown</code>.
 *
 *		When the caller only wants know the equality of the two objects, use <code>dns_obj_equal()</code> instead of <code>dns_obj_compare()</code>
 *		because the former is faster than the latter.
 */
compare_result_t
dns_obj_compare(dns_obj_any_t dns_object1, dns_obj_any_t dns_object2);

/*!
 *	@brief
 *		Sort the DNS objects array by the ascending or descending order.
 *
 *	@param dns_objects
 *		The DNS object array to be sorted.
 *
 *	@param count
 *		The number of objects in the array.
 *
 *	@param order
 *		The order of the sorted array, it can be
 *		<code>sort_order_ascending</code> for the ascending order.
 *		<code>sort_order_descending</code> for the descending order.
 *
 *	@discussion
 *		The object has to have a comparator that can determine the specific order of the two objects that have the same kind, in order to be sortable. If no
 *		comparator is available or the comparator can only determine the equality of the objects, the array will be returned untouched.
 */
void
dns_objs_sort(dns_objs_any_t dns_objects, size_t count, sort_order_t order);

#define MDNS_DISPOSE_DNS_OBJ(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, dns_obj_release)

#define dns_obj_replace(PTR, OBJ)			\
	do {									\
		if ((OBJ) != NULL) {				\
			dns_obj_retain(OBJ);			\
		}									\
		if (*(PTR)) {						\
			MDNS_DISPOSE_DNS_OBJ(*(PTR));	\
		}									\
		*(PTR) = (OBJ);						\
	} while(0)

#define dns_obj_forget(PTR)					\
	do {									\
		if (*(PTR)) {						\
			dns_obj_release(*(PTR));		\
			*(PTR) = NULL;					\
		}									\
	} while(0)

#endif // DNS_OBJECT_H
