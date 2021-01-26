# $NetBSD: directive-error.mk,v 1.4 2021/01/26 23:51:20 rillig Exp $
#
# Tests for the .error directive, which prints an error message and exits
# immediately, unlike other "fatal" parse errors, which continue to parse
# until the end of the current top-level makefile.

# TODO: Implementation

# FIXME: The "parsing warnings being treated as errors" is irrelevant.
.MAKEFLAGS: -W
.error message
