# $NetBSD: shell-csh.mk,v 1.2 2020/10/03 15:21:12 rillig Exp $
#
# Tests for using a C shell for running the commands.

.SHELL: name="csh" path="csh"

all:
	# This command is both printed and executed.
	echo normal

	# This command is only executed.
	@echo hidden

	# This command is both printed and executed.
	+echo always

	# This command is both printed and executed.
	-echo ignore errors
