/* $NetBSD: .indent.pro,v 1.6 2023/05/16 12:46:43 rillig Exp $ */

-l78		/* Keep 2 columns distance from the 80-column margin. */
-di0		/* Do not indent variable names in global declarations. */
-eei		/* Indent expressions in 'if' and 'while' once more. */
-i4		/* Indent by 4 spaces, for traditional reasons. */
-ldi0		/* Do not indent variable names in local declarations. */
-nlp		/* Do not indent function arguments. */
-ta		/* For proper formatting of type casts. */
