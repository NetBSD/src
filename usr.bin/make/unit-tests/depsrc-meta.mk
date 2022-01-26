# $NetBSD: depsrc-meta.mk,v 1.5 2022/01/26 22:19:25 rillig Exp $
#
# Tests for the special source .META in dependency declarations.

# TODO: Implementation
# TODO: Explanation

.MAIN: all

.if make(actual-test)
.MAKEFLAGS: -dM
.MAKE.MODE=	meta curDirOk=true
.endif

actual-test: depsrc-meta-target
depsrc-meta-target: .META
	@> ${.TARGET}-file
	@rm -f ${.TARGET}-file

check-results:
	@echo 'Targets from meta mode:'
	@awk '/^TARGET/ { print "| " $$0 }' depsrc-meta-target.meta
	@rm depsrc-meta-target.meta

all:
	@${MAKE} -f ${MAKEFILE} actual-test
	@${MAKE} -f ${MAKEFILE} check-results
