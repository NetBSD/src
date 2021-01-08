/*	$NetBSD: msg_204.c,v 1.2 2021/01/08 01:40:03 rillig Exp $	*/
# 3 "msg_204.c"

// Test for message: controlling expressions must have scalar type [204]

extern void
extern_function(void);

void (*function_pointer)(void);

/*
 * Since func.c 1.39 from 2020-12-31 18:51:28, lint had produced an error
 * when a controlling expression was a function.  Pointers to functions were
 * ok though.
 */
void
bug_between_2020_12_31_and_2021_01_08(void)
{
	if (extern_function)
		extern_function();

	/*
	 * FIXME: For some reason, the ampersand is discarded in
	 *  build_ampersand.  This only has a visible effect if the
	 *  t_spec in check_controlling_expression is evaluated too early,
	 *  as has been the case before func.c 1.52 from 2021-01-08.
	 */
	if (&extern_function)
		extern_function();

	/* This one has always been ok since pointers are scalar types. */
	if (function_pointer)
		function_pointer();
}
