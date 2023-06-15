/* $NetBSD: lsym_rparen_or_rbracket.c,v 1.5 2023/06/15 09:19:07 rillig Exp $ */

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
