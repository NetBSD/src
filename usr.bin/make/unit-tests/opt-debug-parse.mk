# $NetBSD: opt-debug-parse.mk,v 1.5 2022/01/08 23:41:43 rillig Exp $
#
# Tests for the -dp command line option, which adds debug logging about
# makefile parsing.

.MAKEFLAGS: -dp

# TODO: Implementation

# Before parse.c 1.639 from 2022-01-08, PrintStackTrace and other diagnostics
# printed a wrong line number, using the last line before the loop body, while
# it should rather be the line number where the .for loop starts.
#
# Before parse.c 1.643 from 2022-01-08, PrintStackTrace tried to be too clever
# by merging stack trace entries, printing confusing line numbers as a result.
.for \
    var \
    in \
    value
.info trace with multi-line .for loop head
.endfor

# Before parse.c 1.461 from 2022-01-08, the debug log said it returned to
# the line of the '.include' instead of the line following it.
.include "/dev/null"

.MAKEFLAGS: -d0

all: .PHONY
