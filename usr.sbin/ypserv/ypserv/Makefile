#	$NetBSD: Makefile,v 1.10 1999/01/19 03:53:27 lukem Exp $

PROG=	ypserv
SRCS=	acl.c gram.y scan.l ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.acl.5 securenet.5 ypserv.8

LIBCDIR=${.CURDIR}/../../../lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -I. -I${LIBCDIR}/include
YHEADER=1

.include <bsd.prog.mk>

gethnamaddr.o: gethnamaddr.c
	${COMPILE.c} -UYP ${.IMPSRC}

getnetnamadr.o: getnetnamadr.c
	${COMPILE.c} -UYP ${.IMPSRC}
