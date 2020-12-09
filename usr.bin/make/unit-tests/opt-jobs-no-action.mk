# $NetBSD: opt-jobs-no-action.mk,v 1.1 2020/12/09 00:25:00 rillig Exp $
#
# Tests for the combination of the options -j and -n, which prints the
# commands instead of actually running them.
#
# The format of the output differs from the output of only the -n option,
# without the -j.  This is because all this code is implemented twice, once
# in compat.c and once in job.c.

.MAKEFLAGS: -j1 -n

# Change the templates for running the commands in jobs mode, to make it
# easier to see what actually happens.
#
# The shell attributes are handled by Job_ParseShell.
# The shell attributes 'quiet' and 'echo' don't need a trailing newline,
# this is handled by the [0] != '\0' checks in Job_ParseShell.
# The '\#' is handled by ParseGetLine.
# The '\n' is handled by Str_Words in Job_ParseShell.
# The '$$' is handled by Var_Subst in ParseDependency.
.SHELL: \
	name=sh \
	path=${.SHELL} \
	quiet="\# .echoOff" \
	echo="\# .echoOn" \
	filter="\# .noPrint\n" \
	check="\# .errOnOrEcho\n""echo \"%s\"\n" \
	ignore="\# .errOffOrExecIgnore\n""%s\n" \
	errout="\# .errExit\n""{ %s \n} || exit $$?\n"

all:
	# The following command is regular, it is printed twice:
	# - first using the template shell.errOnOrEcho,
	# - then using the template shell.errExit.
	false regular

	# The following command is silent, it is printed once, using the
	# template shell.errExit.
	@: silent

	# The following command ignores errors, it is printed once, using
	# the default template for cmdTemplate, which is "%s\n".
	# XXX: Why is it not printed using shell.errOnOrEcho as well?
	# XXX: The '-' should not influence the echoing of the command.
	-false ignore-errors

	# The following command ignores the -n command line option, it is
	# not handled by the Job module but by the Compat module, see the
	# '!silent' in Compat_RunCommand.
	+echo run despite the -n option

	# TODO: test all 8 combinations of '-', '+', '@'.
	# TODO: for each of the above test, test '-true' and '-false'.
	# The code with its many branches feels like a big mess.
	# See opt-jobs.mk for the corresponding tests without the -n option.
