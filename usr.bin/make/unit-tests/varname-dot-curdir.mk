# $NetBSD: varname-dot-curdir.mk,v 1.4 2020/10/04 21:53:28 rillig Exp $
#
# Tests for the special .CURDIR variable.

# TODO: Implementation

# Until 2020-10-04, assigning the result of a shell assignment to .CURDIR
# tried to add the shell command ("echo /") to the .PATH instead of the
# output of the shell command ("/").  Since "echo /" does not exist, the
# .PATH was left unmodified.  See VarAssign_Eval.
#
# Since 2020-10-04, the output of the shell command is added to .PATH.
.CURDIR!=	echo /
.if ${.PATH:M/} != "/"
.  error
.endif

# A normal assignment works fine, as does a substitution assignment.
# Appending to .CURDIR does not make sense, therefore it doesn't matter that
# this code path is buggy as well.
.CURDIR=	/
.if ${.PATH:M/} != "/"
.  error
.endif

all:
	@:;
