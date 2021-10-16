/* $NetBSD: opt_ip.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-ip' and '-nip'.
 *
 * The option '-ip' indents parameter declarations from the left margin.
 *
 * The option '-nip' TODO.
 */

/* FIXME: The options -ip and -nip produce the same output. */

#indent input
int
several_parameters_1(int a,
int b,
const char *cp);

int
several_parameters_2(
int a,
int b,
const char *cp);

int
several_parameters_3(
int a1, int a2,
int b1, int b2,
const char *cp);
#indent end

#indent run -ip
int
several_parameters_1(int a,
		     int b,
		     const char *cp);

int
several_parameters_2(
		     int a,
		     int b,
		     const char *cp);

int
several_parameters_3(
		     int a1, int a2,
		     int b1, int b2,
		     const char *cp);
#indent end

#indent run -nip
int
several_parameters_1(int a,
		     int b,
		     const char *cp);

int
several_parameters_2(
		     int a,
		     int b,
		     const char *cp);

int
several_parameters_3(
		     int a1, int a2,
		     int b1, int b2,
		     const char *cp);
#indent end
