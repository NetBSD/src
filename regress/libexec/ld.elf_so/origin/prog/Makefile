# $NetBSD: Makefile,v 1.3 2007/05/19 20:52:03 christos Exp $

.include <bsd.own.mk>

MKMAN=no
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
