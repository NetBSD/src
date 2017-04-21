/*	$NetBSD: check_arg.h,v 1.2.4.2 2017/04/21 16:52:52 bouyer Exp $	*/

#ifndef _CHECK_ARG_INCLUDED_
#define _CHECK_ARG_INCLUDED_

/*++
/* NAME
/*	check_arg 3h
/* SUMMARY
/*	type checking/narrowing/widening for unprototyped arguments
/* SYNOPSIS
/*	#include <check_arg.h>
/*
/*	/* Example checking infrastructure for int, int *, const int *. */
/*	CHECK_VAL_HELPER_DCL(tag, int);
/*	CHECK_PTR_HELPER_DCL(tag, int);
/*	CHECK_CPTR_HELPER_DCL(tag, int);
/*
/*	/* Example variables with type int, int *, const int *. */
/*	int int_val;
/*	int *int_ptr;
/*	const int *int_cptr;
/*
/*	/* Example variadic function with type-flag arguments. */
/*	func(FLAG_INT_VAL, CHECK_VAL(tag, int, int_val),
/*	     FLAG_INT_PTR, CHECK_PTR(tag, int, int_ptr),
/*	     FLAG_INT_CPTR, CHECK_CPTR(tag, int, int_cptr)
/*	     FLAG_END);
/* DESCRIPTION
/*	This module implements wrappers for unprototyped function
/*	arguments, to enable the same type checking, type narrowing,
/*	and type widening as for prototyped function arguments. The
/*	wrappers may also be useful in other contexts.
/*
/*	Typically, these wrappers are hidden away in a per-module
/*	header file that is read by the consumers of that module.
/*	To protect consumers against name collisions between wrappers
/*	in different header files, wrappers should be called with
/*	a distinct per-module tag value.  The tag syntax is that
/*	of a C identifier.
/*
/*	Use CHECK_VAL(tag, type, argument) for arguments with a
/*	basic type: int, long, etc., and types defined with "typedef"
/*	where indirection is built into the type itself (for example,
/*	the result of "typedef int *foo" or function pointer
/*	typedefs).
/*
/*	Use CHECK_PTR(tag, type, argument) for non-const pointer
/*	arguments, CHECK_CPTR(tag, type, argument) for const pointer
/*	arguments, and CHECK_PPTR(tag, type, argument) for pointer-
/*	to-pointer arguments.
/*
/*	Use CHECK_*_HELPER_DCL(tag, type) to provide the
/*	checking infrastructure for all CHECK_*(tag, type, ...)
/*	instances with the same *, tag and type. Depending on
/*	the compilation environment, the infrastructure consists
/*	of an inline function definition or a dummy assignment
/*	target declaration.
/*
/*	The compiler should report the following problems:
/* .IP \(bu
/*	Const pointer argument where a non-const pointer is expected.
/* .IP \(bu
/*	Pointer argument where a non-pointer is expected and
/*	vice-versa.
/* .IP \(bu
/*	Pointer/pointer type mismatches except void/non-void pointers.
/*	Just like function prototypes, all CHECK_*PTR() wrappers
/*	cast their result to the desired type.
/* .IP \(bu
/*	Non-constant non-pointer argument where a pointer is expected.
/*. PP
/*	Just like function prototypes, the CHECK_*PTR() wrappers
/*	handle "bare" numerical constants by casting their argument
/*	to the desired pointer type.
/*
/*	Just like function prototypes, the CHECK_VAL() wrapper
/*	cannot force the caller to specify a particular non-pointer
/*	type and casts its argument value to the desired type which
/*	may wider or narrower than the argument value.
/* IMPLEMENTATION

 /*
  * Choose between an implementation based on inline functions (standardized
  * with C99) or conditional assignment (portable to older compilers, with
  * some caveats as discussed below).
  */
#ifndef NO_INLINE

 /*
  * Parameter checks expand into inline helper function calls.
  */
#define CHECK_VAL(tag, type, v) check_val_##tag##type(v)
#define CHECK_PTR(tag, type, v) check_ptr_##tag##type(v)
#define CHECK_CPTR(tag, type, v) check_cptr_##tag##type(v)
#define CHECK_PPTR(tag, type, v) check_pptr_##tag##type(v)

 /*
  * Macros to instantiate the inline helper functions.
  */
#define CHECK_VAL_HELPER_DCL(tag, type) \
	static inline type check_val_##tag##type(type v) { return v; }
#define CHECK_PTR_HELPER_DCL(tag, type) \
	static inline type *check_ptr_##tag##type(type *v) { return v; }
#define CHECK_CPTR_HELPER_DCL(tag, type) \
	static inline const type *check_cptr_##tag##type(const type *v) \
	    { return v; }
#define CHECK_PPTR_HELPER_DCL(tag, type) \
	static inline type **check_pptr_##tag##type(type **v) { return v; }

#else					/* NO_INLINE */

 /*
  * Parameter checks expand into unreachable conditional assignments.
  * Inspired by OpenSSL's verified pointer check, our implementation also
  * detects const/non-const pointer conflicts, and it also supports
  * non-pointer expressions.
  */
#define CHECK_VAL(tag, type, v) ((type) (1 ? (v) : (CHECK_VAL_DUMMY(type) = (v))))
#define CHECK_PTR(tag, type, v) ((type *) (1 ? (v) : (CHECK_PTR_DUMMY(type) = (v))))
#define CHECK_CPTR(tag, type, v) \
	((const type *) (1 ? (v) : (CHECK_CPTR_DUMMY(type) = (v))))
#define CHECK_PPTR(tag, type, v) ((type **) (1 ? (v) : (CHECK_PPTR_DUMMY(type) = (v))))

 /*
  * These macros instantiate assignment target declarations. Since the
  * assignment is made in unreachable code, the compiler "should" not emit
  * any references to those assignment targets. We use the "extern" class so
  * that gcc will not complain about unused variables. Using "extern" breaks
  * when a compiler does emit references unreachable assignment targets.
  * Hopefully, those cases will be rare.
  */
#define CHECK_VAL_HELPER_DCL(tag, type) extern type CHECK_VAL_DUMMY(type)
#define CHECK_PTR_HELPER_DCL(tag, type) extern type *CHECK_PTR_DUMMY(type)
#define CHECK_CPTR_HELPER_DCL(tag, type) extern const type *CHECK_CPTR_DUMMY(type)
#define CHECK_PPTR_HELPER_DCL(tag, type) extern type **CHECK_PPTR_DUMMY(type)

 /*
  * The actual dummy assignment target names.
  */
#define CHECK_VAL_DUMMY(type) check_val_dummy_##type
#define CHECK_PTR_DUMMY(type) check_ptr_dummy_##type
#define CHECK_CPTR_DUMMY(type) check_cptr_dummy_##type
#define CHECK_PPTR_DUMMY(type) check_pptr_dummy_##type

#endif					/* NO_INLINE */

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
