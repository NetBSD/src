/* $NetBSD: token_keyword_struct_union_enum.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
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
