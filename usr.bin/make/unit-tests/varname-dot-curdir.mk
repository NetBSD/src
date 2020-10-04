# $NetBSD: varname-dot-curdir.mk,v 1.3 2020/10/04 20:06:48 rillig Exp $
#
# Tests for the special .CURDIR variable.

# TODO: Implementation

# As of 2020-10-04, assigning the result of a shell command to .CURDIR tries
# to add the shell command to the .PATH instead of the output of the shell
# command.  Since "echo /" does not exist, the .PATH is left unmodified.
# See Parse_DoVar at the very bottom.
.CURDIR!=	echo /
.if ${.PATH:M/}
.  error
.endif

# A normal assignment works fine, as does a substitution assignment.
# Appending to .CURDIR does not make sense, therefore it doesn't matter that
# this code path is buggy as well.
.CURDIR=	/
.if !${.PATH:M/}
.  error
.endif

all:
	@:;
