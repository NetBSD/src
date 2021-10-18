/* $NetBSD: opt_dj.c,v 1.3 2021/10/18 07:11:31 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-dj' and '-ndj'.
 *
 * The option '-dj' left-justifies declarations.
 *
 * The option '-ndj' indents declarations the same as code.
 */

/* For top-level declarations, '-dj' and '-ndj' produce the same output. */
#indent input
int i;
int *ip;
const char *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end

#indent run -dj
int		i;
int	       *ip;
const char     *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
#indent end

#indent run-equals-prev-output -ndj

#indent input
void example(void) {
	int decl;
	code();
}
#indent end

#indent run -dj
void
example(void)
{
int		decl;
	code();
}
#indent end

#indent run -ndj
void
example(void)
{
	int		decl;
	code();
}
#indent end
