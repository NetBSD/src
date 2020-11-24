# $NetBSD: deptgt-end-fail.mk,v 1.2 2020/11/24 17:59:42 rillig Exp $
#
# Tests for an error in the .END node.
#
# Before 2020-11-25, an error in the .END target did not print the "Stop.",
# even though this was intended.  The cause for this was a missing condition
# in Compat_Run, in the code handling the .END node.

all:
	: $@

.END:
	false
