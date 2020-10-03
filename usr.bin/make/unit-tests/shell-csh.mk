# $NetBSD: shell-csh.mk,v 1.3 2020/10/03 15:23:42 rillig Exp $
#
# Tests for using a C shell for running the commands.

.SHELL: name="csh" path="${:!which csh!}"

# The -j option is needed to cover the code in JobOutput.
#
# FIXME: The "se " does not belong in the output.
.MAKEFLAGS: -j1

all:
	# This command is both printed and executed.
	echo normal

	# This command is only executed.
	@echo hidden

	# This command is both printed and executed.
	+echo always

	# This command is both printed and executed.
	-echo ignore errors
