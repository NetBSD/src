#	$NetBSD: Makefile,v 1.6 2002/09/18 14:00:35 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	chflags
SRCS=	chflags.c stat_flags.c
.PATH:	${NETBSDSRCDIR}/bin/ls
CPPFLAGS+=	-I${NETBSDSRCDIR}/bin/ls

.include <bsd.prog.mk>
