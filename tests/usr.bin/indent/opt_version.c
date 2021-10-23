/* $NetBSD: opt_version.c,v 1.1 2021/10/23 20:23:27 rillig Exp $ */
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
FreeBSD indent 2.0
#indent end
