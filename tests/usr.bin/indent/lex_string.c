/* $NetBSD: lex_string.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Test lexing of string literals.
 */

#indent input
char simple[] = "x";
char multi[] = "xy";
char empty[] = "";
char null[] = "\0";
char escape_hex[] = "\x3f";
char escape_octal[] = "\040";
char escape_a[] = "\a";
char escape_b[] = "\b";
char escape_f[] = "\f";
char escape_n[] = "\n";
char escape_t[] = "\t";
char escape_v[] = "\v";
char escape_single_quote[] = "\'";
char escape_double_quote[] = "\"";
char escape_backslash[] = "\\";

char escape_newline[] = "\
";
#indent end

#indent run-equals-input -di0


/*
 * Concatenated string literals are separated with a single space.
 */
#indent input
char concat[] = "line 1\n"
"line2"		"has"   "several""words\n";
#indent end

#indent run -di0
char concat[] = "line 1\n"
"line2" "has" "several" "words\n";
#indent end
