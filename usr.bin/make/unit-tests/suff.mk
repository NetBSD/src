# $NetBSD: suff.mk,v 1.1 2025/01/10 23:00:38 rillig Exp $
#
# Demonstrate suffix rules and dependency resolution.

all: .PHONY edge-case.to everything

.MAKEFLAGS: -dsv

.SUFFIXES: .from .to

.from.to:
	: Making ${.TARGET} from ${.ALLSRC}.

# When making this target, ${.ARCHIVE} is undefined, but there's no warning.
# expect: Var_Parse: ${.ARCHIVE}.additional (eval-defined)
edge-case.to: ${.PREFIX}${.ARCHIVE}.additional

edge-case.from edge-case.additional:
	: Making ${.TARGET} out of nothing.

everything: .PHONY a*.mk
	: Making ${.TARGET} from ${.ALLSRC}.
