#	$NetBSD: Makefile,v 1.10 2001/10/24 02:31:10 thorpej Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	xlint
SRCS=	xlint.c mem.c
.if defined(XLINT_TARGET_PREFIX)
CPPFLAGS+=-DTARGET_PREFIX=\"${XLINT_TARGET_PREFIX}\"
PROGNAME= ${XLINT_TARGET_PREFIX}lint
MAN=	${XLINT_TARGET_PREFIX}lint.1
.else
PROGNAME= lint
MAN=	lint.1
.endif

.if defined(XLINT_TARGET_PREFIX)
${XLINT_TARGET_PREFIX}lint1: ${.CURDIR}/lint.1
	cp ${.IMPSRC} ${.TARGET}
.endif

CPPFLAGS+=-I${.CURDIR}/../lint1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
