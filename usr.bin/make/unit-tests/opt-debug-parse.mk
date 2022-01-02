# $NetBSD: opt-debug-parse.mk,v 1.2 2022/01/02 03:23:55 rillig Exp $
#
# Tests for the -dp command line option, which adds debug logging about
# makefile parsing.

.MAKEFLAGS: -dp

# TODO: Implementation

# In PrintStackTrace, the line number of the .for loop is wrong.  The actual
# line number printed is the last line before the loop body, while it should
# rather be the line number where the .for loop starts.
.for \
    var \
    in \
    value
.info trace with multi-line .for loop head
.endfor
# FIXME: The .exp file says 'in .include from opt-debug-parse.mk:18', which is
# completely wrong.  It should rather say 'in .for loop from :13'.

.MAKEFLAGS: -d0

all: .PHONY
