# $NetBSD: deptgt.mk,v 1.4 2020/09/14 18:21:26 rillig Exp $
#
# Tests for special targets like .BEGIN or .SUFFIXES in dependency
# declarations.

# TODO: Implementation

# Just in case anyone tries to compile several special targets in a single
# dependency line: That doesn't work, and make immediately rejects it.
.SUFFIXES .PHONY: .c.o

# Keyword "parse.c:targets"
#
# The following lines demonstrate how 'target' is set and reset during
# parsing of dependencies.  To see it in action, set breakpoints in:
#
#	ParseDoDependency	at the beginning
#	ParseFinishLine		at "targets = NULL"
#	Parse_File		at "Lst_Free(targets)"
#	Parse_File		at "targets = Lst_Init()"
#	Parse_File		at "!inLine"
#
target1 target2: sources
	: command1
	: command2
VAR=value
	: command3

all:
	@:;
