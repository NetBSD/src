# $NetBSD: varname-dot-newline.mk,v 1.5 2023/01/17 19:42:47 rillig Exp $
#
# Tests for the special .newline variable, which contains a single newline
# character (U+000A).


# https://austingroupbugs.net/view.php?id=1549 proposes:
# > After all macro expansion is complete, when an escaped <newline> is
# > found in a command line in a makefile, the command line that is executed
# > shall contain the <backslash>, the <newline>, and the next line, except
# > that the first character of the next line shall not be included if it is
# > a <tab>.
#
# The above quote assumes that each resulting <newline> character has a "next
# line", but that's not how the .newline variable works.
BACKSLASH_NEWLINE:=	\${.newline}


# Contrary to the special variable named "" that is used in expressions like
# ${:Usome-value}, the variable ".newline" is not protected against
# modification.  Nobody exploits that though.

NEWLINE:=	${.newline}

.newline=	overwritten

.if ${.newline} == ${NEWLINE}
.  info The .newline variable cannot be overwritten.  Good.
.else
.  info The .newline variable can be overwritten.  Just don't do that.
.endif

# Restore the original value.
.newline=	${NEWLINE}

all:
	@echo 'first${.newline}second'
	@echo 'backslash newline: <${BACKSLASH_NEWLINE}>'
