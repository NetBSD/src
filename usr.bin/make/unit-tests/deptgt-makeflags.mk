# $NetBSD: deptgt-makeflags.mk,v 1.3 2020/09/10 21:22:07 rillig Exp $
#
# Tests for the special target .MAKEFLAGS in dependency declarations,
# which adds command line options later, at parse time.

# The -D option sets a variable in the "Global" scope and thus can be
# undefined later.
.MAKEFLAGS: -D VAR

.if ${VAR} != 1
.  error
.endif

.undef VAR

.if defined(VAR)
.  error
.endif

.MAKEFLAGS: -D VAR

.if ${VAR} != 1
.  error
.endif

.MAKEFLAGS: VAR="value"' with'\ spaces

.if ${VAR} != "value with spaces"
.  error
.endif

# Variables set on the command line as VAR=value are placed in the
# "Command" scope and thus cannot be undefined.
.undef VAR

.if ${VAR} != "value with spaces"
.  error
.endif

all:
	@:;
