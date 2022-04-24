/* $NetBSD: psym_decl.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the parser symbol psym_decl, which represents a declaration.
 *
 * Since C99, declarations and statements can be mixed in blocks.
 *
 * A label can be followed by a statement but not by a declaration.
 *
 * Indent distinguishes global and local declarations.
 *
 * Declarations can be for functions or for variables.
 */

// TODO: prove that psym_decl can only ever occur at the top of the stack.
// TODO: delete decl_level if the above is proven.

//indent input
int global_var;
int global_array = [1,2,3,4];
int global_array = [
1
,2,
3,
4,
];
//indent end

//indent run -di0
int global_var;
int global_array = [1, 2, 3, 4];
int global_array = [
		    1
		    ,2,
		    3,
		    4,
];
//indent end
