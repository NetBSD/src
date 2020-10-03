# $NetBSD: shell-csh.mk,v 1.4 2020/10/03 15:38:13 rillig Exp $
#
# Tests for using a C shell for running the commands.

# The shell path must be an absolute path.
# This is only obvious in parallel mode since in compat mode,
# simple commands are executed via execve directly.
.SHELL: name="csh" path="${:!which csh!}"

# In parallel mode, the commandShell->noPrint command is filtered from
# the output, rather naively (in JobOutput).
#
# Until 2020-10-03, the output in parallel mode was garbled because
# the definition of the csh had been wrong since 1993 at least.
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

	# In the C shell, "unset verbose" is set as the noPrint command.
	# Therefore it is filtered from the output, rather naively.
	@echo 'They chatted in the sunset verbosely.'
