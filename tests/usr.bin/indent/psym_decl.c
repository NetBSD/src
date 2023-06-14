/* $NetBSD: psym_decl.c,v 1.5 2023/06/14 09:31:05 rillig Exp $ */

/*
 * Tests for the parser symbol psym_decl, which represents a declaration.
 *
 * Since C99, declarations and statements can be mixed in blocks.
 *
 * In C, a label can be followed by a statement but not by a declaration.
 *
 * Indent distinguishes global and local declarations.
 *
 * Declarations can be for functions or for variables.
 */

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


// Declarations can be nested.
//indent input
struct level_1 {
	union level_2 {
		enum level_3 {
			level_3_c_1,
			level_3_c_2,
		}		level_3;
	}		level_2;
} level_1;
//indent end

// The outermost declarator 'level_1' is indented as a global variable.
// The inner declarators 'level_2' and 'level_3' are indented as local
// variables.
// XXX: This is inconsistent, as in practice, struct members are usually
// aligned, while local variables aren't.
//indent run-equals-input -ldi0
