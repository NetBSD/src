# $NetBSD: suff-incomplete.mk,v 1.1 2020/11/21 10:32:42 rillig Exp $
#
# Tests incomplete transformation rules, which are ignored.

all: suff-incomplete.c

.MAKEFLAGS: -dps

.SUFFIXES:

.SUFFIXES: .a .b .c

# This rule has no commands and no dependencies, therefore it is incomplete
# and not added to the transformation rules.
#
# See Suff_EndTransform.
.a.b:

# This rule has a dependency, therefore it is a complete transformation.
# Its commands are taken from a .DEFAULT target, if there is any.
.a.c: ${.PREFIX}.dependency

.DEFAULT:
	: Making ${.TARGET} from ${.IMPSRC} all ${.ALLSRC} by default.

# XXX: The debug log says "transformation .DEFAULT complete", which is wrong.
# .DEFAULT is not a transformation.

# XXX: The output of this test says "Making suff-incomplete.c from
# suff-incomplete.c".  It doesn't make sense to make something out of itself.
