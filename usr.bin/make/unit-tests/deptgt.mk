# $NetBSD: deptgt.mk,v 1.3 2020/09/14 17:43:36 rillig Exp $
#
# Tests for special targets like .BEGIN or .SUFFIXES in dependency
# declarations.

# TODO: Implementation

# Just in case anyone tries to compile several special targets in a single
# dependency line: That doesn't work, and make immediately rejects it.
.SUFFIXES .PHONY: .c.o

all:
	@:;
