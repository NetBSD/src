/* $NetBSD: opt_dj.c,v 1.9 2023/06/26 12:21:18 rillig Exp $ */

/*
 * Tests for the options '-dj' and '-ndj'.
 *
 * The option '-dj' left-justifies declarations of local variables.
 *
 * The option '-ndj' indents declarations the same as code.
 */

/* For top-level declarations, '-dj' and '-ndj' produce the same output. */
//indent input
int i;
int *ip;
const char *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
//indent end

//indent run -dj
int		i;
int	       *ip;
const char     *ccp;
const void *****vppppp;
const void ******vpppppp;
const void ********vpppppppp;
//indent end

//indent run-equals-prev-output -ndj


//indent input
void example(void) {
	int decl;
	code();
}
//indent end

//indent run -dj
void
example(void)
{
int		decl;
	code();
}
//indent end

//indent run -ndj
void
example(void)
{
	int		decl;
	code();
}
//indent end


/*
 * The option '-dj' does not influence traditional function definitions.
 */
//indent input
double
dbl_plus3(a, b, c)
double a, b, c;
{
	return a + b + c;
}
//indent end

//indent run -dj
double
dbl_plus3(a, b, c)
	double		a, b, c;
{
	return a + b + c;
}
//indent end


//indent input
struct a {
	struct b {
		struct c {
			struct d1 {
				int e;
			} d1;
			struct d2 {
				int e;
			} d2;
		} c;
	} b;
};
//indent end

//indent run -d0
struct a {
	struct b {
		struct c {
			struct d1 {
				int		e;
			}		d1;
			struct d2 {
				int		e;
			}		d2;
		}		c;
	}		b;
};
//indent end

//indent run-equals-prev-output -dj

//indent run-equals-input -di0


//indent input
{
	{
		struct a {
			struct b {
				struct c {
					struct d1 {
						int e;
					} d1;
					struct d2 {
						int e;
					} d2;
				} c;
			} b;
		};
	}
}
//indent end

//indent run -dj
{
	{
struct a {
	struct b {
		struct c {
			struct d1 {
				int		e;
			}		d1;
			struct d2 {
				int		e;
			}		d2;
		}		c;
	}		b;
};
	}
}
//indent end
