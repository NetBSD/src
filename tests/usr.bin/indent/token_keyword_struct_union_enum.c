/* $NetBSD: token_keyword_struct_union_enum.c,v 1.5 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the keywords 'struct', 'union' and 'enum'.
 */

//indent input
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
//indent end

//indent run-equals-input


/* See FreeBSD r303485. */
//indent input
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
//indent end

//indent run
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
//indent end


/* Comment between 'struct' and the tag name; doesn't occur in practice. */
//indent input
struct   /* comment */   tag var;
//indent end

//indent run -di0
struct /* comment */ tag var;
//indent end
