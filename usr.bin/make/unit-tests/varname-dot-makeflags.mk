# $NetBSD: varname-dot-makeflags.mk,v 1.6 2023/02/25 11:59:12 rillig Exp $
#
# Tests for the special .MAKEFLAGS variable, which collects almost all
# command line arguments and passes them on to any child processes via
# the environment variable MAKEFLAGS (without leading '.').
#
# See also:
#	varname-dot-makeoverrides.mk

.info MAKEFLAGS=<${MAKEFLAGS:Uundefined}>
.info .MAKEFLAGS=<${.MAKEFLAGS}>
.info .MAKEOVERRIDES=<${.MAKEOVERRIDES:Uundefined}>

# Append an option with argument, a plain option and a variable assignment.
.MAKEFLAGS: -DVARNAME -r VAR=value

.info MAKEFLAGS=<${MAKEFLAGS:Uundefined}>
.info .MAKEFLAGS=<${.MAKEFLAGS}>
.info .MAKEOVERRIDES=<${.MAKEOVERRIDES}>

# After parsing, the environment variable 'MAKEFLAGS' is set based on
runtime:
	@echo '$@: MAKEFLAGS=<'${MAKEFLAGS:Q}'>'
	@echo '$@: .MAKEFLAGS=<'${.MAKEFLAGS:Q}'>'
	@echo '$@: .MAKEOVERRIDES=<'${.MAKEOVERRIDES:Q}'>'
