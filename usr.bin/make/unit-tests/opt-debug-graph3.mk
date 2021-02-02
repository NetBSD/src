# $NetBSD: opt-debug-graph3.mk,v 1.2 2021/02/02 17:27:35 rillig Exp $
#
# Tests for the -dg3 command line option, which prints the input
# graph before exiting on error.
#
# FIXME: The documentation is wrong.  There is no debug output despite
# the error.

.MAKEFLAGS: -dg3

.MAIN: all

made-target: .PHONY
	: 'Making $@.'

error-target: .PHONY
	false

aborted-target: .PHONY aborted-target-dependency
aborted-target-dependency: .PHONY
	false

all: made-target error-target aborted-target
