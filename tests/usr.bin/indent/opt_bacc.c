/* $NetBSD: opt_bacc.c,v 1.9 2022/04/24 08:52:44 rillig Exp $ */

/*
 * Tests for the options '-bacc' and '-nbacc' ("blank line around conditional
 * compilation").
 *
 * The option '-bacc' forces a blank line around every conditional compilation
 * block.  For example, in front of every #ifdef and after every #endif.
 * Other blank lines surrounding such blocks are swallowed.
 *
 * The option '-nbacc' TODO.
 */


/* Example code without surrounding blank lines. */
#indent input
int		a;
#if 0
int		b;
#endif
int		c;
#indent end

/*
 * XXX: As of 2021-11-19, the option -bacc has no effect on declarations since
 * process_type resets out.blank_line_before unconditionally.
 */
#indent run -bacc
int		a;
/* $ FIXME: expecting a blank line here */
#if 0
int		b;
#endif
/* $ FIXME: expecting a blank line here */
int		c;
#indent end

/*
 * With '-nbacc' the code is unchanged since there are no blank lines to
 * remove.
 */
#indent run-equals-input -nbacc


/* Example code containing blank lines. */
#indent input
int		space_a;

#if 0

int		space_b;

#endif

int		space_c;
#indent end

#indent run -bacc
int		space_a;
/* $ FIXME: expecting a blank line here */
#if 0

/* $ FIXME: expecting NO blank line here */
int		space_b;
#endif

int		space_c;
#indent end

/* The option '-nbacc' does not remove anything. */
#indent run-equals-input -nbacc


/*
 * Preprocessing directives can also occur in function bodies.
 */
#indent input
const char *
os_name(void)
{
#if defined(__NetBSD__) || defined(__FreeBSD__)
	return "BSD";
#else
	return "unknown";
#endif
}
#indent end

#indent run -bacc
const char *
os_name(void)
{
/* $ FIXME: expecting a blank line here. */
#if defined(__NetBSD__) || defined(__FreeBSD__)
/* $ FIXME: expecting NO blank line here. */

	return "BSD";
#else
/* $ FIXME: expecting NO blank line here. */

	return "unknown";
#endif
/* $ FIXME: expecting a blank line here. */
}
#indent end

#indent run-equals-input -nbacc


/*
 * Test nested preprocessor directives.
 */
#indent input
#if outer
#if inner
int decl;
#endif
#endif
#indent end

#indent run -di0 -bacc
#if outer

#if inner
int decl;
#endif

#endif
#indent end

#indent run-equals-input -di0 -nbacc


/*
 * Test nested preprocessor directives that are interleaved with declarations.
 */
#indent input
#ifdef outer
int outer_above;
#ifdef inner
int inner;
#endif
int outer_below;
#endif
#indent end

#indent run-equals-input -di0 -bacc

#indent run-equals-input -di0 -nbacc
