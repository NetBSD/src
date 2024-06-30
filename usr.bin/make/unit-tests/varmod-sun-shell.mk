# $NetBSD: varmod-sun-shell.mk,v 1.3 2024/06/30 11:00:06 rillig Exp $
#
# Tests for the :sh variable modifier, which runs the shell command
# given by the variable value and returns its output.
#
# This modifier has been added on 1996-05-29.
#
# See also:
#	ApplyModifier_SunShell

.if ${echo word:L:sh} != "word"
.  error
.endif

# If the command exits with non-zero, a warning is printed.
# expect+1: warning: while evaluating variable "echo word; false": "echo word; false" returned non-zero status
.if ${echo word; false:L:sh} != "word"
.  error
.endif


.MAKEFLAGS: -dv			# to see the "Capturing" debug output
# expect+1: warning: while evaluating variable "echo word; false": "echo word; false" returned non-zero status
_:=	${echo word; ${:Ufalse}:L:sh}
.MAKEFLAGS: -d0


all:
