# $NetBSD: directive-export-literal.mk,v 1.3 2020/10/03 09:37:04 rillig Exp $
#
# Tests for the .export-literal directive, which exports a variable value
# without expanding it.

UT_VAR=		value with ${UNEXPANDED} expression

.export-literal UT_VAR

all:
	@echo "$$UT_VAR"
