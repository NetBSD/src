/* $NetBSD: lsym_period.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the token lsym_period, which represents '.' in these contexts:
 *
 * In an initializer, '.' starts a named designator (since C99).
 *
 * In an expression, 'sou.member' accesses the member 'member' in the struct
 * or union 'sou'.
 *
 * In a function prototype declaration, the sequence '.' '.' '.' marks the
 * start of a variable number of arguments.  It would have been more intuitive
 * to model them as a single token, but it doesn't make any difference for
 * formatting the code.
 *
 * See also:
 *	lsym_word.c		for '.' inside numeric constants
 */

/* Designators in an initialization */
//indent input
struct point {
	int x;
	int y;
} p = {
	.x = 3,
	.y = 4,
};
//indent end

//indent run-equals-input -di0


/* Accessing struct members */
//indent input
time_t
get_time(struct stat st)
{
	return st.st_mtime > 0 ? st . st_atime : st.st_ctime;
}
//indent end

//indent run
time_t
/* $ FIXME: The '{' must be in the next line. */
get_time(struct stat st){
	return st.st_mtime > 0 ? st.st_atime : st.st_ctime;
}
//indent end

//indent run -Ttime_t
time_t
get_time(struct stat st)
{
	return st.st_mtime > 0 ? st.st_atime : st.st_ctime;
}
//indent end


/* Varargs in a function declaration */
//indent input
void my_printf(const char *, ...);
//indent end

//indent run-equals-input -di0
