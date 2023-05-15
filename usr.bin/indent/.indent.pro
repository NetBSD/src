/* $NetBSD: .indent.pro,v 1.5 2023/05/15 20:35:56 rillig Exp $ */

-l78		/* Keep 2 columns distance from the 80-column margin. */
-di0		/* Do not indent variable names in global declarations. */
-eei		/* Indent expressions in 'if' and 'while' once more. */
-nfc1		/* Do not format CVS Id comments. */
-i4		/* Indent by 4 spaces, for traditional reasons. */
-ldi0		/* Do not indent variable names in local declarations. */
-nlp		/* Do not indent function arguments. */
-ta		/* For proper formatting of type casts. */
