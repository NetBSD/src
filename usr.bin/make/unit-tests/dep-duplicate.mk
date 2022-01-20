# $NetBSD: dep-duplicate.mk,v 1.2 2022/01/20 19:16:25 rillig Exp $
#
# Test for a target whose commands are defined twice.  This generates a
# warning, not an error, so ensure that the correct commands are kept.
#
# Also ensure that the diagnostics mention the correct file in case of
# included files.  Since parse.c 1.231 from 2018-12-22 and before parse.c
# 1.653 from 2022-01-20, the wrong filename had been printed if the file of
# the first commands section was in a file that was included by its relative
# path.

# expect: make: "dep-duplicate.inc" line 3: warning: using previous script for "all" defined here
# FIXME: The file "dep-duplicate.inc" has no line 3.

all: .PHONY
	@exec > dep-duplicate.main; \
	echo '# empty line 1'; \
	echo '# empty line 2'; \
	echo 'all:; @echo main-output'; \
	echo '.include "dep-duplicate.inc"'

	@exec > dep-duplicate.inc; \
	echo 'all:; @echo inc-output'

	# The main file must be specified using a relative path, just like the
	# default 'makefile' or 'Makefile', to produce the same result when
	# run via ATF or 'make test'.
	@${MAKE} -r -f dep-duplicate.main

	@rm -f dep-duplicate.main
	@rm -f dep-duplicate.inc
