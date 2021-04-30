/*	$NetBSD: gcc_attribute.c,v 1.1 2021/04/30 23:49:36 rillig Exp $	*/
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

/* expect+1: syntax error 'nonnull' */
void __attribute__((nonnull(1, 2)))
my_memcpy(void *dest, const void *src, unsigned long len);

/* expect+1: syntax error 'unknown_attribute' */
void __attribute__((unknown_attribute))
function_with_unknown_attribute(void);
