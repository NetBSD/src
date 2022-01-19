# $NetBSD: dep-duplicate.mk,v 1.1 2022/01/19 22:10:41 rillig Exp $
#
# Test for a target whose commands are defined twice.  This generates a
# warning, not an error, so ensure that the correct commands are kept.
#
# Also ensure that the diagnostics mention the correct file in case of
# included files.  This particular case had been broken in parse.c 1.231 from
# 2018-12-22.

# expect: make: "dep-duplicate.inc" line 15: warning: using previous script for "all" defined here
# FIXME: The file "dep-duplicate.inc" has no line 15.

# expect: first
all: .PHONY
	@echo first

_!=	echo 'all:;echo second' > dep-duplicate.inc
.include "${.CURDIR}/dep-duplicate.inc"

.END:
	@rm -f dep-duplicate.inc
