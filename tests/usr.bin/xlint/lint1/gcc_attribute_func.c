/*	$NetBSD: gcc_attribute_func.c,v 1.3 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "gcc_attribute_func.c"

/*
 * Tests for the GCC __attribute__ for functions.
 *
 * https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html
 */

void deprecated_function(void)
    __attribute__((__noreturn__))
    __attribute__((__aligned__(8), __cold__))
    __attribute__((__deprecated__("do not use while driving")));

__attribute__((__cold__))
void attribute_as_prefix(void);

void __attribute__((__cold__)) attribute_after_type_spec(void);
void *__attribute__((__cold__)) attribute_before_name(void);
/*TODO: do not allow __attribute__ after function name */
void *attribute_after_name __attribute__((__cold__))(void);
void *attribute_after_parameters(void) __attribute__((__cold__));

/*
 * The attribute 'used' does not influence static functions, it only
 * applies to function parameters.
 */
/* expect+2: warning: static function 'used_function' unused [236] */
static void __attribute__((used))
used_function(void)
{
}

/* expect+2: warning: static function 'unused_function' unused [236] */
static void
unused_function(void)
{
}
