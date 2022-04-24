/* $NetBSD: opt_version.c,v 1.4 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the option '--version', which outputs the program version and
 * exits immediately.
 */

//indent input
When the option '--version' is given, any other options are ignored.
Therefore the source file, if given, can contain arbitrary text that
even might generate syntax errors when given to a C compiler.
//indent end

//indent run --version
NetBSD indent 2.1
//indent end
