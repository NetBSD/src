/*	$NetBSD: gcc_attribute.c,v 1.2 2021/05/01 07:25:07 rillig Exp $	*/
# 3 "gcc_attribute.c"

/*
 * Tests for the various attributes for functions, types, statements that are
 * provided by GCC.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
 * https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Variable-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Type-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Enumerator-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Statement-Attributes.html
 * https://gcc.gnu.org/onlinedocs/gcc/Label-Attributes.html
 */

void __attribute__((noinline))
do_not_inline(void)
{
}

/* All pointer arguments must be nonnull. */
void __attribute__((nonnull))
function_nonnull(void *, const void *, int);

/* Arguments 1 and 2 must be nonnull. */
void __attribute__((nonnull(1, 2)))
function_nonnull_list(void *, const void *, int);

/* expect+1: syntax error 'unknown_attribute' */
void __attribute__((unknown_attribute))
function_with_unknown_attribute(void);
