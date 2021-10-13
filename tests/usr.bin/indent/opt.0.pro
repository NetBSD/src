/* $NetBSD: opt.0.pro,v 1.2 2021/10/13 23:33:52 rillig Exp $ */
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
