#	$NetBSD: Makefile,v 1.13 2001/01/12 01:43:48 cgd Exp $

PROG=	ypserv
SRCS=	ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.8

LIBCDIR=${.CURDIR}/../../../lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -DLIBWRAP -I. -I${LIBCDIR}/include
YHEADER=1

LDADD+=	-lwrap -lutil
DPADD+=	${LIBWRAP} ${LIBUTIL}

.include <bsd.prog.mk>

gethnamaddr.o: gethnamaddr.c
	${COMPILE.c} -UYP -D_LIBC ${.IMPSRC}

getnetnamadr.o: getnetnamadr.c
	${COMPILE.c} -UYP -D_LIBC ${.IMPSRC}
