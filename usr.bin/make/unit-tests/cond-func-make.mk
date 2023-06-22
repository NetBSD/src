# $NetBSD: cond-func-make.mk,v 1.4 2023/06/22 09:09:08 rillig Exp $
#
# Tests for the make() function in .if conditions, which tests whether
# the argument has been passed as a target via the command line or later
# via the .MAKEFLAGS special dependency target.

.if !make(via-cmdline)
.  error
.endif
.if make(via-dot-makeflags)
.  error
.endif

.MAKEFLAGS: via-dot-makeflags

.if !make(via-cmdline)
.  error
.endif
.if !make(via-dot-makeflags)
.  error
.endif

# TODO: warn about the malformed pattern
.if make([)
.  error
.endif

via-cmdline via-dot-makeflags:
	: $@
