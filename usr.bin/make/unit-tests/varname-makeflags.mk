# $NetBSD: varname-makeflags.mk,v 1.4 2021/12/27 20:17:35 rillig Exp $
#
# Tests for the special MAKEFLAGS variable, which is basically just a normal
# environment variable.  It is closely related to .MAKEFLAGS but captures the
# state of .MAKEFLAGS at the very beginning of make, before any makefiles are
# read.

# TODO: Implementation

.MAKEFLAGS: -d0

# The unit tests are run with an almost empty environment.  In particular,
# the variable MAKEFLAGS is not set.  The '.MAKEFLAGS:' above also doesn't
# influence the environment variable MAKEFLAGS, therefore it is still
# undefined at this point.
.if ${MAKEFLAGS:Uundefined} != "undefined"
.  error
.endif

# The special variable .MAKEFLAGS is influenced though.
# See varname-dot-makeflags.mk for more details.
.if ${.MAKEFLAGS} != " -r -k -d 0"
.  error
.endif


# In POSIX mode, the environment variable MAKEFLAGS can contain letters only,
# for compatibility.  These letters are exploded to form regular options.
OUTPUT!=	env MAKEFLAGS=ikrs ${MAKE} -f /dev/null -v .MAKEFLAGS
.if ${OUTPUT} != " -i -k -r -s -V .MAKEFLAGS"
.  error
.endif

# As soon as there is a single non-alphabetic character in the environment
# variable MAKEFLAGS, it is no longer split.  In this example, the word
# "d0ikrs" is treated as a target, but the option '-v' prevents any targets
# from being built.
OUTPUT!=	env MAKEFLAGS=d0ikrs ${MAKE} -f /dev/null -v .MAKEFLAGS
.if ${OUTPUT} != " -V .MAKEFLAGS"
.  error ${OUTPUT}
.endif


all:
