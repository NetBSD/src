# $NetBSD: var-class-global.mk,v 1.3 2022/01/23 16:09:38 rillig Exp $
#
# Tests for global variables, which are the most common variables.

# Global variables can be assigned and appended to.
GLOBAL=		value
GLOBAL+=	addition
.if ${GLOBAL} != "value addition"
.  error
.endif

# Global variables can be removed from their scope.
.undef GLOBAL
.if defined(GLOBAL)
.  error
.endif

all: .PHONY
