#	$NetBSD: Makefile,v 1.7 2005/01/12 16:42:45 xtraeme Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

WARNS=	3
PROG=	chflags
SRCS=	chflags.c stat_flags.c
.PATH:	${NETBSDSRCDIR}/bin/ls
CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls

.include <bsd.prog.mk>
