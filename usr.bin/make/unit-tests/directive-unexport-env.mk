# $NetBSD: directive-unexport-env.mk,v 1.5 2020/12/06 17:29:27 rillig Exp $
#
# Tests for the .unexport-env directive.

# TODO: Implementation

.unexport-en			# oops: misspelled
.unexport-env			# ok
.unexport-environment		# oops: misspelled

# As of 2020-12-06, the directive unexport-env is not supposed to accept
# arguments, but it does without complaining about them.
.MAKEFLAGS: -dv
UT_EXPORTED=	value
UT_UNEXPORTED=	value
.export UT_EXPORTED
.unexport-env UT_EXPORTED UT_UNEXPORTED
.MAKEFLAGS: -d0

all:
	@:;
