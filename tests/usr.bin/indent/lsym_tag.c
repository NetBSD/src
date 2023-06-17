/* $NetBSD: lsym_tag.c,v 1.10 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for the token lsym_tag, which represents one of the keywords
 * 'struct', 'union' or 'enum' that declare, define or use a tagged type.
 */

/* TODO: Add systematic tests for 'struct'. */
/* TODO: Add systematic tests for 'union'. */
/* TODO: Add systematic tests for 'enum'. */


//indent input
int
indent_enum_constants(void)
{
	enum color {
		red,
		green
	};
	enum color	colors[] = {
		red,
		red,
		red,
	};
	/*
	 * Ensure that the token sequence 'enum type {' only matches if there
	 * are no other tokens in between, to prevent statement continuations
	 * from being indented like enum constant declarations.
	 *
	 * See https://gnats.netbsd.org/55453.
	 */
	if (colors[0] == (enum color)1) {
		return 1
		  + 2
		  + 3;
	}
	return 0;
}
//indent end

//indent run-equals-input -ci2


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


/*
 * Ensure that the names of struct members are all indented the same.
 * Before 2023-05-15, the indentation depended on their type name.
 */
//indent input
struct outer {
	enum {
		untagged_constant,
	} untagged_member,
	  second_untagged_member;
	enum tag_name {
		tagged_constant,
	} tagged_member,
	  second_tagged_member;
};
//indent end

//indent run-equals-input -di0


/*
 * The initializer of an enum constant needs to be indented like any other
 * initializer, especially the continuation lines.
 */
//indent input
enum multi_line {
	ml1 = 1
	    + 2
	    + offsetof(struct s, member)
	    + 3,
	ml2 = 1
	    + 2
	    + offsetof(struct s, member)
	    + 3,
};
//indent end

//indent run-equals-input -ci4

//indent run-equals-input -ci4 -nlp


/*
 * When 'typedef' or a tag is followed by a name, that name marks a type and a
 * following '*' marks a pointer type.
 */
//indent input
{
	// $ Syntactically invalid but shows that '*' is not multiplication.
	a = struct x * y;
	a = (struct x * y)z;
}
//indent end

//indent run
{
	// $ Everything before the '*' is treated as a declaration.
	a = struct x   *y;
	a = (struct x *y)z;
}
//indent end
