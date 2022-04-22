/* $NetBSD: token_preprocessing.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*-
 * Tests for indenting preprocessing directives:
 *
 * #define
 * #ifdef
 * #pragma
 * #line
 */


#indent input
#include <system-header.h>
#include "local-header.h"
#indent end

#indent run-equals-input


/*
 * Nested conditional compilation.
 */
#indent input
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
#indent end

#indent run
#if 0
#else
#endif

#if 0				/* if comment */
#else				/* else comment */
#endif				/* endif comment */

#if 0				/* outer if comment */
/* $ XXX: The indentation is removed, which can get confusing */
#if nested			/* inner if comment */
#else				/* inner else comment */
#endif				/* inner endif comment */
#endif				/* outer endif comment */
#indent end


#indent input
#define multi_line_definition /* first line
 * middle
 * final line
 */ actual_value
#indent end

#indent run-equals-input


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
#indent input
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
#indent end

#indent run
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
#indent end


/*
 * A preprocessing directive inside an expression keeps the state about
 * whether the next operator is unary or binary.
 */
#indent input
int binary_plus = 3
#define intermediate 1
	+4;
int unary_plus =
#define intermediate 1
	+ 4;
#indent end

#indent run
int		binary_plus = 3
#define intermediate 1
+ 4;
int		unary_plus =
#define intermediate 1
+4;
#indent end


/*
 * Before io.c 1.135 from 2021-11-26, indent fixed malformed preprocessing
 * lines that had arguments even though they shouldn't. It is not the task of
 * an indenter to fix code, that's what a linter is for.
 */
#indent input
#if 0
#elif 1
#else if 3
#endif 0
#indent end

#indent run-equals-input


/*
 * Existing comments are indented just like code comments.
 *
 * This means that the above wrong preprocessing lines (#else with argument)
 * need to be fed through indent twice until they become stable. Since
 * compilers issue warnings about these invalid lines, not much code still has
 * these, making this automatic fix an edge case.
 */
#indent input
#if 0		/* comment */
#else		/* comment */
#endif		/* comment */

#if 0/* comment */
#else/* comment */
#endif/* comment */
#indent end

#indent run
#if 0				/* comment */
#else				/* comment */
#endif				/* comment */

#if 0				/* comment */
#else				/* comment */
#endif				/* comment */
#indent end
