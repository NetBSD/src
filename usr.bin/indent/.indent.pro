/* $NetBSD: .indent.pro,v 1.4 2023/05/15 09:22:53 rillig Exp $ */

-l78		/* Keep 2 columns distance from the 80-column margin. */
-di0		/* Do not indent variable names in global declarations. */
/* XXX: -eei does not work; the expressions are indented only a single level. */
-eei		/* Indent expressions in 'if' and 'while' once more. */
-nfc1		/* Do not format CVS Id comments. */
-i4		/* Indent by 4 spaces, for traditional reasons. */
-ldi0		/* Do not indent variable names in local declarations. */
-nlp		/* Do not indent function arguments. */
-ta		/* Identifiers ending in '_t' are considered type names. */
-TFILE		/* Additional types, for proper formatting of '*'. */
-Tlexer_symbol
-Tparen_level_props
-Tparser_symbol
-Tkeyword_kind
