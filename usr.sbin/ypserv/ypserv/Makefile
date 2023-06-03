#	$NetBSD: Makefile,v 1.22 2023/06/03 09:09:25 lukem Exp $

.include <bsd.own.mk>

PROG=	ypserv
SRCS=	ypserv.c ypserv_proc.c ypserv_db.c ypserv_xdr.c \
	gethnamaddr.c getnetnamadr.c
MAN=	ypserv.8

LIBCDIR=${NETBSDSRCDIR}/lib/libc
.PATH:	${LIBCDIR}/net
CPPFLAGS+=-DOPTIMIZE_DB -DLIBWRAP -I. -I${LIBCDIR}/include
YHEADER=1

LDADD+=	-lwrap -lblocklist -lutil
DPADD+=	${LIBWRAP} ${LIBBLOCKLIST} ${LIBUTIL}

CPPFLAGS.gethnamaddr.c=	-UYP -D_LIBC
CPPFLAGS.getnetnamadr.c=-UYP -D_LIBC

SANITIZER_RENAME_SYMBOL+=	gethostbyname2

COPTS.ypserv.c+=	${CC_WNO_CAST_FUNCTION_TYPE}

.include <bsd.prog.mk>
