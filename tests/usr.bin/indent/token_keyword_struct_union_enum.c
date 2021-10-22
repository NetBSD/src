/* $NetBSD: token_keyword_struct_union_enum.c,v 1.2 2021/10/22 19:46:41 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the keywords 'struct', 'union' and 'enum'.
 */

#indent input
struct stat {
	mode_t		st_mode;
};

union variant {
	enum {
	}		tag;
	int		v_int;
	long		v_long;
	bool		v_bool;
	void	       *v_pointer;
};
#indent end

#indent run-equals-input


/* See FreeBSD r303485. */
/* $FreeBSD: head/usr.bin/indent/tests/struct.0 334564 2018-06-03 16:21:15Z pstef $ */
#indent input
int f(struct x *a);

void
t(void)
{
	static const struct {
		int	a;
		int	b;
	} c[] = {
		{ D, E },
		{ F, G }
	};
}

void u(struct x a) {
	int b;
	struct y c = (struct y *)&a;
}
#indent end

#indent run
int		f(struct x *a);

void
t(void)
{
	static const struct {
		int		a;
		int		b;
	}		c[] = {
		{D, E},
		{F, G}
	};
}

void
u(struct x a)
{
	int		b;
	struct y	c = (struct y *)&a;
}
#indent end
