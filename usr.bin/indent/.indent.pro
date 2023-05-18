/* $NetBSD: .indent.pro,v 1.7 2023/05/18 04:23:03 rillig Exp $ */

-bap		/* Force a blank line after function body. */
-br		/* Place '{' to the right side. */
-ce		/* Place '} else' on the same line. */
-ci4		/* Indent statement continuations with 4 spaces. */
-cli0		/* Don't indent 'case' relative to the 'switch'. */
-d0		/* Indent comments in the same column as the code. */
-di0		/* Do not indent variable declarations. */
-i8		/* Use a single tab (8 columns) per indentation level. */
-ip		/* Indent parameter declarations. */
-l79		/* Leave a single empty column on 80-column displays. */
-nbc		/* Don't force each declarator on a separate line. */
-ncdb		/* Allow single-line block comments. */
-ndj		/* Indent declarations in the same column as the code. */
-ei		/* Place 'else if' on the same line. */
-nfc1		/* Don't format comments in line 1, to preserve CVS IDs. */
-nlp		/* Indent statement continuations by a fixed amount. */
-npcs		/* Don't place a space between function name and '('. */
-psl		/* Place function names in column 1. */
-sc		/* Prefix multi-line block comments with '*'. */
-sob		/* Swallow optional blank lines. */

-ta		/* For proper formatting of type casts. */
