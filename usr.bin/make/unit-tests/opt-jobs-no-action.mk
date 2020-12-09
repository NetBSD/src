# $NetBSD: opt-jobs-no-action.mk,v 1.4 2020/12/09 07:57:52 rillig Exp $
#
# Tests for the combination of the options -j and -n, which prints the
# commands instead of actually running them.
#
# The format of the output differs from the output of only the -n option,
# without the -j.  This is because all this code is implemented twice, once
# in compat.c and once in job.c.
#
# See also:
#	opt-jobs.mk
#		The corresponding tests without the -n option
#	opt-no-action-combined.mk
#		The corresponding tests without the -j option

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

all: documented combined
.ORDER: documented combined

# Explain the most basic cases in detail.
documented: .PHONY
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

	@+echo


# Test all combinations of the 3 RunFlags.
#
# TODO: Closely inspect the output whether it makes sense.
# XXX: silent=no always=no ignerr={no,yes} should be almost the same.
#
SILENT.no=	# none
SILENT.yes=	@
ALWAYS.no=	# none
ALWAYS.yes=	+
IGNERR.no=	echo running
IGNERR.yes=	-echo running; false
#
combined:
	@+echo 'begin $@'
	@+echo
.for silent in no yes
.  for always in no yes
.    for ignerr in no yes
	@+echo silent=${silent} always=${always} ignerr=${ignerr}
	${SILENT.${silent}}${ALWAYS.${always}}${IGNERR.${ignerr}}
	@+echo
.    endfor
.  endfor
.endfor
	@+echo 'end $@'
