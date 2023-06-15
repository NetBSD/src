/* $NetBSD: opt_bacc.c,v 1.13 2023/06/15 09:19:07 rillig Exp $ */

/*
 * Tests for the options '-bacc' and '-nbacc' ("blank line around conditional
 * compilation").
 *
 * The option '-bacc' forces a blank line around every conditional compilation
 * block.  For example, in front of every #ifdef and after every #endif.
 * Other blank lines surrounding such blocks are swallowed.
 *
 * The option '-nbacc' leaves the vertical spacing as-is.
 */


/* Example code without surrounding blank lines. */
//indent input
int		a;
#if 0
int		b;
#endif
int		c;
//indent end

//indent run -bacc
int		a;

#if 0
int		b;
#endif

int		c;
//indent end

/* The option '-nbacc' does not remove anything. */
//indent run-equals-input -nbacc


/* Example code containing blank lines. */
//indent input
int		space_a;

#if 0

int		space_b;

#endif

int		space_c;
//indent end

//indent run-equals-input -bacc

/* The option '-nbacc' does not remove anything. */
//indent run-equals-input -nbacc


/*
 * Preprocessing directives can also occur in function bodies.
 */
//indent input
const char *
os_name(void)
{
#if defined(__NetBSD__) || defined(__FreeBSD__)
	return "BSD";
#else
	return "unknown";
#endif
}
//indent end

//indent run -bacc
const char *
os_name(void)
{

#if defined(__NetBSD__) || defined(__FreeBSD__)
	return "BSD";
#else
	return "unknown";
#endif

}
//indent end

//indent run-equals-input -nbacc


/*
 * Test nested preprocessor directives.
 */
//indent input
#if outer
#if inner
int decl;
#endif
#endif
//indent end

//indent run-equals-input -di0 -bacc

//indent run-equals-input -di0 -nbacc


/*
 * Test nested preprocessor directives that are interleaved with declarations.
 */
//indent input
#ifdef outer
int outer_above;
#ifdef inner
int inner;
#endif
int outer_below;
#endif
//indent end

//indent run -di0 -bacc
#ifdef outer
int outer_above;

#ifdef inner
int inner;
#endif

int outer_below;
#endif
//indent end

//indent run-equals-input -di0 -nbacc


//indent input
/* before */
#if 0
/* between if and else */
#else
#if 1
#endif
#endif
/* after */
//indent end

//indent run -bacc
/* before */
// $ XXX: The 'before' comment may refer to the '#if', so it is not obvious
// $ XXX: that this blank line is useful.

#if 0
/* between if and else */
#else
// $ XXX: This blank line looks unintended, as both lines are preprocessing
// $ XXX: directives.

#if 1
#endif
#endif

/* after */
//indent end
