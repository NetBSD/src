# $NetBSD: Makefile,v 1.5 2009/01/28 16:11:23 christos Exp $

NOMAN=# defined (must be above bsd.own.mk)

.include <bsd.own.mk>

PROG=origin
LIBFOO != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
PROGDIR != cd ${.CURDIR} && ${PRINTOBJDIR}
LDFLAGS+=-Wl,-R'$${ORIGIN}' -Wl,-L${LIBFOO} -lfoo
CPPFLAGS+=-I${.CURDIR}/../lib

${PROG}: libfoo.so.0

libfoo.so.0:
	-@cp ${LIBFOO}/libfoo.so.0 .

CLEANFILES+=libfoo.so.0

#	can't use ./${PROG} here because the kernel is still broken
#	checkout the #ifdef notyet in kern_exec.c
regress: ${PROG}
	${PROGDIR}/${PROG}

.include <bsd.prog.mk>
