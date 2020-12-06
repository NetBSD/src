# $NetBSD: deptgt-end-fail.mk,v 1.3 2020/12/06 21:22:04 rillig Exp $
#
# Tests for an error in the .END node.
#
# Before 2020-11-25, an error in the .END target did not print the "Stop.",
# even though this was intended.  The cause for this was a missing condition
# in Compat_Run, in the code handling the .END node.

all:
	: $@

.END:
	: Making ${.TARGET} out of nothing.
	false
