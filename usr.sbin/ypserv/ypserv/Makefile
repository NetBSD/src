#	$NetBSD: Makefile,v 1.11 1999/01/22 02:36:12 thorpej Exp $

PROG=	ypserv
SRCS=	ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.8

LIBCDIR=${.CURDIR}/../../../lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -DLIBWRAP -I. -I${LIBCDIR}/include
YHEADER=1

LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

.include <bsd.prog.mk>

gethnamaddr.o: gethnamaddr.c
	${COMPILE.c} -UYP ${.IMPSRC}

getnetnamadr.o: getnetnamadr.c
	${COMPILE.c} -UYP ${.IMPSRC}
