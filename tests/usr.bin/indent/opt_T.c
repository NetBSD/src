/* $NetBSD: opt_T.c,v 1.5 2023/06/17 22:09:24 rillig Exp $ */

/*
 * Tests for the option '-T', which specifies a single identifier that indent
 * will recognize as a type name.  This affects the formatting of
 * syntactically ambiguous expressions that could be casts or multiplications,
 * among others.
 */

//indent input
int cast = (custom_type_name)   *   arg;

int mult = (unknown_type_name)   *   arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t)   *   arg;
//indent end

//indent run -Tcustom_type_name -di0
int cast = (custom_type_name)*arg;

int mult = (unknown_type_name) * arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t) * arg;
//indent end

//indent run -Tcustom_type_name -di0 -cs
int cast = (custom_type_name) *arg;

int mult = (unknown_type_name) * arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t) * arg;
//indent end


/*
 * The keyword table has precedence over the custom-specified types; otherwise,
 * the following lines would be declarations, and the declarators would be
 * indented by 16.
 */
//indent input
{
	break x;
	continue x;
	goto x;
	return x;
}
//indent end

//indent run-equals-input -Tbreak -Tcontinue -Tgoto -Treturn
