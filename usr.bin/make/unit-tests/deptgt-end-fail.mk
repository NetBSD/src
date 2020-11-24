# $NetBSD: deptgt-end-fail.mk,v 1.1 2020/11/24 15:36:51 rillig Exp $
#
# Tests for an error in the .END node.
#
# Before 2020-11-25, an error in the .END target did not print the "Stop.",
# even though this was intended.  The cause for this was a simple typo in
# a variable name.

all:
	: $@

.END:
	false
