/* $NetBSD: lsym_preprocessing.c,v 1.15 2023/06/16 23:19:01 rillig Exp $ */

/*
 * Tests for the token lsym_preprocessing, which represents a '#' that starts
 * a preprocessing line.
 *
 * #define
 * #ifdef
 * #include
 * #line
 * #pragma
 *
 * The whole preprocessing line is processed separately from the main source
 * code, without much tokenizing or parsing.
 */

// TODO: test '#' in the middle of a non-preprocessing line
// TODO: test stringify '#'
// TODO: test token paste '##'

//indent input
// TODO: add input
//indent end

//indent run-equals-input


/*
 * Whitespace in the following preprocessing directives is preserved.
 */
//indent input
#define space ' '		/* the 'define' is followed by a space */
#define	tab '\t'		/* the 'define' is followed by a tab */
#if   0				/* 3 spaces */
#elif		0		/* 2 tabs */
#elif	0	>	1	/* tabs between the tokens */
#endif
//indent end

//indent run-equals-input

// TODO: #define unfinished_string "...
// TODO: #define unfinished_char '...
// TODO: # 123 "file.h"
// TODO: backslash-newline
// TODO: block comment
// TODO: line comment


//indent input
#include <system-header.h>
#include "local-header.h"
//indent end

//indent run-equals-input


/*
 * Nested conditional compilation.
 */
//indent input
#if 0
#else
#endif

#if 0 /* if comment */
#else /* else comment */
#endif /* endif comment */

#if 0 /* outer if comment */
#  if nested /* inner if comment */
#  else /* inner else comment */
#  endif /* inner endif comment */
#endif /* outer endif comment */
//indent end

//indent run-equals-input


//indent input
#define multi_line_definition /* first line
 * middle
 * final line
 */ actual_value
//indent end

//indent run-equals-input


/*
 * Before indent.c 1.129 from 2021-10-08, indent mistakenly interpreted quotes
 * in comments as starting a string literal. The '"' in the comment started a
 * string, the next '"' finished the string, and the following '/' '*' was
 * interpreted as the beginning of a comment. This comment lasted until the
 * next '*' '/', which in this test is another preprocessor directive, solely
 * for symmetry.
 *
 * The effect was that the extra space after d2 was not formatted, as that
 * line was considered part of the comment.
 */
//indent input
#define comment_in_string_literal "/* no comment "
int this_is_an_ordinary_line_again;

int d1 ;
#define confuse_d /*"*/ "/*"
int d2 ;
#define resolve_d "*/"
int d3 ;

int s1 ;
#define confuse_s /*'*/ '/*'
int s2 ;
#define resolve_s '*/'
int s3 ;
//indent end

//indent run
#define comment_in_string_literal "/* no comment "
int		this_is_an_ordinary_line_again;

int		d1;
#define confuse_d /*"*/ "/*"
int		d2;
#define resolve_d "*/"
int		d3;

int		s1;
#define confuse_s /*'*/ '/*'
int		s2;
#define resolve_s '*/'
int		s3;
//indent end


/*
 * A preprocessing directive inside an expression keeps the state about
 * whether the next operator is unary or binary.
 */
//indent input
int binary_plus = 3
#define intermediate 1
	+4;
int unary_plus =
#define intermediate 1
	+ 4;
//indent end

//indent run
int		binary_plus = 3
#define intermediate 1
+ 4;
int		unary_plus =
#define intermediate 1
+4;
//indent end


/*
 * Before io.c 1.135 from 2021-11-26, indent fixed malformed preprocessing
 * lines that had arguments even though they shouldn't. It is not the task of
 * an indenter to fix code, that's what a linter is for.
 */
//indent input
#if 0
#elif 1
#else if 3
#endif 0
//indent end

//indent run-equals-input


/*
 * Existing comments are indented just like code comments.
 *
 * This means that the above wrong preprocessing lines (#else with argument)
 * need to be fed through indent twice until they become stable. Since
 * compilers issue warnings about these invalid lines, not much code still has
 * these, making this automatic fix an edge case.
 */
//indent input
#if 0		/* comment */
#else		/* comment */
#endif		/* comment */

#if 0/* comment */
#else/* comment */
#endif/* comment */
//indent end

//indent run-equals-input


/*
 * Multi-line comments in preprocessing lines.
 */
//indent input
#define eol_comment		// EOL

#define no_wrap_comment		/* line 1
				 * line 2
				 * line 3
				 */

#define fixed_comment		/*- line 1
				 * line 2
				 * line 3
				 */

#define two_comments /* 1 */ /* 2 */ /*3*/
#define three_comments		/* first */ /* second */ /*third*/
//indent end

//indent run-equals-input


/*
 * Do not touch multi-line macro definitions.
 */
//indent input
#define do_once(stmt)		\
do {				\
	stmt;			\
} while (/* constant condition */ false)
//indent end

//indent run-equals-input


/*
 * The 'INDENT OFF' state is global, it does not depend on the preprocessing
 * directives, otherwise the declarations for 'on' and 'after' would be moved
 * to column 1.
 */
//indent input
int first_line;
	int before;
#if 0
/*INDENT OFF*/
	int off;
#else
	int on;
#endif
	int after;
//indent end

//indent run -di0
int first_line;
int before;
#if 0
/*INDENT OFF*/
	int off;
#else
	int on;
#endif
	int after;
//indent end


/*
 * Before 2023-06-14, indent was limited to 5 levels of conditional compilation
 * directives.
 */
//indent input
#if 1
#if 2
#if 3
#if 4
#if 5
#if 6
#endif 6
#endif 5
#endif 4
#endif 3
#endif 2
#endif 1
//indent end

//indent run-equals-input


/*
 * Unrecognized and unmatched preprocessing directives are preserved.
 */
//indent input
#else
#elif 0
#elifdef var
#endif

#unknown
# 3 "file.c"
//indent end

//indent run
#else
#elif 0
#elifdef var
#endif

#unknown
# 3 "file.c"
// exit 1
// error: Standard Input:1: Unmatched #else
// error: Standard Input:2: Unmatched #elif
// error: Standard Input:3: Unmatched #elifdef
// error: Standard Input:4: Unmatched #endif
//indent end


/*
 * The '#' can only occur at the beginning of a line, therefore indent does not
 * care when it occurs in the middle of a line.
 */
//indent input
int no = #;
//indent end

//indent run -di0
int no =
#;
//indent end


/*
 * Preprocessing directives may be indented; indent moves them to the beginning
 * of a line.
 */
//indent input
#if 0
	#if 1 \
	 || 2
	#endif
#endif
//indent end

//indent run
#if 0
#if 1 \
	 || 2
#endif
#endif
//indent end
