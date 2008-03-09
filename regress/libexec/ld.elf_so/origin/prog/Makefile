# $NetBSD: Makefile,v 1.4 2008/03/09 22:44:41 dholland Exp $

NOMAN=# defined (must be above bsd.own.mk)

.include <bsd.own.mk>

PROG=origin
LIBFOO != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LDFLAGS+=-Wl,-R'$${ORIGIN}' -Wl,-L${LIBFOO} -lfoo
CPPFLAGS+=-I${.CURDIR}/../lib

${PROG}: libfoo.so.0

libfoo.so.0:
	-@cp ${LIBFOO}/libfoo.so.0 .

CLEANFILES+=libfoo.so.0

regress: ${PROG}
	./${PROG}

.include <bsd.prog.mk>
