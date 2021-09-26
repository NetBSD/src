/* $NetBSD: .indent.pro,v 1.1 2021/09/26 19:02:35 rillig Exp $ */

-di0		/* Do not indent variable names in global declarations. */
-nfc1		/* Do not format CVS Id comments. */
-i4		/* Indent by 4 spaces, for traditional reasons. */
-ldi0		/* Do not indent variable names in local declarations. */
-nlp		/* Do not indent function arguments. */
-ta		/* Identifiers ending in '_t' are considered type names. */
-TFILE		/* Additional types, for proper formatting of '*'. */
-Ttoken_type
