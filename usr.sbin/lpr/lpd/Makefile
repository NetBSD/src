#	$NetBSD: Makefile,v 1.17 2003/10/19 05:51:44 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	lpd
MAN=	lpd.8
SRCS=	lpd.c printjob.c recvjob.c lpdchar.c key.c modes.c ttcompat.c rcmd.c

CPPFLAGS+=-DLIBWRAP
LDADD+=	-lwrap
DPADD+=	${LIBWRAP}

CPPFLAGS.rcmd.c=	-DINET6

.PATH:	${NETBSDSRCDIR}/lib/libc/net

.include <bsd.prog.mk>
