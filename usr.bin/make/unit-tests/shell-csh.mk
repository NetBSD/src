# $NetBSD: shell-csh.mk,v 1.1 2020/10/03 14:39:36 rillig Exp $
#
# Tests for using a C shell for running the commands.

.SHELL: name="csh" path="csh"

# Contrary to sh and ksh, the csh does not know the ':' command.
# Therefore this test uses 'true' instead.
all:
	true normal
	@true hidden
	+true always
	-true ignore errors
