# $NetBSD: varname-dot-makeflags.mk,v 1.3 2023/02/25 09:02:45 rillig Exp $
#
# Tests for the special .MAKEFLAGS variable, which collects almost all
# command line arguments and passes them on to any child processes via
# the environment variable MAKEFLAGS (without leading '.').
#
# See also:
#	varname-dot-makeoverrides.mk

all: spaces_stage_0 dollars_stage_0 append_stage_0


# When options are parsed, the option and its argument are appended as
# separate words to .MAKEFLAGS.  Special characters in the option argument
# are not quoted though.  It seems to have not been necessary since at least
# 1993.
spaces_stage_0:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@echo "$@: env MAKEFLAGS=<$$MAKEFLAGS>"
	@${MAKE} -f ${MAKEFILE} spaces_stage_1 -d00000 -D"VARNAME WITH SPACES"

# At this point, the 'VARNAME WITH SPACES' is no longer recognizable as a
# single command line argument.  In practice, variable names don't contain
# spaces.
spaces_stage_1:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@echo "$@: env MAKEFLAGS=<$$MAKEFLAGS>"


# Demonstrate that '$' characters are altered when they are passed on to child
# make processes via MAKEFLAGS.
dollars_stage_0:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	# The '$$$$' gets parsed as a literal '$$', making the actual variable
	# value '$${varname}'.
	#
	# When Main_ExportMAKEFLAGS adds the variable DOLLARS to MAKEFLAGS, it
	# first evaluates the variable value, resulting in '${varname}'.
	#
	# This value is then escaped as '\$\{varname\}', to ensure that the
	# variable is later interpreted as a single shell word.
	@${MAKE} -f ${MAKEFILE} dollars_stage_1 DOLLARS='$$$${varname}'

# The environment variable 'MAKEFLAGS' now contains the variable assignment
# 'DOLLARS=\$\{varname\}', including the backslashes.
#
# expect: dollars_stage_1: env MAKEFLAGS=< -r -k DOLLARS=\$\{varname\}>
#
# When Main_ParseArgLine calls Str_Words to parse the flags from MAKEFLAGS, it
# removes the backslashes, resulting in the plain variable assignment
# 'DOLLARS=${varname}'.
dollars_stage_1:
	@echo "$@: env MAKEFLAGS=<$$MAKEFLAGS>"

	# At this point, evaluating the environment variable 'MAKEFLAGS' has
	# strange side effects, as the string '\$\{varname\}' is interpreted
	# as:
	#
	#	\		a single backslash
	#	$\		the value of the variable named '\'
	#	{varname\}	a literal string
	#
	# Since the variable name '\' is not defined, the resulting value is
	# '\{varname\}'.
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'

	# The modifier ':q' escapes a '$' in the variable value to '$$', but
	# it's too late, as that modifier applies after the expression has
	# been evaluated.
	@echo '$@: MAKEFLAGS:q=<'${MAKEFLAGS:q}'>'

	# The value of the variable DOLLARS is now '${varname}'.  Since there
	# is no variable named 'varname', this expression evaluates to ''.
	@${MAKE} -f ${MAKEFILE} dollars_stage_2

# Uncomment these lines to see the above variable expansions in action.
#${:U\\}=backslash
#varname=varvalue

dollars_stage_2:
	@echo "$@: env MAKEFLAGS=<$$MAKEFLAGS>"
	@echo '$@: dollars=<'${DOLLARS:Q}'>'
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@${MAKE} -f ${MAKEFILE} dollars_stage_3

dollars_stage_3:
	@echo "$@: env MAKEFLAGS=<$$MAKEFLAGS>"
	@echo '$@: dollars=<'${DOLLARS:Uundefined:Q}'>'
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'


# Demonstrates in which exact order the MAKEFLAGS are built together from the
# parent MAKEFLAGS and the flags from the command line, in particular that
# variable assignments are passed at the end, after the options.
append_stage_0:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@${MAKE} -Dbefore-0 -f ${MAKEFILE} append_stage_1 VAR0=value -Dafter-0

append_stage_1:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@${MAKE} -Dbefore-1 -f ${MAKEFILE} append_stage_2 VAR1=value -Dafter-1

append_stage_2:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@${MAKE} -Dbefore-2 -f ${MAKEFILE} append_stage_3 VAR2=value -Dafter-2

append_stage_3:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
