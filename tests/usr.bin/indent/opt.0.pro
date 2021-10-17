/* $NetBSD: opt.0.pro,v 1.3 2021/10/17 18:13:00 rillig Exp $ */
/* $FreeBSD$ */

/* The latter of the two options wins. */
-di5
-di12

/* It is possible to embed comments in the options, but nobody does that. */
-/* comment */bacc
-T/* define a type */custom_type

/* For int options, trailing garbage would lead to an error message. */
-i3

/*
 * For float options, trailing garbage is ignored.
 *
 * See atof.
 */
-cli3.5garbage

-b/*/acc	/* The comment is '/' '*' '/', making the option '-bacc'. */
