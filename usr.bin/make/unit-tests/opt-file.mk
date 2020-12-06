# $NetBSD: opt-file.mk,v 1.3 2020/12/06 19:00:48 rillig Exp $
#
# Tests for the -f command line option.

# TODO: Implementation

all: .PHONY file-ending-in-backslash

# Passing '-' as the filename reads from stdin.  This is unusual but possible.
#
# In the unlikely case where a file ends in a backslash instead of a newline,
# that backslash is trimmed.  See ParseGetLine.
file-ending-in-backslash: .PHONY
	@printf '%s' 'VAR=value\' | ${MAKE} -r -f - -v VAR

all:
	@:;
