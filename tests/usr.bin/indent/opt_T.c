/* $NetBSD: opt_T.c,v 1.2 2021/11/19 22:24:29 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-T', which specifies a single identifier that indent
 * will recognize as a type name.  This affects the formatting of
 * syntactically ambiguous expressions that could be casts or multiplications,
 * among others.
 */

#indent input
int cast = (custom_type_name)   *   arg;

int mult = (unknown_type_name)   *   arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t)   *   arg;
#indent end

#indent run -Tcustom_type_name -di0
int cast = (custom_type_name)*arg;

int mult = (unknown_type_name) * arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t) * arg;
#indent end

#indent run -Tcustom_type_name -di0 -cs
int cast = (custom_type_name) *arg;

int mult = (unknown_type_name) * arg;

/* See the option -ta for handling these types. */
int suff = (unknown_type_name_t) * arg;
#indent end
