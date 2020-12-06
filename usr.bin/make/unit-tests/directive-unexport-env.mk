# $NetBSD: directive-unexport-env.mk,v 1.4 2020/12/06 16:06:11 rillig Exp $
#
# Tests for the .unexport-env directive.

# TODO: Implementation

.unexport-en			# oops: misspelled
.unexport-env			# ok
.unexport-environment		# oops: misspelled

# Before 2020-12-06, the directive unexport-env was implemented strangely.
# According to its documentation, it does not take any arguments, but the
# Implementation accepted variable names as arguments and produced wrong debug
# logging for them, saying "Unexporting" for variables that at this point were
# not exported anymore.
.MAKEFLAGS: -dv
UT_EXPORTED=	value
UT_UNEXPORTED=	value
.export UT_EXPORTED
.unexport-env UT_EXPORTED UT_UNEXPORTED
.MAKEFLAGS: -d0

all:
	@:;
