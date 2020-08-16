# $NetBSD: varmod-undefined.mk,v 1.2 2020/08/16 14:25:16 rillig Exp $
#
# Tests for the :U variable modifier, which returns the given string
# if the variable is undefined.
#
# This modifier is heavily used when expanding .for loops.

# TODO: Implementation

all:
	@:;
