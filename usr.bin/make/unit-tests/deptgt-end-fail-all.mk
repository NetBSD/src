# $NetBSD: deptgt-end-fail-all.mk,v 1.1 2020/12/06 21:22:04 rillig Exp $
#
# Test whether the commands from the .END target are run even if there is
# an error before.  The manual page says "after everything else is done",
# which leaves room for interpretation.

all: .PHONY
	: Making ${.TARGET} out of nothing.
	false

.END:
	: Making ${.TARGET} out of nothing.
	false
