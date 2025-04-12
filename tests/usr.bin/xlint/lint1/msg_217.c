/*	$NetBSD: msg_217.c,v 1.17 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_217.c"

// Test for message: function '%s' falls off bottom without returning value [217]

/* lint1-extra-flags: -X 351 */

int
random(int n)
{
	if (n < 0)
		return -3;
}
/* expect-1: warning: function 'random' falls off bottom without returning value [217] */

/*
 * The pattern 'do { } while (0)' is often used in statement macros.
 * Putting a 'return' at the end of such a macro is legitimate, the embracing
 * 'do { } while (0)' is probably there to conform to a coding standard or
 * to otherwise reduce confusion.
 *
 * Seen in external/bsd/libevent/dist/event_tagging.c, function
 * encode_int_internal.
 *
 * Before tree.c 1.243 from 2021-03-21, lint wrongly reported that the
 * 'while 0' was unreachable.  This has been fixed by allowing the 'while 0'
 * in a do-while-false loop to be unreachable.  The same could be useful for a
 * do-while-true.
 *
 * Before func.c 1.83 from 2021-03-21, lint wrongly reported that the function
 * would fall off the bottom.
 */
int
do_while_return(int i)
{
	do {
		return i;
	} while (0);
}

/*
 * C99 5.1.2.2.3 "Program termination" p1 defines that as a special exception,
 * the function 'main' does not have to return a value, reaching the bottom
 * is equivalent to returning 0.
 *
 * Before func.c 1.72 from 2021-02-21, lint had wrongly warned about this.
 */
int
main(void)
{
}

int
reachable_continue_leads_to_endless_loop(void)
{
	for (;;) {
		if (1)
			continue;
		break;
	}
}

int
unreachable_continue_falls_through(void)
{
	for (;;) {
		if (0)
			/* expect+1: warning: 'continue' statement not reached [193] */
			continue;
		break;
	}
}
/* expect-1: warning: function 'unreachable_continue_falls_through' falls off bottom without returning value [217] */


_Noreturn void noreturn_c11(void);
[[noreturn]] void noreturn_c23(void);
__attribute__((__noreturn__)) void noreturn_gnu_prefix(void);
void __attribute__((__noreturn__)) noreturn_gnu_infix(void);
void noreturn_gnu_suffix(void) __attribute__((__noreturn__));

int
call_noreturn_c11(void)
{
	noreturn_c11();
}

inline int
call_noreturn_c23(void)
{
	noreturn_c23();
}

int
call_noreturn_gnu_prefix(void)
{
	noreturn_gnu_prefix();
}

int
call_noreturn_gnu_infix(void)
{
	noreturn_gnu_infix();
}

int
call_noreturn_gnu_suffix(void)
{
	noreturn_gnu_suffix();
}


double *force_function_attributes_in_diagnostic[] = {
	// Force the word 'noreturn' to occur in a diagnostic.
	/* expect+1: warning: invalid combination of 'pointer to double' and 'pointer to noreturn function(void) returning void', op 'init' [124] */
	noreturn_c23,
	// The 'inline' does affect the type of the function.
	/* expect+1: warning: invalid combination of 'pointer to double' and 'pointer to function(void) returning int', op 'init' [124] */
	call_noreturn_c23,
};
