# $NetBSD: varmod-sysv.mk,v 1.2 2020/08/16 14:25:16 rillig Exp $
#
# Tests for the ${VAR:from=to} variable modifier, which replaces the suffix
# "from" with "to".  It can also use '%' as a wildcard.
#
# This modifier is applied when the other modifiers don't match exactly.

# TODO: Implementation

all:
	@:;
