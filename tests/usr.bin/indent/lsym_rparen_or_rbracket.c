/* $NetBSD: lsym_rparen_or_rbracket.c,v 1.8 2023/06/16 14:26:27 rillig Exp $ */

/*
 * Tests for the token lsym_rparen_or_lbracket, which represents ')' or ']',
 * the counterparts for '(' and '['.
 *
 * See also:
 *	lsym_lparen_or_lbracket.c
 */

//indent input
int var = (3);
int cast = (int)3;
int cast = (int)(3);
int call = function(3);
int array[3] = {1, 2, 3};
int array[3] = {[2] = 3};
//indent end

//indent run-equals-input -di0


//indent input
int a = array[
3
];
{
int a = array[
3
];
}
//indent end

//indent run -di0
int a = array[
	      3
];
{
	int a = array[
		      3
// $ FIXME: Should be one level to the left since it is the outermost bracket.
		];
}
//indent end

//indent run -di0 -nlp
int a = array[
	3
];
{
	int a = array[
		3
// $ FIXME: Should be one level to the left since it is the outermost bracket.
		];
}
//indent end


/*
 * Cast expressions and compound expressions, taken from lint and make.
 */
//indent input
// This ')' is not a cast.
char *Buf_DoneData(Buffer *) MAKE_ATTR_USE;

{
	dcs_align((u_int)dcs);
	mpools.pools[i] = (memory_pool){NULL, 0, 0};
	list_add(l, (const char[3]){'-', (char)c, '\0'});
}
//indent end

//indent run-equals-input -ci4 -di0 -nlp
