#	$NetBSD: Makefile.inc,v 1.18 2006/06/17 18:04:23 uwe Exp $

SRCS+=	${ASSRCS}
LSRCS+= ${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
DPSRCS+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}
CLEANFILES+=${ASSRCS:S/S$/c/g:C/^./Lint_&/g}

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S
