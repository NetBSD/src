/* $NetBSD: opt_version.c,v 1.2 2021/10/28 20:31:17 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '--version', which outputs the program version and
 * exits immediately.
 */

#indent input
When the option '--version' is given, any other options are ignored.
Therefore the source file, if given, can contain arbitrary text that
even might generate syntax errors when given to a C compiler.
#indent end

#indent run --version
NetBSD indent 2.1
#indent end
