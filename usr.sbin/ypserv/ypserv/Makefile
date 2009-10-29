#	$NetBSD: Makefile,v 1.18 2009/10/29 14:41:02 christos Exp $

.include <bsd.own.mk>

PROG=	ypserv
SRCS=	ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.8

LIBCDIR=${NETBSDSRCDIR}/lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DOPTIMIZE_DB -DLIBWRAP -I. -I${LIBCDIR}/include
YHEADER=1

LDADD+=	-lwrap -lutil
DPADD+=	${LIBWRAP} ${LIBUTIL}

CPPFLAGS.gethnamaddr.c=	-UYP -D_LIBC
CPPFLAGS.getnetnamadr.c=-UYP -D_LIBC

.include <bsd.prog.mk>
