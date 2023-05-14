/* $NetBSD: opt_fc1.c,v 1.11 2023/05/14 17:53:38 rillig Exp $ */

/*
 * Tests for the options '-fc1' and '-nfc1'.
 *
 * The option '-fc1' formats comments in column 1.
 *
 * The option '-nfc1' preserves the original formatting of comments that start
 * in column 1.
 */

//indent input
/*
 * A comment
 * in column 1.
 *
 *
 *
 */
//indent end

//indent run -fc1
/*
 * A comment in column 1.
 *
 *
 *
 */
//indent end

//indent run-equals-input -nfc1


//indent input
/* $ Neither indentation nor surrounding spaces. */
/*narrow*/

/* $ Indented by a single space, single spaces around the text. */
 /* space */

/* $ Indented by a single tab, single tabs around the text. */
	/*	indented tab	*/

/* $ The second comment is moved to a separate line. */
/* col1 space-padded */ /* space-padded */

/* $ Both comment texts get surrounded by spaces. */
/*col1 no-padding*//*no-padding*/
//indent end

//indent run -fc1
/* $ The comment text got surrounded by spaces. */
/* narrow */

/* $ The indentation got removed. */
/* space */

/* $ The indentation got removed. */
/*	indented tab	*/

/* $ Each comment got its separate line. */
/* col1 space-padded */
/* space-padded */

/* $ Both comment texts got surrounded by spaces. */
/* col1 no-padding */
/* no-padding */
//indent end

//indent run -nfc1
/* $ No spaces got added around the comment text. */
/*narrow*/

/* $ The indentation of a single space was preserved. */
/* $ If the comment were moved to column 1, it would change from the area */
/* $ of 'comments that may be formatted' to the area of 'comments that must */
/* $ not be formatted. The indentation of a single space prevents this. */
 /* space */

/* $ The indentation was changed from a single tab to a single space. */
 /*	indented tab	*/

/* $ The second comment moved to a separate line. */
/* col1 space-padded */
 /* space-padded */

/* $ The 'col1' comment starts in column 1 and is thus not modified. */
/* $ The second comment started further to the right and thus was modified */
/* $ by moving it to a separate line, but not to column 1. */
/*col1 no-padding*/
 /* no-padding */
//indent end


/*
 * Since 2019-04-04 and before pr_comment.c 1.123 from 2021-11-25, the
 * function analyze_comment wrongly joined the two comments.
 */
//indent input
/*
 * A multi-line comment that starts
 * in column 1.
 *//* followed by another multi-line comment
 * that starts in column 4.
 */
//indent end

//indent run -fc1
/*
 * A multi-line comment that starts in column 1.
 */
/*
 * followed by another multi-line comment that starts in column 4.
 */
//indent end

//indent run -nfc1
/*
 * A multi-line comment that starts
 * in column 1.
 */
 /*
  * followed by another multi-line comment that starts in column 4.
  */
//indent end


//indent input
/* comment */ int decl2; /* comment */
/* looooooooooooooooooooooooooooooooooooooooong first comment */ int decl2; /* second comment */
/* first comment */ int decl2; /* looooooooooooooooooooooooooooooooooooooooong second comment */
//indent end

//indent run -fc1
/* comment */ int decl2;	/* comment */
/* looooooooooooooooooooooooooooooooooooooooong first comment */ int decl2;	/* second comment */
/* first comment */ int decl2;	/* looooooooooooooooooooooooooooooooooooooooong
				 * second comment */
//indent end

//indent run-equals-prev-output -nfc1
