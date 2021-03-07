#	$NetBSD: Makefile,v 1.19 2021/03/07 15:09:12 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap -lblocklist
DPADD+=	${LIBWRAP} ${LIBBLOCKLIST}

.if (${USE_INET6} != "no")
CPPFLAGS.rcmd.c=	-DINET6
.endif

.PATH:	${NETBSDSRCDIR}/lib/libc/net

.include <bsd.prog.mk>
