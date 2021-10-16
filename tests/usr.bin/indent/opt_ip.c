/* $NetBSD: opt_ip.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/* FIXME: The options -ip and -nip produce the same output. */

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
/* FIXME: The options -ip and -nip produce the same output. */

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
/* FIXME: The options -ip and -nip produce the same output. */

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
