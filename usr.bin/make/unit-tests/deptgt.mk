# $NetBSD: deptgt.mk,v 1.5 2020/09/14 18:27:15 rillig Exp $
#
# Tests for special targets like .BEGIN or .SUFFIXES in dependency
# declarations.

# TODO: Implementation

# Just in case anyone tries to compile several special targets in a single
# dependency line: That doesn't work, and make immediately rejects it.
.SUFFIXES .PHONY: .c.o

# The following lines demonstrate how 'targets' is set and reset during
# parsing of dependencies.  To see it in action, set breakpoints in:
#
#	ParseDoDependency	at the beginning
#	ParseFinishLine		at "targets = NULL"
#	Parse_File		at "Lst_Free(targets)"
#	Parse_File		at "targets = Lst_Init()"
#	Parse_File		at "!inLine"
#
# Keywords:
#	parse.c:targets

target1 target2: sources	# targets := [target1, target2]
	: command1		# targets == [target1, target2]
	: command2		# targets == [target1, target2]
VAR=value			# targets := NULL
	: command3		# parse error, since targets == NULL

all:
	@:;
