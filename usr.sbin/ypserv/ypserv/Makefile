#	$NetBSD: Makefile,v 1.16 2006/05/11 23:16:31 mrg Exp $

.include <bsd.own.mk>

PROG=	ypserv
SRCS=	ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.8

LIBCDIR=${NETBSDSRCDIR}/lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DYY_NO_UNPUT -DOPTIMIZE_DB -DLIBWRAP -I. -I${LIBCDIR}/include
YHEADER=1

LDADD+=	-lwrap -lutil
DPADD+=	${LIBWRAP} ${LIBUTIL}

CPPFLAGS.gethnamaddr.c=	-UYP -D_LIBC
CPPFLAGS.getnetnamadr.c=-UYP -D_LIBC

.if ${HAVE_GCC} == 4
COPTS.ypserv_db.c=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>
