# $NetBSD: varmod-assign-shell.mk,v 1.2 2021/12/28 10:47:00 rillig Exp $
#
# Tests for the variable modifier '::!=', which assigns the output of a shell
# command to the variable, but only if the command exited successfully.  This
# is different from the variable assignment operator '!=', which also assigns
# when the shell command fails or crashes.
#
# The variable modifier '::!=' and its close relatives have been around since
# var.c 1.45 from 2000-06-01.
#
# Before 2020.08.25.21.16.53, the variable modifier '::!=' had a bug for
# unsuccessful commands, it put the previous value of the variable into the
# error message instead of the command that was executed.  That's where the
# counterintuitive error message 'make: "previous" returned non-zero status'
# comes from.
#
# BUGS
#	Even though the variable modifier '::!=' produces an error message,
#	the exit status of make is still 0.
#
#	Having an error message instead of a warning like for the variable
#	assignment operator '!=' is another unnecessary inconsistency.

DIRECT=		previous
DIRECT!=	echo output; false

ASSIGNED=	previous
_:=		${ASSIGNED::!=echo output; false}

all:
	@echo DIRECT=${DIRECT:Q}
	@echo ASSIGNED=${ASSIGNED:Q}
