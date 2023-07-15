/*	$NetBSD: c23.c,v 1.5 2023/07/15 16:17:38 rillig Exp $	*/
# 3 "c23.c"

// Tests for the option -Ac23, which allows features from C23 and all earlier
// ISO standards, but none of the GNU extensions.
//
// See also:
//	msg_353.c		for empty initializer braces

/* lint1-flags: -Ac23 -w -X 351 */

int
empty_initializer_braces(void)
{
	struct s {
		int member;
	} s;

	// Empty initializer braces were introduced in C23.
	s = (struct s){};
	s = (struct s){s.member};
	return s.member;
}

// The keyword 'thread_local' was introduced in C23.
thread_local int globally_visible;

// Thread-local functions don't make sense; lint allows them, though.
thread_local void
thread_local_function(void)
{
}

void
function(void)
{
	// Not sure whether it makes sense to have a function-scoped
	// thread-local variable.  Don't warn for now, let the compilers handle
	// this case.
	thread_local int function_scoped_thread_local;
}

// 'extern' and 'thread_local' can be combined.  The other storage classes
// cannot be combined.
extern thread_local int extern_thread_local_1;
thread_local extern int extern_thread_local_2;
