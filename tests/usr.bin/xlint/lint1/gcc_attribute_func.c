/*	$NetBSD: gcc_attribute_func.c,v 1.1 2021/07/06 17:33:07 rillig Exp $	*/
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

/* just to trigger _some_ error, to keep the .exp file */
/* expect+1: error: syntax error 'syntax_error' [249] */
__attribute__((syntax_error));
