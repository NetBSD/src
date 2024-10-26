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

#ifndef REF_COUNT_H
#define REF_COUNT_H

//======================================================================================================================
// MARK: - Headers

//#include "dnssec_common.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>	// For offsetof().

#include "general.h"
#include "nullability.h"
#include "dns_assert_macros.h"

//======================================================================================================================
// MARK: - General Object Helpers

#define OBJECT_TYPEDEF_OPAQUE_POINTER(NAME) typedef struct NAME ## _s * NAME ## _t

// Check if structure `struct <NAME>_s` starts with structure `struct <BASE_NAME>_s <BASE_NAME>;` as the first member.
#define OBJECT_BASE_CHECK(NAME, BASE_NAME) 															\
	check_compile_time(offsetof(struct NAME ## _s, base) == 0);										\
	check_compile_time(sizeof(((struct NAME ## _s *)0)->base) == sizeof(struct BASE_NAME ## _s));	\
	extern int8_t _obj_base_type_check[sizeof(&(((NAME ## _t)0)->base) == ((BASE_NAME ## _t)0))]	\

#ifndef STRUCT_PTR_DECLARE
	#define STRUCT_PTR_DECLARE(STRUCT_NAME) struct STRUCT_NAME ## _s * NULLABLE STRUCT_NAME
#endif

#ifndef STRUCT_ARRAY_PTR_DECLARE
	#define STRUCT_ARRAY_PTR_DECLARE(STRUCT_NAME) struct STRUCT_NAME ## _s * NULLABLE * NULLABLE STRUCT_NAME ## s
#endif

typedef enum {
	compare_result_less		= -1,
	compare_result_equal	= 0,
	compare_result_greater	= 1,
	compare_result_notequal,	// Can ensure that the objects being compared are not equal. Do not know about the exact order.
	compare_result_unknown,		// Unable to determine the comparison result.
} compare_result_t;

typedef enum {
	sort_order_ascending,
	sort_order_descending
} sort_order_t;

//======================================================================================================================
// MARK: - Reference Count Object Helper Macros

// The macros below provide a way to define a reference counted object that comes with comparator and finalizer.
// To define a new reference counted object:
// 1. In the definition of the `ref_count_obj_any_t`, add `REF_COUNT_OBJECT_DECLARE_SUPPORTED_FAMILY(<object family name>);`
//	  to declare that the object family supports reference count and all the methods provided by `ref_count.h` can be
//	  used by the object family.
// 2. Create a header/source file for the object family, define the retain method, release method and compare method for
//	  the object family. This step ensures that all the objects under the same family can be retained, released and
//	  compared, without having to call the underlying ref_count object base. This provides an additional layer of type
//	  check.
// 3. In the definition of the `ref_count_obj_any_t`, add `REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(<object family name>,
//	  <object type name>);`. This step ensures that this new reference counted object supports the comparator and the
//	  finalizer defined by `ref_count.h`.
// 4. Put `REF_COUNT_OBJECT_DEFINE_FULL(<object family name>, <object type name>);` in the .c file of the object.
// 5. Define a structure with name of `struct <object family name>_<object type name>_s` in the .c file of the object.
//	  The structure should start with `struct ref_count_obj_s	base;` so that it can have reference count and object
//	  property.
// 6. Implement the comparator and the finalizer in the .c file of the object.
//
// Note: If an object family has been defined previously, start from step 3.

// Declare a initializer for the reference counted object.
#define REF_COUNT_OBJECT_DECLARE_INITIALIZER(FAMILY_NAME, NAME)									\
	static void																					\
	_ ## FAMILY_NAME ## _ ## NAME ## _initialize(FAMILY_NAME ## _ ## NAME ## _t NONNULL object)

//======================================================================================================================


// Declare a comparator for the reference counted object.
#define REF_COUNT_OBJECT_DECLARE_COMPARATOR(FAMILY_NAME, NAME)									\
	static compare_result_t																		\
	_ ## FAMILY_NAME ## _ ## NAME ## _compare(FAMILY_NAME ## _ ## NAME ## _t NONNULL object1,	\
											  FAMILY_NAME ## _ ## NAME ## _t NONNULL object2,	\
											  bool check_equality_only)

//======================================================================================================================

// Declare a finalizer for the reference counted object.
#define REF_COUNT_OBJECT_DECLARE_FINALIZER(FAMILY_NAME, NAME)									\
	static void																					\
	_ ## FAMILY_NAME ## _ ## NAME ## _finalize(FAMILY_NAME ## _ ## NAME ## _t NONNULL object)

//======================================================================================================================

// Define a New() function for the reference count object to allocate the memory and initialize the memory.
#define REF_COUNT_OBJECT_DEFINE_NEW_FUNC(FAMILY_NAME, NAME)									\
	static FAMILY_NAME ## _ ## NAME ## _t													\
	_ ## FAMILY_NAME ## _ ## NAME ## _new(void)												\
	{																						\
		const FAMILY_NAME ## _ ## NAME ## _t obj = (FAMILY_NAME ## _ ## NAME ## _t)ref_count_obj_alloc(sizeof(*obj));\
		if (obj == NULL) {																	\
			return NULL;																	\
		}																					\
																							\
		ref_count_obj_init(obj, &_ ## FAMILY_NAME ## _ ## NAME ## _kind);					\
		ref_count_obj_retain(obj);															\
		return obj;																			\
	}																						\
	extern int8_t _dummy_variable_to_enforce_semicolon

//======================================================================================================================

// Define the kind instance of the reference count object.
#define REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE_BASIC(FAMILY_NAME, NAME, ...)				\
	const struct ref_count_kind_s _ ## FAMILY_NAME ## _ ## NAME ## _kind = { 			\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN()	\
		.superkind	= &ref_count_kind,													\
		.name		= # FAMILY_NAME "_" # NAME,											\
		.finalize	= _ ## FAMILY_NAME ## _ ## NAME ## _finalize,						\
		__VA_ARGS__																		\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()		\
	};																					\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## NAME, ref_count_obj)

#define REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE(FAMILY_NAME, NAME)				\
	REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE_BASIC(FAMILY_NAME, NAME,				\
		.compare	= _ ## FAMILY_NAME ## _ ## NAME ## _compare,				\
	)

#define REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE_WITH_INIT(FAMILY_NAME, NAME)		\
	REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE_BASIC(FAMILY_NAME, NAME,				\
		.init = _ ## FAMILY_NAME ## _ ## NAME ## _initialize					\
	)

//======================================================================================================================

// Define a reference count enabled object.
#define REF_COUNT_OBJECT_DEFINE_FULL(FAMILY_NAME, NAME)			\
	REF_COUNT_OBJECT_DECLARE_COMPARATOR(FAMILY_NAME, NAME);		\
	REF_COUNT_OBJECT_DECLARE_FINALIZER(FAMILY_NAME, NAME);		\
	REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE(FAMILY_NAME, NAME);	\
	REF_COUNT_OBJECT_DEFINE_NEW_FUNC(FAMILY_NAME, NAME)

#define REF_COUNT_OBJECT_DEFINE_WITH_INIT_WITHOUT_COMPARATOR(FAMILY_NAME, NAME)	\
	REF_COUNT_OBJECT_DECLARE_INITIALIZER(FAMILY_NAME, NAME);					\
	REF_COUNT_OBJECT_DECLARE_FINALIZER(FAMILY_NAME, NAME);						\
	REF_COUNT_OBJECT_DEFINE_KIND_INSTANCE_WITH_INIT(FAMILY_NAME, NAME);			\
	REF_COUNT_OBJECT_DEFINE_NEW_FUNC(FAMILY_NAME, NAME)


//======================================================================================================================
// MARK: - Subkind of Reference Counted Object Helper Macros

// The macros below provide a way to define an object that is a subkind of the reference count object that is defined by
// the macros above.

// Define a new kind type for the reference count object so that its subkind can reference it. `...` are the additional
// members needed for the kind type.
// This is the way how the subkind know what super kind it has.
#define REF_COUNT_OBJECT_DEFINE_KIND_TYPE_FOR_SUBKIND(FAMILY_NAME, NAME, ...)							\
	typedef const struct FAMILY_NAME ## _ ## NAME ## _kind_s *	FAMILY_NAME ## _ ## NAME ## _kind_t;	\
	struct FAMILY_NAME ## _ ## NAME ## _kind_s {														\
		struct ref_count_kind_s	base;																	\
		FAMILY_NAME ## _ ## NAME ## _init_fields_f NONNULL FAMILY_NAME ## _ ## NAME ## _init_fields;	\
		__VA_ARGS__																						\
	};																									\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## NAME ## _kind, ref_count_kind);								\
	extern const struct ref_count_kind_s _ ## FAMILY_NAME ## _ ## NAME ## _kind

//======================================================================================================================

// Define the kind instance for the subkind of the reference count object. The reference count object has to exist
// before its subkind can be defined. There are four variations of the helper:
// 1. REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_ABSTRUCT: the subkind does not need to implement comparator and finalizer.
// 2. REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_FINALIZER: the subkind does not need to implement finalizer,
//	  but it needs to implement comparator.
// 3. REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_COMPARATOR: the subkind does not need to implement
//	  comparator, but it needs to implement finalizer.
// 4. REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_FULL: the subkind needs to implement both comparator and finalizer.
//
// No comparator subkind use case: it means that the super kind of the subkind already has a comparator and no more
// specific comparator needed.
// No finalizer subkind use case: it means that the subkind does not allocate new memory for its new member when doing
// member initialization. Therefore, there is no need for such subkind to define a finalizer that does nothing.

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_ABSTRUCT(FAMILY_NAME, SUPER, NAME, ...)					\
	const struct FAMILY_NAME ## _ ## SUPER ## _kind_s _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _kind = {	\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN()							\
		.base = {																								\
			.superkind	= &_ ## FAMILY_NAME ## _ ## SUPER ## _kind,												\
			.name 		= # FAMILY_NAME "_" # SUPER "_" # NAME,													\
		},																										\
		.FAMILY_NAME ## _ ## SUPER ## _init_fields = FAMILY_NAME ## _ ## SUPER ## _init_fields,					\
		__VA_ARGS__																								\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()								\
	};																											\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## SUPER ## _ ## NAME, FAMILY_NAME ## _ ## SUPER)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_FINALIZER(FAMILY_NAME, SUPER, NAME, ...)			\
	const struct FAMILY_NAME ## _ ## SUPER ## _kind_s _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _kind = {	\
		.base = {																								\
			.superkind	= &_ ## FAMILY_NAME ## _ ## SUPER ## _kind,												\
			.name 		= # FAMILY_NAME "_" # SUPER "_" # NAME,													\
			.compare	= _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _compare,								\
		},																										\
		.FAMILY_NAME ## _ ## SUPER ## _init_fields = FAMILY_NAME ## _ ## SUPER ## _init_fields,					\
		__VA_ARGS__																								\
	};																											\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## SUPER ## _ ## NAME, FAMILY_NAME ## _ ## SUPER)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_COMPARATOR(FAMILY_NAME, SUPER, NAME, ...)			\
	const struct FAMILY_NAME ## _ ## SUPER ## _kind_s _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _kind = {	\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN()							\
		.base = {																								\
			.superkind	= &_ ## FAMILY_NAME ## _ ## SUPER ## _kind,												\
			.name 		= # FAMILY_NAME "_" # SUPER "_" # NAME,													\
			.finalize	= _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _finalize								\
		},																										\
		.FAMILY_NAME ## _ ## SUPER ## _init_fields = FAMILY_NAME ## _ ## SUPER ## _init_fields,					\
		__VA_ARGS__																								\
		MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()								\
	};																											\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## SUPER ## _ ## NAME, FAMILY_NAME ## _ ## SUPER)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_FULL(FAMILY_NAME, SUPER, NAME, ...)						\
	const struct FAMILY_NAME ## _ ## SUPER ## _kind_s _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _kind = {	\
		.base = {																								\
			MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_BEGIN()						\
			.superkind	= &_ ## FAMILY_NAME ## _ ## SUPER ## _kind,												\
			.name 		= # FAMILY_NAME "_" # SUPER "_" # NAME,													\
			.compare	= _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _compare,								\
			.finalize	= _ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _finalize								\
			MDNS_CLANG_IGNORE_INCOMPATIBLE_FUNCTION_POINTER_TYPES_STRICT_WARNING_END()							\
		},																										\
		.FAMILY_NAME ## _ ## SUPER ## _init_fields = FAMILY_NAME ## _ ## SUPER ## _init_fields,					\
		__VA_ARGS__																								\
	};																											\
	OBJECT_BASE_CHECK(FAMILY_NAME ## _ ## SUPER ## _ ## NAME, FAMILY_NAME ## _ ## SUPER)

//======================================================================================================================

// Define a New() function for the subkind of reference count object to allocate the memory and initialize the memory.
#define REF_COUNT_OBJECT_SUBKIND_DEFINE_NEW_FUNC(FAMILY_NAME, SUPER, NAME)						\
	static FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _t											\
	_ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _new(void)									\
	{																							\
		const FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _t obj = (FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _t)ref_count_obj_alloc(sizeof(*obj));\
		if (obj == NULL) {																		\
			return NULL;																		\
		}																						\
																								\
		ref_count_obj_init(obj, &_ ## FAMILY_NAME ## _ ## SUPER ## _ ## NAME ## _kind.base);	\
		ref_count_obj_retain(obj);																\
		return obj;																				\
	}																							\
	extern int8_t _dummy_variable_to_enforce_semicolon

//======================================================================================================================

// Define a subkind object of the reference counted object, based on whether the subkind needs the comparator and
// the finalizer. `...` are the additional member that the kind type of reference counted object needs to initialize
// for this subkind.
#define REF_COUNT_OBJECT_SUBKIND_DEFINE_FULL(FAMILY_NAME, SUPER, NAME, ...)						\
	REF_COUNT_OBJECT_DECLARE_COMPARATOR(FAMILY_NAME, SUPER ## _ ## NAME);						\
	REF_COUNT_OBJECT_DECLARE_FINALIZER(FAMILY_NAME, SUPER ## _ ## NAME);						\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_FULL(FAMILY_NAME, SUPER, NAME, __VA_ARGS__);	\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_NEW_FUNC(FAMILY_NAME, SUPER, NAME)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_WITHOUT_COMPARATOR(FAMILY_NAME, SUPER, NAME, ...)						\
	REF_COUNT_OBJECT_DECLARE_FINALIZER(FAMILY_NAME, SUPER ## _ ## NAME);										\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_COMPARATOR(FAMILY_NAME, SUPER, NAME, __VA_ARGS__);	\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_NEW_FUNC(FAMILY_NAME, SUPER, NAME)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_WITHOUT_FINALIZER(FAMILY_NAME, SUPER, NAME, ...)						\
	REF_COUNT_OBJECT_DECLARE_COMPARATOR(FAMILY_NAME, SUPER ## _ ## NAME);										\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_WITHOUT_FINALIZER(FAMILY_NAME, SUPER, NAME, __VA_ARGS__);		\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_NEW_FUNC(FAMILY_NAME, SUPER, NAME)

#define REF_COUNT_OBJECT_SUBKIND_DEFINE_ABSTRUCT(FAMILY_NAME, SUPER, NAME, ...)						\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_KIND_INSTANCE_ABSTRUCT(FAMILY_NAME, SUPER, NAME, __VA_ARGS__);	\
	REF_COUNT_OBJECT_SUBKIND_DEFINE_NEW_FUNC(FAMILY_NAME, SUPER, NAME)

//======================================================================================================================
// MARK: - Reference Counted Object Declaration

#define REF_COUNT_OBJECT_DECLARE_SUPPORTED_FAMILY(FAMILY_NAME) 							STRUCT_PTR_DECLARE(FAMILY_NAME)
#define REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(FAMILY_NAME, NAME)					STRUCT_PTR_DECLARE(FAMILY_NAME ## _ ## NAME)
#define REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(FAMILY_NAME, SUPER, NAME)		REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(FAMILY_NAME, SUPER ## _ ## NAME)

#define REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_FAMILY(FAMILY_NAME) 						STRUCT_ARRAY_PTR_DECLARE(FAMILY_NAME)
#define REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(FAMILY_NAME, NAME)					STRUCT_ARRAY_PTR_DECLARE(FAMILY_NAME ## _ ## NAME)
#define REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(FAMILY_NAME, SUPER, NAME)	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(FAMILY_NAME, SUPER ## _ ## NAME)

typedef union {
	// The universal reference count enabled object.
	STRUCT_PTR_DECLARE(ref_count_obj);

	// -------- The DNS Object families --------
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_FAMILY(dns_obj);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_obj, domain_name);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_obj, rr);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, cname);	// CNAME RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, soa);	// SOA RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, srv);	// SRV RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, nsec);	// NSEC RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, ds);		// DS RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, rrsig);	// RRSIG RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, dnskey);	// DNSKEY RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dns_obj, rr, nsec3);	// NSEC3 RR

	// -------- The DNSSEC Object families --------
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_FAMILY(dnssec_obj);
	// The validation task manager object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, validation_manager);
	// The callback context object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, context);
	// The trust anchor object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, trust_anchor);
	// The trust anchor manager object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, trust_anchor_manager);
	// The domain name object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, domain_name);
	// The resource record set object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, rrset);
	// The resource record validator object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, rr_validator);
	// The denial of existence object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, denial_of_existence);
	// The resource record and its subkind object in the DNSSEC Object families.
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, rr);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, cname);	// CNAME RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, soa);		// SOA RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, srv);		// SRV RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, nsec);	// NSEC RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, ds);		// DS RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, rrsig);	// RRSIG RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, dnskey);	// DNSKEY RR
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, nsec3);	// NSEC3 RR

	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, resource_record_member);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dnssec_obj, dns_question_member);

	// -------- The DNS Push Object families --------
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_FAMILY(dns_push_obj);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_push_obj, context);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_push_obj, discovered_service_manager);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_push_obj, dns_question_member);
	REF_COUNT_OBJECT_DECLARE_SUPPORTED_OBJECT(dns_push_obj, resource_record_member);

} ref_count_obj_any_t __attribute__((__transparent_union__));

typedef union {
	// The universal reference count enabled objects array.
	STRUCT_ARRAY_PTR_DECLARE(ref_count_obj);

	// -------- The DNSSEC Object array families --------
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_FAMILY(dnssec_obj);
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(dnssec_obj, domain_name);			// domain names array

	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(dnssec_obj, rr);					// RRs array
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, nsec);		// NSEC RRs array
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, dnskey);	// DNSKEY RRs array
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(dnssec_obj, rr, nsec3);		// NSEC3 RRs array

	// -------- The DNS Object families --------
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_FAMILY(dns_obj);
	REF_COUNT_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(dns_obj, domain_name);				// domain names array

} ref_count_objs_any_t __attribute__((__transparent_union__));

//======================================================================================================================
// MARK: - The Reference Count Structure Base

typedef void
(*ref_count_init_f)(ref_count_obj_any_t object);

typedef compare_result_t
(*ref_count_compare_f)(ref_count_obj_any_t object1, ref_count_obj_any_t object2, bool check_equality_only);

typedef void
(*ref_count_finalize_f)(ref_count_obj_any_t object);

typedef const struct ref_count_kind_s *ref_count_kind_t;
struct ref_count_kind_s {
	ref_count_kind_t NULLABLE		superkind;	// Points to the kind instance of the super kind.
	const char * NONNULL			name;		// The name of the current kind. Just for information purpose.
	ref_count_init_f NULLABLE		init;		// The memory initialization method, most of time it will be NULL.
	ref_count_compare_f NULLABLE	compare;	// The comparator of the kind.
	ref_count_finalize_f NULLABLE	finalize;	// The finalizer of the kind.
};
// Note the difference between memory initialization and member(field) initialization, memory initialization will
// initialize the allocated memory to a default value, for example, zero. However, member(field) initialization will
// initialize the specific object's member to a meaningful value. Most of time, memory initialization is unnecessary
// since we are using calloc() to reset the allocated memory to zero, the memory initialization has been finished and no
// need to explicitly define an initializer. However, under some cases where zero is a meaningful value, for example,
// socket value 0. We have to explicitly define the memory initialization method so that the member can be set to an
// invalid value, such as -1 for socket. We need the member to have a INVALID value because the finalizer needs to know
// if it needs to do some clean up. If the member has an INVALID value, the finalizer does not need to free any resource
// possibly related to it. This allow us to release the object immediately after allocating memory for it, even before
// initializing its member. (the member initialization).

extern const struct ref_count_kind_s ref_count_kind; // ref_count_kind is defined in ref_count.c

typedef struct ref_count_obj_s * ref_count_obj_t;

struct ref_count_obj_s {
	uint32_t					ref_count;
	ref_count_kind_t NONNULL	kind;
};

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Allocate the memory for the reference counted object.
 *
 *	@param size
 *		The size of the object to be allocated.
 *
 *	@result
 *		The memory allocated for the object, or <code>NULL</code> if the memory allocation fails.
 */
ref_count_obj_t NULLABLE
ref_count_obj_alloc(size_t size);

/*!
 *	@brief
 *		Set the object's kind and do memory initialization. Under most of cases, memory initialization will not be performed. (zero initialization is already finished by
 *		<code>ref_count_obj_alloc()(</code>).
 *
 *	@param ref_count_object
 *		The newly allocated reference counted object.
 *
 *	@param new_kind
 *		The kind of this newly allocated reference counted object.
 *
 *	@discussion
 *		Any newly allocated object has to call this function to set its kind.
 */
void
ref_count_obj_init(ref_count_obj_any_t ref_count_object, ref_count_kind_t NONNULL new_kind);

/*!
 *	@brief
 *		Retain a reference counted object by increasing the reference count by one.
 *
 *	@param ref_count_object
 *		The supported reference counted object contained in <code>ref_count_obj_any_t</code> union.
 *
 *	@result
 *		The retained reference counted object.
 */
ref_count_obj_t NONNULL
ref_count_obj_retain(ref_count_obj_any_t ref_count_object);

/*!
 *	@brief
 *		Release a reference counted object by increasing the reference count by one. If the reference count becomes zero after releasing, the object will be
 *		finalized.
 *
 *	@param ref_count_object
 *		The supported reference counted object contained in <code>ref_count_obj_any_t</code> union.
 */
void
ref_count_obj_release(ref_count_obj_any_t ref_count_object);

/*!
 *	@brief
 *		Compare two reference counted objects, based on the definition of the comparator of the object.
 *
 *	@param ref_count_object1
 *		The supported reference counted object contained in <code>ref_count_obj_any_t</code> union.
 *
 *	@param ref_count_object2
 *		The supported reference counted object contained in <code>ref_count_obj_any_t</code> union.
 *
 *	@param check_equality_only
 *		Indicate whether the caller only wants to know if the two objects are equal or not. The comparison will be faster when it is true.
 *
 *	@result
 *		<code>compare_result_less</code> if <code>ref_count_object1</code> is less than <code>ref_count_object2</code>.
 *		<code>compare_result_equal</code> if <code>ref_count_object1</code> is equal to <code>ref_count_object2</code>.
 *		<code>compare_result_greater</code> if <code>ref_count_object1</code> is greater than <code>ref_count_object2</code>.
 *		<code>compare_result_notequal</code> if two objects can be compared and they are not equal, but the specific order of the two objects cannot be determined.
 *		<code>compare_result_unknown</code> if two objects have different kind or no any comparator available to determine the comparison result, or any unexpected
 *			cases defined by the comparator.
 *
 *	@discussion
 *		If the kind of the two objects has no comparator defined, the comparator of the super kind will be used to determine their equality. Such process will
 *		continue until one available comparator is found or the root kind (NULL) is reached. If no comparator is available for the current kind, the result will be
 *		<code>compare_result_unknown</code>.
 */
compare_result_t
ref_count_obj_compare(ref_count_obj_any_t ref_count_object1, ref_count_obj_any_t ref_count_object2, bool check_equality_only);

/*!
 *	@brief
 *		Sort the reference counted objects array by the ascending or descending order.
 *
 *	@param ref_count_objects
 *		The reference counted object array to be sorted.
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
ref_count_objs_sort(ref_count_objs_any_t ref_count_objects, size_t count, sort_order_t order);

#endif // REF_COUNT_H
