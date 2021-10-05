/* $NetBSD: .indent.pro,v 1.2 2021/10/05 22:09:05 rillig Exp $ */

-di0		/* Do not indent variable names in global declarations. */
/* XXX: -eei does not work; the expressions are indented only a single level. */
-eei		/* Indent expressions in 'if' and 'while' once more. */
-nfc1		/* Do not format CVS Id comments. */
-i4		/* Indent by 4 spaces, for traditional reasons. */
-ldi0		/* Do not indent variable names in local declarations. */
-nlp		/* Do not indent function arguments. */
-ta		/* Identifiers ending in '_t' are considered type names. */
-TFILE		/* Additional types, for proper formatting of '*'. */
-Ttoken_type
