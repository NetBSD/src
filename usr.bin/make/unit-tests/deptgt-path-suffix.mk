# $NetBSD: deptgt-path-suffix.mk,v 1.3.4.1 2025/08/02 05:58:33 perseant Exp $
#
# Tests for the special target .PATH.suffix in dependency declarations.

# TODO: Implementation

# expect+1: Suffix ".c" not defined (yet)
.PATH.c: ..

.SUFFIXES: .c

# Now the suffix is defined, and the path is recorded.
.PATH.c: ..

all:
	@:;
