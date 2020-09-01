# $NetBSD: opt-m-include-dir.mk,v 1.3 2020/09/01 19:17:58 rillig Exp $
#
# Tests for the -m command line option.

.MAKEFLAGS: -m .../buf.c
.MAKEFLAGS: -m .../does-not-exist
.MAKEFLAGS: -m .../${.PARSEFILE:T}

# Whether or not buf.c exists depends on whether the source code of make
# is available.  When running the tests in src/usr.bin/make, it succeeds,
# and when running the tests in src/tests/usr.bin/make, it fails.

# This file should never exist.
.if exists(does-not-exist)
.  error
.endif

# This test assumes that this test is run in the same directory as the
# test file.
.if !exists(${.PARSEFILE})
.  error
.endif

all:
	@:;
