# $NetBSD: directive-unexport.mk,v 1.3 2020/10/30 17:55:10 rillig Exp $
#
# Tests for the .unexport directive.

# TODO: Implementation

# First, export 3 variables.
A=	a
B=	b
C=	c
.export A B C

# Show the exported variables and their values.
.info ${:!env|sort|grep -v -E '^(MAKELEVEL|MALLOC_OPTIONS|PATH|PWD)'!}
.info ${.MAKE.EXPORTED}

# XXX: Now try to unexport all of them.  The variables are still exported
# but not mentioned in .MAKE.EXPORTED anymore.
# See the ":N" in Var_UnExport for the implementation.
*=	asterisk
.unexport *

.info ${:!env|sort|grep -v -E '^(MAKELEVEL|MALLOC_OPTIONS|PATH|PWD)'!}
.info ${.MAKE.EXPORTED}

all:
	@:;
