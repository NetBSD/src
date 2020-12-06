# $NetBSD: opt-file.mk,v 1.4 2020/12/06 19:18:26 rillig Exp $
#
# Tests for the -f command line option.

# TODO: Implementation

all: .PHONY
all: file-ending-in-backslash
all: file-containing-null-byte

# Passing '-' as the filename reads from stdin.  This is unusual but possible.
#
# In the unlikely case where a file ends in a backslash instead of a newline,
# that backslash is trimmed.  See ParseGetLine.
file-ending-in-backslash: .PHONY
	@printf '%s' 'VAR=value\' \
	| ${MAKE} -r -f - -v VAR

# If a file contains null bytes, the rest of the line is skipped, and parsing
# continues in the next line.
#
# XXX: It would be safer to just quit parsing in such a situation.
file-containing-null-byte: .PHONY
	@printf '%s\n' 'VAR=value' 'VAR2=VALUE2' \
	| tr 'l' '\0' \
	| ${MAKE} -r -f - -v VAR -v VAR2

all:
	@:;
