# $NetBSD: varname-dot-makeflags.mk,v 1.10 2025/05/18 22:51:02 rillig Exp $
#
# Tests for the special .MAKEFLAGS variable, which collects almost all
# command line arguments and passes them on to any child processes via
# the environment variable MAKEFLAGS (without leading '.').
#
# See also:
#	varname-dot-makeoverrides.mk

# expect+1: MAKEFLAGS=<undefined>
.info MAKEFLAGS=<${MAKEFLAGS:Uundefined}>
# expect+1: .MAKEFLAGS=< -r -k>
.info .MAKEFLAGS=<${.MAKEFLAGS}>
# expect+1: .MAKEOVERRIDES=<>
.info .MAKEOVERRIDES=<${.MAKEOVERRIDES:Uundefined}>

# Append an option with argument, a plain option and a variable assignment.
.MAKEFLAGS: -DVARNAME -r VAR=value

# expect+1: MAKEFLAGS=<undefined>
.info MAKEFLAGS=<${MAKEFLAGS:Uundefined}>
# expect+1: .MAKEFLAGS=< -r -k -D VARNAME -r>
.info .MAKEFLAGS=<${.MAKEFLAGS}>
# expect+1: .MAKEOVERRIDES=< VAR>
.info .MAKEOVERRIDES=<${.MAKEOVERRIDES}>

# The environment variable 'MAKEFLAGS' is available to child processes
# while parsing the makefiles, just like the exported make variables.
.if ${:!echo "\${MAKEFLAGS-undef}"!} != " -r -k -D VARNAME -r VAR=value"
.  error
.endif

# After parsing, the environment variable 'MAKEFLAGS' is set based on the
# special variables '.MAKEFLAGS' and '.MAKEOVERRIDES'.
runtime:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@echo '$@: .MAKEFLAGS=<'${.MAKEFLAGS:Q}'>'
	@echo '$@: .MAKEOVERRIDES=<'${.MAKEOVERRIDES:Q}'>'
