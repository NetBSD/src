# $NetBSD: directive-for-null.mk,v 1.4.2.1 2025/08/02 05:58:34 perseant Exp $
#
# Test for parsing a .for loop that accidentally contains a null byte.
#
# expect: make: (stdin):2: Zero byte read from file

all: .PHONY
	@printf '%s\n' \
	    '.for i in 1 2 3' \
	    'VAR=value' \
	    '.endfor' \
	| tr 'l' '\0' \
	| ${MAKE} -f -
