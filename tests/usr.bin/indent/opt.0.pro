/* $NetBSD: opt.0.pro,v 1.1 2021/10/07 18:07:25 rillig Exp $ */
/* $FreeBSD$ */

/* The latter of the two options wins. */
-di5
-di12

/* It is possible to embed comments in the options, but nobody does that. */
-/* comment */bacc
-T/* define a type */custom_type

/*
 * For int or float options, trailing garbage is ignored.
 *
 * See atoi, atof.
 */
-i3garbage
-cli3.5garbage

-b/*/acc	/* The comment is '/' '*' '/', making the option '-bacc'. */

/*
 * For boolean options, trailing garbage is ignored.
 *
 * See strncmp
 */
-bacchus	/* Is interpreted as '-bacc'. */
